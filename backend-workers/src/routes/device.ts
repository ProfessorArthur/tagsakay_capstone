import { Hono } from "hono";
import {
  authMiddleware,
  requireRole,
  deviceAuthMiddleware,
} from "../middleware/auth";
import { apiRateLimit } from "../middleware/rateLimit";
import { devices } from "../db/schema";
import { eq, desc } from "drizzle-orm";
import type { Database } from "../db";
import { generateApiKey, hashApiKey } from "../lib/auth";

type Env = {
  Bindings: {
    DATABASE_URL: string;
    JWT_SECRET: string;
    SESSION_SECRET: string;
  };
  Variables: {
    db: Database;
    user?: any;
    device?: any;
  };
};

const app = new Hono<Env>();

// POST /api/devices/register - Register new device (admin/superadmin only)
app.post(
  "/register",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const user = c.get("user");
      const { macAddress, name, location } = await c.req.json();

      if (!macAddress || !name || !location) {
        return c.json(
          {
            success: false,
            message: "macAddress, name, and location are required",
          },
          400
        );
      }

      // Format deviceId from MAC address (remove colons)
      const deviceId = macAddress.replace(/:/g, "");

      // Check if device already exists
      const [existingDevice] = await db
        .select()
        .from(devices)
        .where(eq(devices.deviceId, deviceId))
        .limit(1);

      if (existingDevice) {
        return c.json(
          {
            success: false,
            message: "Device already registered",
          },
          409
        );
      }

      // Generate API key for device
      const apiKey = generateApiKey();
      const hashedApiKey = await hashApiKey(apiKey);

      // Create new device
      const [newDevice] = await db
        .insert(devices)
        .values({
          deviceId,
          macAddress,
          name,
          location,
          apiKey: hashedApiKey,
          isActive: true,
          registrationMode: false,
          scanMode: false,
          pendingRegistrationTagId: "",
        })
        .returning();

      return c.json(
        {
          success: true,
          message: "Device registered successfully",
          data: {
            device: {
              id: newDevice.id,
              deviceId: newDevice.deviceId,
              macAddress: newDevice.macAddress,
              name: newDevice.name,
              location: newDevice.location,
              isActive: newDevice.isActive,
            },
            apiKey: apiKey, // Return plain API key only once
          },
        },
        201
      );
    } catch (error: any) {
      console.error("Device registration error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to register device",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/devices - List all devices (admin/superadmin only)
app.get("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");

    const allDevices = await db
      .select({
        id: devices.id,
        deviceId: devices.deviceId,
        macAddress: devices.macAddress,
        name: devices.name,
        location: devices.location,
        isActive: devices.isActive,
        registrationMode: devices.registrationMode,
        scanMode: devices.scanMode,
        lastSeen: devices.lastSeen,
        createdAt: devices.createdAt,
      })
      .from(devices)
      .orderBy(desc(devices.lastSeen));

    return c.json({
      success: true,
      message: `Retrieved ${allDevices.length} devices`,
      data: {
        devices: allDevices,
        total: allDevices.length,
      },
    });
  } catch (error: any) {
    console.error("List devices error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to retrieve devices",
        error: error.message,
      },
      500
    );
  }
});

