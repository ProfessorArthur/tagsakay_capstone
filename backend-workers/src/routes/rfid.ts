import { Hono } from "hono";
import {
  deviceAuthMiddleware,
  authMiddleware,
  requireRole,
} from "../middleware/auth";
import {
  rfids,
  rfidScans,
  users,
  NewRfidScan,
  type Device,
} from "../db/schema";
import { eq, desc, and, isNull, sql } from "drizzle-orm";
import type { Database } from "../db";

type Env = {
  Bindings: {
    DATABASE_URL: string;
    JWT_SECRET: string;
    SESSION_SECRET: string;
  };
  Variables: {
    db: Database;
    device?: Device;
    user?: any;
  };
};

const app = new Hono<Env>();

const normalizeTagId = (value: unknown): string => {
  if (typeof value !== "string") return "";
  return value.trim().toUpperCase();
};

const fetchUnregisteredTags = async (db: Database, limit: number) => {
  const unregisteredScans = await db
    .select()
    .from(rfidScans)
    .where(and(eq(rfidScans.status, "failed"), isNull(rfidScans.userId)))
    .orderBy(desc(rfidScans.scanTime))
    .limit(limit);

  const uniqueTags = new Map<
    string,
    {
      id: string;
      tagId: string;
      lastSeen: Date;
      deviceId: string | null;
      location: string | null;
      scanCount: number;
    }
  >();

  unregisteredScans.forEach((scan) => {
    const key = normalizeTagId(scan.rfidTagId);
    if (!uniqueTags.has(key)) {
      uniqueTags.set(key, {
        id: scan.id,
        tagId: key,
        lastSeen: scan.scanTime,
        deviceId: scan.deviceId,
        location: scan.location,
        scanCount: 1,
      });
    } else {
      const existing = uniqueTags.get(key)!;
      existing.scanCount += 1;
      if (!existing.id) existing.id = scan.id;
      if (scan.scanTime > existing.lastSeen) {
        existing.lastSeen = scan.scanTime;
        existing.deviceId = scan.deviceId;
        existing.location = scan.location;
      }
    }
  });

  if (uniqueTags.size > 0) {
    const tagIds = Array.from(uniqueTags.keys());
    const registeredTags = await Promise.all(
      tagIds.map(async (tagId) => {
        const [exists] = await db
          .select({ id: rfids.id })
          .from(rfids)
          .where(sql`${rfids.tagId} ILIKE ${tagId}`)
          .limit(1);

        return exists ? tagId : null;
      })
    );

    registeredTags
      .filter((tagId): tagId is string => Boolean(tagId))
      .forEach((tagId) => uniqueTags.delete(tagId));
  }

  const unregisteredTags = Array.from(uniqueTags.values());

  return {
    unregisteredTags,
    total: unregisteredTags.length,
    scansChecked: unregisteredScans.length,
  };
};

