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
import { eq, desc, and, isNull } from "drizzle-orm";
import type { Database } from "../db";

type Env = {
  Bindings: {
    DATABASE_URL: string;
    JWT_SECRET: string;
  };
  Variables: {
    db: Database;
    device?: Device;
    user?: any;
  };
};

const app = new Hono<Env>();

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

    if (!tagId) {
      return c.json(
        {
          success: false,
          message: "Missing required field: tagId is required",
        },
        400
      );
    }

    console.log(`RFID scan attempt: ${tagId} from device ${device.deviceId}`);

    // Check if RFID exists and is active
    const [rfidTag] = await db
      .select()
      .from(rfids)
      .leftJoin(users, eq(rfids.userId, users.id))
      .where(eq(rfids.tagId, tagId))
      .limit(1);

    if (!rfidTag || !rfidTag.Rfids) {
      console.warn(`Unregistered RFID: ${tagId}`);

      // Record failed scan attempt
      const failedScan: NewRfidScan = {
        rfidTagId: tagId,
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
          data: { tagId, registered: false },
        },
        404
      );
    }

    const { Rfids: rfid, Users: user } = rfidTag;

    // Check if RFID is active
    if (!rfid.isActive) {
      const inactiveScan: NewRfidScan = {
        rfidTagId: tagId,
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
          data: { tagId, active: false },
        },
        403
      );
    }

    // Check if user is active (if associated)
    if (user && !user.isActive) {
      const unauthorizedScan: NewRfidScan = {
        rfidTagId: tagId,
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
          data: { tagId, userName: user.name, userActive: false },
        },
        403
      );
    }

    // Record successful scan
    const successScan: NewRfidScan = {
      rfidTagId: tagId,
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
      .where(eq(rfids.tagId, tagId));

    return c.json({
      success: true,
      message: "Scan recorded successfully",
      data: {
        scan: {
          id: scan.id,
          tagId: scan.rfidTagId,
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
          tagId: rfid.tagId,
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
        isActive: rfids.isActive,
        lastScanned: rfids.lastScanned,
        deviceId: rfids.deviceId,
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
    const tagId = c.req.param("tagId");

    const [rfidData] = await db
      .select({
        id: rfids.id,
        tagId: rfids.tagId,
        isActive: rfids.isActive,
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
      .where(eq(rfids.tagId, tagId))
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
      .where(eq(rfidScans.rfidTagId, tagId))
      .orderBy(desc(rfidScans.scanTime))
      .limit(10);

    return c.json({
      success: true,
      message: "RFID tag retrieved successfully",
      data: {
        rfid: rfidData,
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
      } = await c.req.json();

      if (!tagId) {
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
        .where(eq(rfids.tagId, tagId))
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
          tagId,
          userId: userId || null,
          isActive,
          registeredBy: user.id,
          metadata,
        })
        .returning();

      return c.json(
        {
          success: true,
          message: "RFID tag registered successfully",
          data: {
            rfid: newRfid,
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

// PUT /api/rfid/:tagId - Update RFID tag (admin/superadmin only)
app.put(
  "/:tagId",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const tagId = c.req.param("tagId");
      const { userId, isActive, metadata } = await c.req.json();

      // Check if RFID exists
      const [existingRfid] = await db
        .select()
        .from(rfids)
        .where(eq(rfids.tagId, tagId))
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

      // Update RFID
      const [updatedRfid] = await db
        .update(rfids)
        .set(updateData)
        .where(eq(rfids.tagId, tagId))
        .returning();

      return c.json({
        success: true,
        message: "RFID tag updated successfully",
        data: {
          rfid: updatedRfid,
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
    const tagId = c.req.param("tagId");

    // Check if RFID exists
    const [existingRfid] = await db
      .select()
      .from(rfids)
      .where(eq(rfids.tagId, tagId))
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
    await db.delete(rfids).where(eq(rfids.tagId, tagId));

    return c.json({
      success: true,
      message: "RFID tag deleted successfully",
      data: {
        deletedTagId: tagId,
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

      // Get failed scans (unregistered tags)
      const unregisteredScans = await db
        .select()
        .from(rfidScans)
        .where(and(eq(rfidScans.status, "failed"), isNull(rfidScans.userId)))
        .orderBy(desc(rfidScans.scanTime))
        .limit(limit);

      // Group by tagId and get unique tags with their latest scan
      const uniqueTags = new Map();
      unregisteredScans.forEach((scan) => {
        if (!uniqueTags.has(scan.rfidTagId)) {
          uniqueTags.set(scan.rfidTagId, {
            tagId: scan.rfidTagId,
            lastSeen: scan.scanTime,
            deviceId: scan.deviceId,
            location: scan.location,
            scanCount: 1,
          });
        } else {
          const existing = uniqueTags.get(scan.rfidTagId);
          existing.scanCount += 1;
        }
      });

      const result = Array.from(uniqueTags.values());

      return c.json({
        success: true,
        message: `Retrieved ${result.length} unregistered RFID tags`,
        data: {
          unregisteredTags: result,
          total: result.length,
          scansChecked: unregisteredScans.length,
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

// GET /api/rfid - List all RFIDs (with auth)
// GET /api/rfid/:tagId - Get specific RFID
// POST /api/rfid/register - Register new RFID
// PUT /api/rfid/:tagId - Update RFID
// DELETE /api/rfid/:tagId - Delete RFID
// ... (add more endpoints as needed)

export default app;