// GET /api/devices/active - List active devices
app.get(
  "/active",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");

      const activeDevices = await db
        .select({
          id: devices.id,
          deviceId: devices.deviceId,
          macAddress: devices.macAddress,
          name: devices.name,
          location: devices.location,
          isActive: devices.isActive,
          registrationMode: devices.registrationMode,
          scanMode: devices.scanMode,
          lastSeen: devices.lastSeen,
          createdAt: devices.createdAt,
        })
        .from(devices)
        .where(eq(devices.isActive, true))
        .orderBy(desc(devices.lastSeen));

      return c.json({
        success: true,
        message: `Retrieved ${activeDevices.length} active devices`,
        data: {
          devices: activeDevices,
          total: activeDevices.length,
        },
      });
    } catch (error: any) {
      console.error("List active devices error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to retrieve active devices",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/devices/:deviceId - Get specific device
app.get("/:deviceId", authMiddleware, async (c) => {
  try {
    const db = c.get("db");
    const deviceId = c.req.param("deviceId");

    const [device] = await db
      .select({
        id: devices.id,
        deviceId: devices.deviceId,
        macAddress: devices.macAddress,
        name: devices.name,
        location: devices.location,
        isActive: devices.isActive,
        registrationMode: devices.registrationMode,
        scanMode: devices.scanMode,
        pendingRegistrationTagId: devices.pendingRegistrationTagId,
        lastSeen: devices.lastSeen,
        createdAt: devices.createdAt,
        updatedAt: devices.updatedAt,
      })
      .from(devices)
      .where(eq(devices.deviceId, deviceId))
      .limit(1);

    if (!device) {
      return c.json(
        {
          success: false,
          message: "Device not found",
        },
        404
      );
    }

    return c.json({
      success: true,
      message: "Device retrieved successfully",
      data: {
        device,
      },
    });
  } catch (error: any) {
    console.error("Get device error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to retrieve device",
        error: error.message,
      },
      500
    );
  }
});

// PUT /api/devices/:deviceId - Update device (admin/superadmin only)
app.put(
  "/:deviceId",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const deviceId = c.req.param("deviceId");
      const { name, location, isActive } = await c.req.json();

      // Check if device exists
      const [existingDevice] = await db
        .select()
        .from(devices)
        .where(eq(devices.deviceId, deviceId))
        .limit(1);

      if (!existingDevice) {
        return c.json(
          {
            success: false,
            message: "Device not found",
          },
          404
        );
      }

      // Build update object
      const updateData: any = {
        updatedAt: new Date(),
      };

      if (name !== undefined) updateData.name = name;
      if (location !== undefined) updateData.location = location;
      if (isActive !== undefined) updateData.isActive = isActive;

      // Update device
      const [updatedDevice] = await db
        .update(devices)
        .set(updateData)
        .where(eq(devices.deviceId, deviceId))
        .returning();

      return c.json({
        success: true,
        message: "Device updated successfully",
        data: {
          device: {
            id: updatedDevice.id,
            deviceId: updatedDevice.deviceId,
            name: updatedDevice.name,
            location: updatedDevice.location,
            isActive: updatedDevice.isActive,
          },
        },
      });
    } catch (error: any) {
      console.error("Update device error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to update device",
          error: error.message,
        },
        500
      );
    }
  }
);

// DELETE /api/devices/:deviceId - Remove a device (admin/superadmin only)
app.delete(
  "/:deviceId",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const deviceId = c.req.param("deviceId");

      const result = await db
        .delete(devices)
        .where(eq(devices.deviceId, deviceId))
        .returning({
          id: devices.id,
          deviceId: devices.deviceId,
          name: devices.name,
        });

      if (result.length === 0) {
        return c.json(
          {
            success: false,
            message: "Device not found",
          },
          404
        );
      }

      return c.json({
        success: true,
        message: "Device removed successfully",
        data: {
          device: result[0],
        },
      });
    } catch (error: any) {
      console.error("Delete device error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to remove device",
          error: error.message,
        },
        500
      );
    }
  }
);

// GET /api/devices/:deviceId/commands - Poll for pending commands (device auth + rate limit)
app.get(
  "/:deviceId/commands",
  apiRateLimit,
  deviceAuthMiddleware,
  async (c) => {
    try {
      const db = c.get("db");
      const deviceId = c.req.param("deviceId");
      const device = c.get("device");

      // Verify deviceId matches authenticated device
      if (device && device.deviceId !== deviceId) {
        return c.json(
          {
            success: false,
            message: "Device ID mismatch",
          },
          403
        );
      }

      // Get device state
      const [deviceData] = await db
        .select({
          deviceId: devices.deviceId,
          isActive: devices.isActive,
          registrationMode: devices.registrationMode,
          scanMode: devices.scanMode,
          pendingRegistrationTagId: devices.pendingRegistrationTagId,
          lastSeen: devices.lastSeen,
        })
        .from(devices)
        .where(eq(devices.deviceId, deviceId))
        .limit(1);

      if (!deviceData) {
        return c.json(
          {
            success: false,
            message: "Device not found",
          },
          404
        );
      }

      // Build commands response
      const commands: any[] = [];

      // Check for registration mode command
      if (deviceData.registrationMode && deviceData.pendingRegistrationTagId) {
        commands.push({
          action: "enable_registration",
          tagId: deviceData.pendingRegistrationTagId,
          timestamp: Date.now(),
        });
      } else if (!deviceData.registrationMode) {
        commands.push({
          action: "disable_registration",
          timestamp: Date.now(),
        });
      }

      // Check for scan mode changes
      commands.push({
        action: "scan_mode",
        enabled: deviceData.scanMode,
        timestamp: Date.now(),
      });

      return c.json({
        success: true,
        message: "Commands retrieved",
        data: {
          commands,
          deviceStatus: {
            isActive: deviceData.isActive,
            registrationMode: deviceData.registrationMode,
            scanMode: deviceData.scanMode,
          },
        },
      });
    } catch (error: any) {
      console.error("Get commands error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to retrieve commands",
          error: error.message,
        },
        500
      );
    }
  }
);