// POST /api/rfid/scan - Handle RFID scan from ESP32 device
app.post("/scan", deviceAuthMiddleware, async (c) => {
  try {
    const db = c.get("db");
    const device = c.get("device");

    if (!device) {
      return c.json(
        {
          success: false,
          message: "Device not authenticated",
        },
        401
      );
    }

    const { tagId, location, vehicleId } = await c.req.json();
    const normalizedTagId = normalizeTagId(tagId);

    if (!normalizedTagId) {
      return c.json(
        {
          success: false,
          message: "Missing required field: tagId is required",
        },
        400
      );
    }

    console.log(
      `RFID scan attempt: ${normalizedTagId} from device ${device.deviceId}`
    );

    // Check if RFID exists and is active
    const [rfidTag] = await db
      .select()
      .from(rfids)
      .leftJoin(users, eq(rfids.userId, users.id))
      .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`)
      .limit(1);

    if (!rfidTag || !rfidTag.Rfids) {
      console.warn(`Unregistered RFID: ${normalizedTagId}`);

      // Record failed scan attempt
      const failedScan: NewRfidScan = {
        rfidTagId: normalizedTagId,
        deviceId: device.deviceId,
        location: location || null,
        vehicleId: vehicleId || null,
        status: "failed",
        eventType: "unknown",
        metadata: { reason: "Tag not registered" },
      };

      await db.insert(rfidScans).values(failedScan);

      return c.json(
        {
          success: false,
          message: "RFID tag not registered",
          data: { tagId: normalizedTagId, registered: false },
        },
        404
      );
    }

    const { Rfids: rfid, Users: user } = rfidTag;

    // Check if RFID is active
    if (!rfid.isActive) {
      const inactiveScan: NewRfidScan = {
        rfidTagId: normalizedTagId,
        deviceId: device.deviceId,
        userId: user?.id || null,
        location: location || null,
        vehicleId: vehicleId || null,
        status: "unauthorized",
        eventType: "unknown",
        metadata: { reason: "Tag is inactive" },
      };

      await db.insert(rfidScans).values(inactiveScan);

      return c.json(
        {
          success: false,
          message: "RFID tag is inactive",
          data: { tagId: normalizedTagId, active: false },
        },
        403
      );
    }

    // Check if user is active (if associated)
    if (user && !user.isActive) {
      const unauthorizedScan: NewRfidScan = {
        rfidTagId: normalizedTagId,
        deviceId: device.deviceId,
        userId: user.id,
        location: location || null,
        vehicleId: vehicleId || null,
        status: "unauthorized",
        eventType: "unknown",
        metadata: { reason: "User is inactive" },
      };

      await db.insert(rfidScans).values(unauthorizedScan);

      return c.json(
        {
          success: false,
          message: "User associated with this RFID is inactive",
          data: {
            tagId: normalizedTagId,
            userName: user.name,
            userActive: false,
          },
        },
        403
      );
    }

    // Record successful scan
    const successScan: NewRfidScan = {
      rfidTagId: normalizedTagId,
      deviceId: device.deviceId,
      userId: user?.id || null,
      location: location || null,
      vehicleId: vehicleId || null,
      status: "success",
      eventType: "entry", // Default to entry, can be customized
      metadata: {},
    };

    const [scan] = await db.insert(rfidScans).values(successScan).returning();

    // Update RFID last scanned
    await db
      .update(rfids)
      .set({
        lastScanned: new Date(),
        deviceId: device.deviceId,
      })
      .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`);

    return c.json({
      success: true,
      message: "Scan recorded successfully",
      data: {
        scan: {
          id: scan.id,
          tagId: normalizedTagId,
          scanTime: scan.scanTime,
          status: scan.status,
          eventType: scan.eventType,
        },
        user: user
          ? {
              id: user.id,
              name: user.name,
              role: user.role,
            }
          : null,
        rfid: {
          tagId: normalizeTagId(rfid.tagId),
          isActive: rfid.isActive,
        },
      },
    });
  } catch (error: any) {
    console.error("Scan error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to process scan",
        error: error.message,
      },
      500
    );
  }
});

// GET /api/rfid - List all RFIDs (admin/superadmin only)
app.get("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");

    const allRfids = await db
      .select({
        id: rfids.id,
        tagId: rfids.tagId,
        userId: rfids.userId,
        isActive: rfids.isActive,
        unitNumber: rfids.unitNumber,
        lastScanned: rfids.lastScanned,
        deviceId: rfids.deviceId,
        registeredBy: rfids.registeredBy,
        metadata: rfids.metadata,
        createdAt: rfids.createdAt,
        updatedAt: rfids.updatedAt,
        user: {
          id: users.id,
          name: users.name,
          email: users.email,
          role: users.role,
        },
      })
      .from(rfids)
      .leftJoin(users, eq(rfids.userId, users.id))
      .orderBy(desc(rfids.createdAt));

    return c.json({
      success: true,
      message: `Retrieved ${allRfids.length} RFID tags`,
      data: {
        rfids: allRfids,
        total: allRfids.length,
      },
    });
  } catch (error: any) {
    console.error("List RFIDs error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to retrieve RFIDs",
        error: error.message,
      },
      500
    );
  }
});

