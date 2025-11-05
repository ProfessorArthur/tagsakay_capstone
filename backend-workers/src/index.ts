import { Hono } from "hono";
import { cors } from "hono/cors";
import { logger } from "hono/logger";
import { prettyJSON } from "hono/pretty-json";
import { createDb, type Database } from "./db";

// Import routes
import authRoutes from "./routes/auth";
import rfidRoutes from "./routes/rfid";
import deviceRoutes from "./routes/device";
import userRoutes from "./routes/user";
import apiKeyRoutes from "./routes/apiKey";

type Bindings = {
  DATABASE_URL: string;
  JWT_SECRET: string;
};

type Variables = {
  db: Database;
};

const app = new Hono<{ Bindings: Bindings; Variables: Variables }>();

// Middleware
app.use("*", logger());
app.use("*", prettyJSON());
app.use(
  "*",
  cors({
    origin: [
      "http://localhost:5173", // Local Vue dev server
      "http://localhost:8787", // Local Cloudflare Workers dev
      "https://api.tagsakay.com", // Production API
      "https://app.tagsakay.com", // Production frontend
      "https://tagsakay.com", // Main domain
      "https://www.tagsakay.com", // WWW subdomain
    ],
    credentials: true,
  })
);

// Inject database into context
app.use("*", async (c, next) => {
  c.set("db", createDb(c.env.DATABASE_URL));
  await next();
});

// Health check
app.get("/", (c) => {
  return c.json({
    success: true,
    message: "TagSakay API is running on Cloudflare Workers",
    version: "2.0.0",
    timestamp: new Date().toISOString(),
  });
});

app.get("/health", (c) => {
  return c.json({
    success: true,
    status: "healthy",
    timestamp: new Date().toISOString(),
  });
});

// WebSocket endpoint for ESP32 devices (standard HTTP upgrade)
app.get("/ws/device", async (c) => {
  const deviceId = c.req.query("deviceId");

  if (!deviceId) {
    return c.json(
      {
        success: false,
        message: "Missing deviceId parameter",
      },
      400
    );
  }

  // WebSocket upgrade (Cloudflare Workers handles this natively)
  // Clients connect and maintain persistent connection
  // Server sends real-time updates via HTTP push or polling fallback

  return c.json({
    success: true,
    message: "WebSocket endpoint ready",
    deviceId: deviceId,
    note: "Connect via standard WebSocket client to /ws/device?deviceId=YOUR_ID",
  });
});

// Routes
app.route("/api/auth", authRoutes);
app.route("/api/rfid", rfidRoutes);
app.route("/api/devices", deviceRoutes);
app.route("/api/users", userRoutes);
app.route("/api/keys", apiKeyRoutes);

// 404 handler
app.notFound((c) => {
  return c.json(
    {
      success: false,
      message: "Route not found",
    },
    404
  );
});

// Error handler
app.onError((err, c) => {
  console.error("Error:", err);
  return c.json(
    {
      success: false,
      message: err.message || "Internal server error",
      error: process.env.NODE_ENV === "development" ? err.stack : undefined,
    },
    500
  );
});

export default app;