// POST /api/devices/:deviceId/heartbeat - Device heartbeat (device auth)
app.post("/:deviceId/heartbeat", deviceAuthMiddleware, async (c) => {
  try {
    const db = c.get("db");
    const deviceId = c.req.param("deviceId");
    const device = c.get("device");

    // Verify deviceId matches authenticated device
    if (device && device.deviceId !== deviceId) {
      return c.json(
        {
          success: false,
          message: "Device ID mismatch",
        },
        403
      );
    }

    // Update last seen timestamp
    const [updatedDevice] = await db
      .update(devices)
      .set({
        lastSeen: new Date(),
        updatedAt: new Date(),
      })
      .where(eq(devices.deviceId, deviceId))
      .returning();

    if (!updatedDevice) {
      return c.json(
        {
          success: false,
          message: "Device not found",
        },
        404
      );
    }

    return c.json({
      success: true,
      message: "Heartbeat received",
      data: {
        device: {
          deviceId: updatedDevice.deviceId,
          isActive: updatedDevice.isActive,
          registrationMode: updatedDevice.registrationMode,
          scanMode: updatedDevice.scanMode,
          pendingRegistrationTagId: updatedDevice.pendingRegistrationTagId,
          lastSeen: updatedDevice.lastSeen,
        },
      },
    });
  } catch (error: any) {
    console.error("Heartbeat error:", error);
    return c.json(
      {
        success: false,
        message: "Failed to process heartbeat",
        error: error.message,
      },
      500
    );
  }
});

// POST /api/devices/:deviceId/mode - Change device mode (admin/superadmin only)
app.post(
  "/:deviceId/mode",
  authMiddleware,
  requireRole("admin", "superadmin"),
  async (c) => {
    try {
      const db = c.get("db");
      const deviceId = c.req.param("deviceId");
      const { registrationMode, scanMode, pendingRegistrationTagId } =
        await c.req.json();

      // Check if device exists
      const [existingDevice] = await db
        .select()
        .from(devices)
        .where(eq(devices.deviceId, deviceId))
        .limit(1);

      if (!existingDevice) {
        return c.json(
          {
            success: false,
            message: "Device not found",
          },
          404
        );
      }

      // Build update object
      const updateData: any = {
        updatedAt: new Date(),
      };

      if (registrationMode !== undefined)
        updateData.registrationMode = registrationMode;
      if (scanMode !== undefined) updateData.scanMode = scanMode;
      if (pendingRegistrationTagId !== undefined)
        updateData.pendingRegistrationTagId = pendingRegistrationTagId;

      // Update device mode
      const [updatedDevice] = await db
        .update(devices)
        .set(updateData)
        .where(eq(devices.deviceId, deviceId))
        .returning();

      return c.json({
        success: true,
        message: "Device mode updated successfully",
        data: {
          device: {
            deviceId: updatedDevice.deviceId,
            registrationMode: updatedDevice.registrationMode,
            scanMode: updatedDevice.scanMode,
            pendingRegistrationTagId: updatedDevice.pendingRegistrationTagId,
          },
        },
      });
    } catch (error: any) {
      console.error("Update device mode error:", error);
      return c.json(
        {
          success: false,
          message: "Failed to update device mode",
          error: error.message,
        },
        500
      );
    }
  }
);

export default app;