// GET /api/rfid/:tagId - Get specific RFID by tag ID
app.get("/:tagId", authMiddleware, async (c) => {
  try {
    const db = c.get("db");
    const tagIdParam = c.req.param("tagId");
    const normalizedTagId = normalizeTagId(tagIdParam);

    if (!normalizedTagId) {
      return c.json(
        {
          success: false,
          message: "Invalid RFID tag ID",
        },
        400
      );
    }

    const [rfidData] = await db
      .select({
        id: rfids.id,
        tagId: rfids.tagId,
        isActive: rfids.isActive,
        unitNumber: rfids.unitNumber,
        lastScanned: rfids.lastScanned,
        deviceId: rfids.deviceId,
        metadata: rfids.metadata,
        createdAt: rfids.createdAt,
        updatedAt: rfids.updatedAt,
        user: {
          id: users.id,
          name: users.name,
          email: users.email,
          role: users.role,
          isActive: users.isActive,
        },
      })
      .from(rfids)
      .leftJoin(users, eq(rfids.userId, users.id))
      .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`)
      .limit(1);

    if (!rfidData) {
      return c.json(
        {
          success: false,
          message: "RFID tag not found",
        },
        404
      );
    }

    // Get recent scans for this tag
    const recentScans = await db
      .select()
      .from(rfidScans)
      .where(sql`${rfidScans.rfidTagId} ILIKE ${normalizedTagId}`)
      .orderBy(desc(rfidScans.scanTime))
      .limit(10);

    return c.json({
      success: true,
      message: "RFID tag retrieved successfully",
      data: {
        rfid: {
          ...rfidData,
          tagId: normalizeTagId(rfidData.tagId),
        },
        recentScans: recentScans,
      },
    });
  } catch (error: any) {
    console.error("Get RFID error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to retrieve RFID",
        error: error.message,
      },
      500
    );
  }
});

// POST /api/rfid/register - Register new RFID tag (admin/superadmin only)
app.post(
  "/register",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const user = c.get("user");
      const {
        tagId,
        userId,
        isActive = true,
        metadata = {},
        unitNumber,
      } = await c.req.json();

      const normalizedTagId = normalizeTagId(tagId);

      if (!normalizedTagId) {
        return c.json(
          {
            success: false,
            message: "tagId is required",
          },
          400
        );
      }

      // Check if tag already exists
      const [existingTag] = await db
        .select()
        .from(rfids)
        .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`)
        .limit(1);

      if (existingTag) {
        return c.json(
          {
            success: false,
            message: "RFID tag already registered",
          },
          409
        );
      }

      // If userId provided, verify user exists
      if (userId) {
        const [userExists] = await db
          .select()
          .from(users)
          .where(eq(users.id, userId))
          .limit(1);

        if (!userExists) {
          return c.json(
            {
              success: false,
              message: "User not found",
            },
            404
          );
        }
      }

      // Create new RFID
      const [newRfid] = await db
        .insert(rfids)
        .values({
          tagId: normalizedTagId,
          userId: userId || null,
          isActive,
          registeredBy: user.id,
          metadata,
          unitNumber:
            unitNumber ?? (metadata?.unitNumber as string | undefined) ?? null,
        })
        .returning();

      return c.json(
        {
          success: true,
          message: "RFID tag registered successfully",
          data: {
            rfid: {
              ...newRfid,
              tagId: normalizedTagId,
            },
          },
        },
        201
      );
    } catch (error: any) {
      console.error("Register RFID error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to register RFID",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/rfid/check-recent-scan/:tagId - Verify a tag was scanned recently during registration
app.get(
  "/check-recent-scan/:tagId",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const tagIdParam = c.req.param("tagId");
      const normalizedTagId = normalizeTagId(tagIdParam);

      if (!normalizedTagId) {
        return c.json(
          {
            success: false,
            message: "Invalid RFID tag ID",
          },
          400
        );
      }

      const lookbackWindowMs = 2 * 60 * 1000; // two minutes
      const since = new Date(Date.now() - lookbackWindowMs);

      const [recentScan] = await db
        .select({
          id: rfidScans.id,
          deviceId: rfidScans.deviceId,
          status: rfidScans.status,
          scanTime: rfidScans.scanTime,
          metadata: rfidScans.metadata,
        })
        .from(rfidScans)
        .where(
          and(
            eq(rfidScans.status, "failed"),
            sql`${rfidScans.rfidTagId} ILIKE ${normalizedTagId}`,
            sql`${rfidScans.scanTime} >= ${since}`
          )
        )
        .orderBy(desc(rfidScans.scanTime))
        .limit(1);

      if (!recentScan) {
        return c.json({
          success: true,
          data: {
            found: false,
            scan: null,
          },
        });
      }

      return c.json({
        success: true,
        data: {
          found: true,
          scan: {
            ...recentScan,
            tagId: normalizedTagId,
          },
        },
      });
    } catch (error: any) {
      console.error("Check recent scan error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to verify recent scan",
          error: error.message,
        },
        500
      );
    }
  }
);

