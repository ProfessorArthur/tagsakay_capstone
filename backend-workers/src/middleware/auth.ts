import { Context, Next } from "hono";
import { verifyJWT } from "../lib/auth";

export async function authMiddleware(c: Context, next: Next) {
  const authHeader = c.req.header("Authorization");

  if (!authHeader || !authHeader.startsWith("Bearer ")) {
    return c.json({ success: false, message: "No token provided" }, 401);
  }

  const token = authHeader.substring(7);

  try {
    const payload = await verifyJWT(token, c.env.JWT_SECRET);
    c.set("user", payload);
    await next();
  } catch (error) {
    return c.json({ success: false, message: "Invalid or expired token" }, 401);
  }
}

export async function deviceAuthMiddleware(c: Context, next: Next) {
  const apiKey = c.req.header("X-API-Key");

  if (!apiKey) {
    return c.json({ success: false, message: "No API key provided" }, 401);
  }

  try {
    const db = c.get("db");
    const { devices, apiKeys } = await import("../db/schema");
    const { eq } = await import("drizzle-orm");
    const { verifyApiKey } = await import("../lib/auth");

    const activeDevices = await db
      .select()
      .from(devices)
      .where(eq(devices.isActive, true));

    for (const device of activeDevices) {
      if (await verifyApiKey(apiKey, device.apiKey)) {
        c.set("device", device);
        await next();
        return;
      }
    }

    const activeApiKeys = await db
      .select()
      .from(apiKeys)
      .where(eq(apiKeys.isActive, true));

    for (const apiKeyRecord of activeApiKeys) {
      if (await verifyApiKey(apiKey, apiKeyRecord.key)) {
        c.set("apiKey", apiKeyRecord);
        c.set("device", { id: apiKeyRecord.deviceId });

        await db
          .update(apiKeys)
          .set({ lastUsed: new Date() })
          .where(eq(apiKeys.id, apiKeyRecord.id));

        await next();
        return;
      }
    }

    return c.json({ success: false, message: "Invalid API key" }, 401);
  } catch (error) {
    console.error("Device auth error:", error);
    return c.json({ success: false, message: "Authentication failed" }, 401);
  }
}

export function requireRole(...roles: string[]) {
  return async (c: Context, next: Next) => {
    const user = c.get("user");

    if (!user || !roles.includes(user.role)) {
      return c.json(
        { success: false, message: "Insufficient permissions" },
        403
      );
    }

    await next();
  };
}