// PUT /api/rfid/:tagId - Update RFID tag (admin/superadmin only)
app.put(
  "/:tagId",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const tagIdParam = c.req.param("tagId");
      const normalizedTagId = normalizeTagId(tagIdParam);
      const body = await c.req.json();
      const { userId, isActive, metadata, unitNumber } = body as {
        userId?: number | null;
        isActive?: boolean;
        metadata?: Record<string, any>;
        unitNumber?: string | null;
      };
      const unitNumberFromBody = unitNumber as string | undefined;
      const unitNumberFromMetadata =
        (metadata?.unitNumber as string | undefined) ?? undefined;

      if (!normalizedTagId) {
        return c.json(
          {
            success: false,
            message: "Invalid RFID tag ID",
          },
          400
        );
      }

      // Check if RFID exists
      const [existingRfid] = await db
        .select()
        .from(rfids)
        .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`)
        .limit(1);

      if (!existingRfid) {
        return c.json(
          {
            success: false,
            message: "RFID tag not found",
          },
          404
        );
      }

      // If userId is being updated, verify user exists
      if (userId !== undefined) {
        if (userId !== null) {
          const [userExists] = await db
            .select()
            .from(users)
            .where(eq(users.id, userId))
            .limit(1);

          if (!userExists) {
            return c.json(
              {
                success: false,
                message: "User not found",
              },
              404
            );
          }
        }
      }

      // Build update object
      const updateData: any = {
        updatedAt: new Date(),
      };

      if (userId !== undefined) updateData.userId = userId;
      if (isActive !== undefined) updateData.isActive = isActive;
      if (metadata !== undefined) updateData.metadata = metadata;
      if (unitNumberFromBody !== undefined)
        updateData.unitNumber = unitNumberFromBody || null;
      else if (unitNumberFromMetadata !== undefined)
        updateData.unitNumber = unitNumberFromMetadata || null;

      // Update RFID
      const [updatedRfid] = await db
        .update(rfids)
        .set(updateData)
        .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`)
        .returning();

      return c.json({
        success: true,
        message: "RFID tag updated successfully",
        data: {
          rfid: {
            ...updatedRfid,
            tagId: normalizeTagId(updatedRfid.tagId),
          },
        },
      });
    } catch (error: any) {
      console.error("Update RFID error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to update RFID",
          error: error.message,
        },
        500
      );
    }
  }
);

// DELETE /api/rfid/:tagId - Delete RFID tag (superadmin only)
app.delete("/:tagId", authMiddleware, requireRole("superadmin"), async (c) => {
  try {
    const db = c.get("db");
    const tagIdParam = c.req.param("tagId");
    const normalizedTagId = normalizeTagId(tagIdParam);

    if (!normalizedTagId) {
      return c.json(
        {
          success: false,
          message: "Invalid RFID tag ID",
        },
        400
      );
    }

    // Check if RFID exists
    const [existingRfid] = await db
      .select()
      .from(rfids)
      .where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`)
      .limit(1);

    if (!existingRfid) {
      return c.json(
        {
          success: false,
          message: "RFID tag not found",
        },
        404
      );
    }

    // Delete RFID (scans will remain for audit trail)
    await db.delete(rfids).where(sql`${rfids.tagId} ILIKE ${normalizedTagId}`);

    return c.json({
      success: true,
      message: "RFID tag deleted successfully",
      data: {
        deletedTagId: normalizedTagId,
      },
    });
  } catch (error: any) {
    console.error("Delete RFID error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to delete RFID",
        error: error.message,
      },
      500
    );
  }
});

// GET /api/rfid/unregistered/recent - Get recent unregistered scans (admin/superadmin only)
app.get(
  "/unregistered/recent",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const limit = parseInt(c.req.query("limit") || "50");
      const { unregisteredTags, total, scansChecked } =
        await fetchUnregisteredTags(db, limit);

      return c.json({
        success: true,
        message: `Retrieved ${total} unregistered RFID tags`,
        data: {
          unregisteredTags,
          total,
          scansChecked,
        },
      });
    } catch (error: any) {
      console.error("Get unregistered scans error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to retrieve unregistered scans",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/rfid/scans/unregistered - Legacy alias for unregistered scans list
app.get(
  "/scans/unregistered",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const limit = parseInt(c.req.query("limit") || "50");

      const { unregisteredTags, total, scansChecked } =
        await fetchUnregisteredTags(db, limit);

      return c.json({
        success: true,
        message: `Retrieved ${total} unregistered RFID tags`,
        data: {
          unregisteredTags,
          total,
          scansChecked,
        },
      });
    } catch (error: any) {
      console.error("Get unregistered scans error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to retrieve unregistered scans",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/rfid/scans/recent - Retrieve recent RFID scans
app.get(
  "/scans/recent",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const limit = Math.min(
        Math.max(parseInt(c.req.query("limit") || "25", 10), 1),
        200
      );

      const scans = await db
        .select({
          id: rfidScans.id,
          rfidTagId: rfidScans.rfidTagId,
          deviceId: rfidScans.deviceId,
          userId: rfidScans.userId,
          location: rfidScans.location,
          vehicleId: rfidScans.vehicleId,
          scanTime: rfidScans.scanTime,
          status: rfidScans.status,
          eventType: rfidScans.eventType,
          metadata: rfidScans.metadata,
          userName: users.name,
          userRole: users.role,
          userActive: users.isActive,
        })
        .from(rfidScans)
        .leftJoin(users, eq(rfidScans.userId, users.id))
        .orderBy(desc(rfidScans.scanTime))
        .limit(limit);

      const normalizedScans = scans.map((scan) => ({
        id: scan.id,
        rfidTagId: normalizeTagId(scan.rfidTagId),
        deviceId: scan.deviceId,
        userId: scan.userId,
        location: scan.location,
        vehicleId: scan.vehicleId,
        scanTime: scan.scanTime,
        status: scan.status,
        eventType: scan.eventType,
        metadata: scan.metadata ?? {},
        user: scan.userId
          ? {
              id: scan.userId,
              name: scan.userName,
              role: scan.userRole,
              isActive: scan.userActive,
            }
          : null,
      }));

      return c.json({
        success: true,
        message: `Retrieved ${normalizedScans.length} RFID scans`,
        data: {
          scans: normalizedScans,
        },
      });
    } catch (error: any) {
      console.error("Get recent RFID scans error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to retrieve recent scans",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/rfid - List all RFIDs (with auth)
// GET /api/rfid/:tagId - Get specific RFID
// POST /api/rfid/register - Register new RFID
// PUT /api/rfid/:tagId - Update RFID
// DELETE /api/rfid/:tagId - Delete RFID
// ... (add more endpoints as needed)

export default app;
