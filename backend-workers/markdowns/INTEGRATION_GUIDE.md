# Security Integration Guide

## Quick Start - Applying Security to Your Backend

This guide shows you how to integrate all the security enhancements into your existing TagSakay backend.

## Step 1: Update index.ts (Main Application)

Replace your current middleware setup with this:

```typescript
import { Hono } from "hono";
import { logger } from "hono/logger";
import { prettyJSON } from "hono/pretty-json";

// Import new security middleware
import { 
  securityHeaders, 
  validateContentType, 
  requestSizeLimit, 
  secureCORS 
} from "./middleware/security";
import { apiRateLimit } from "./middleware/rateLimit";

// Import routes
import authRoutes from "./routes/auth";
import userRoutes from "./routes/user";
import rfidRoutes from "./routes/rfid";
import deviceRoutes from "./routes/device";
import apiKeyRoutes from "./routes/apiKey";

type Env = {
  Bindings: {
    DATABASE_URL: string;
    JWT_SECRET: string;
  };
  Variables: {
    db: Database;
    user?: any;
    device?: any;
    apiKey?: any;
  };
};

const app = new Hono<Env>();

// ============================================================================
// GLOBAL MIDDLEWARE (Applied to ALL routes)
// ============================================================================

// Logging
app.use("*", logger());
app.use("*", prettyJSON());

// Security headers (MUST be first to ensure headers on all responses)
app.use("*", securityHeaders);

// Content-Type validation
app.use("*", validateContentType);

// Request size limits (prevent DoS)
app.use("*", requestSizeLimit);

// CORS with specific allowed origins
app.use("*", secureCORS([
  "http://localhost:5173",    // Development frontend
  "https://tagsakay.com",     // Production frontend
  "https://www.tagsakay.com"  // Production frontend (www)
]));

// Database injection
app.use("*", async (c, next) => {
  const { neon } = await import("@neondatabase/serverless");
  const { drizzle } = await import("drizzle-orm/neon-http");

  const sql = neon(c.env.DATABASE_URL);
  const db = drizzle(sql);

  c.set("db", db);
  await next();
});

// ============================================================================
// RATE LIMITING (Applied to API routes only, not health checks)
// ============================================================================

app.use("/api/*", apiRateLimit);

// ============================================================================
// ROUTES
// ============================================================================

// Health check (no auth, no rate limit)
app.get("/", (c) => {
  return c.json({
    success: true,
    message: "TagSakay RFID API - Secure Edition",
    version: "2.0.0",
    timestamp: new Date().toISOString()
  });
});

app.get("/health", (c) => {
  return c.json({
    status: "healthy",
    timestamp: new Date().toISOString()
  });
});

// API routes
app.route("/api/auth", authRoutes);
app.route("/api/users", userRoutes);
app.route("/api/rfid", rfidRoutes);
app.route("/api/devices", deviceRoutes);
app.route("/api/keys", apiKeyRoutes);

// ============================================================================
// ERROR HANDLING
// ============================================================================

// 404 handler
app.notFound((c) => {
  return c.json({
    success: false,
    message: "Endpoint not found"
  }, 404);
});

// Global error handler
app.onError((err, c) => {
  console.error("Unhandled error:", err);
  
  // Don't leak error details in production
  const isDev = c.env.ENVIRONMENT !== "production";
  
  return c.json({
    success: false,
    message: "An unexpected error occurred",
    ...(isDev && { error: err.message, stack: err.stack })
  }, 500);
});

export default app;
```

## Step 2: Update User Routes (Example)

Here's how to apply security to `routes/user.ts`:

```typescript
import { Hono } from "hono";
import { authMiddleware, requireRole } from "../middleware/auth";
import { validateRequestBody } from "../lib/validation";
import { securityLogger, SecurityEventType, SeverityLevel } from "../lib/securityLogger";
import { hashPassword } from "../lib/auth";
import { users, rfids } from "../db/schema";
import { eq } from "drizzle-orm";
import type { Database } from "../db";

// ... (type definitions)

const app = new Hono<Env>();

// GET /api/users - List all users
app.get("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  const db = c.get("db");
  const currentUser = c.get("user");

  try {
    const allUsers = await db
      .select({
        id: users.id,
        name: users.name,
        email: users.email,
        role: users.role,
        rfidTag: users.rfidTag,
        isActive: users.isActive,
        createdAt: users.createdAt,
        updatedAt: users.updatedAt,
        rfidTags: rfids,
      })
      .from(users)
      .leftJoin(rfids, eq(rfids.assignedUserId, users.id));

    // Group by user
    const userMap = new Map();
    allUsers.forEach((row) => {
      if (!userMap.has(row.id)) {
        userMap.set(row.id, {
          id: row.id,
          name: row.name,
          email: row.email,
          role: row.role,
          rfidTag: row.rfidTag,
          isActive: row.isActive,
          createdAt: row.createdAt,
          updatedAt: row.updatedAt,
          assignedRfids: [],
        });
      }
      if (row.rfidTags) {
        userMap.get(row.id).assignedRfids.push(row.rfidTags);
      }
    });

    return c.json({
      success: true,
      data: Array.from(userMap.values()),
    });
  } catch (error: any) {
    securityLogger.log({
      eventType: SecurityEventType.ERROR,
      severity: SeverityLevel.MEDIUM,
      username: currentUser.email,
      endpoint: "/api/users",
      message: "Failed to list users",
      metadata: { error: error.message }
    });

    return c.json(
      {
        success: false,
        message: "Failed to retrieve users",
      },
      500
    );
  }
});

// POST /api/users - Create new user (with validation)
app.post("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  const db = c.get("db");
  const currentUser = c.get("user");
  const ipAddress = c.req.header("CF-Connecting-IP") || "unknown";
  
  const body = await c.req.json();

  // Validate input
  const validation = validateRequestBody(body, {
    name: { type: "string", required: true, minLength: 2, maxLength: 100 },
    email: { type: "email", required: true },
    password: { type: "string", required: true, minLength: 8, maxLength: 128 },
    role: { 
      type: "enum", 
      required: true, 
      allowedValues: ["superadmin", "admin", "driver"] as const 
    },
    rfidTag: { type: "rfid", required: false }
  });

  if (!validation.valid) {
    securityLogger.log({
      eventType: SecurityEventType.VALIDATION_FAILED,
      severity: SeverityLevel.LOW,
      username: currentUser.email,
      endpoint: "/api/users",
      message: "User creation validation failed",
      metadata: { errors: validation.errors }
    });

    return c.json(
      {
        success: false,
        message: "Validation failed",
        errors: validation.errors,
      },
      400
    );
  }

  const { name, email, password, role, rfidTag } = validation.sanitized!;

  try {
    // Check if email exists
    const [existingUser] = await db
      .select()
      .from(users)
      .where(eq(users.email, email))
      .limit(1);

    if (existingUser) {
      return c.json(
        {
          success: false,
          message: "Email already exists",
        },
        409
      );
    }

    // Check if RFID exists
    if (rfidTag) {
      const [existingRfid] = await db
        .select()
        .from(users)
        .where(eq(users.rfidTag, rfidTag))
        .limit(1);

      if (existingRfid) {
        return c.json(
          {
            success: false,
            message: "RFID tag already assigned",
          },
          409
        );
      }
    }

    // Hash password
    const hashedPassword = await hashPassword(password);

    // Create user
    const [newUser] = await db
      .insert(users)
      .values({
        name,
        email,
        password: hashedPassword,
        role,
        rfidTag: rfidTag || null,
        isActive: true,
      })
      .returning();

    // Log user creation
    securityLogger.log({
      eventType: SecurityEventType.LOGIN_SUCCESS, // You could create USER_CREATED event
      severity: SeverityLevel.LOW,
      username: currentUser.email,
      ipAddress,
      endpoint: "/api/users",
      message: `User ${email} created by ${currentUser.email}`,
      metadata: { createdUserId: newUser.id, role: newUser.role }
    });

    // Remove password from response
    const { password: _, ...userWithoutPassword } = newUser;

    return c.json(
      {
        success: true,
        message: "User created successfully",
        data: userWithoutPassword,
      },
      201
    );
  } catch (error: any) {
    securityLogger.log({
      eventType: SecurityEventType.ERROR,
      severity: SeverityLevel.HIGH,
      username: currentUser.email,
      ipAddress,
      endpoint: "/api/users",
      message: "User creation failed",
      metadata: { error: error.message }
    });

    return c.json(
      {
        success: false,
        message: "Failed to create user",
      },
      500
    );
  }
});

// ... (apply same pattern to PUT and DELETE endpoints)

export default app;
```

## Step 3: Testing Checklist

### A. Test Rate Limiting

```powershell
# Test auth rate limit (should block after 5 requests/minute)
1..10 | ForEach-Object {
  $response = Invoke-RestMethod -Uri "http://localhost:8787/api/auth/login" `
    -Method POST `
    -Body '{"email":"test@example.com","password":"wrong"}' `
    -ContentType "application/json" `
    -ErrorAction SilentlyContinue
  
  Write-Host "Request $_`: $($response.message)"
  Start-Sleep -Milliseconds 500
}
```

### B. Test Account Lockout

```powershell
# Should lock after 5 failed attempts
1..7 | ForEach-Object {
  $response = Invoke-RestMethod -Uri "http://localhost:8787/api/auth/login" `
    -Method POST `
    -Body '{"email":"user@example.com","password":"wrongpassword"}' `
    -ContentType "application/json" `
    -ErrorAction SilentlyContinue
  
  Write-Host "Attempt $_`: $($response.message)"
}
```

### C. Test Input Validation

```powershell
# Invalid email
Invoke-RestMethod -Uri "http://localhost:8787/api/auth/register" `
  -Method POST `
  -Body '{"email":"not-an-email","password":"validpassword123","name":"Test"}' `
  -ContentType "application/json"

# Weak password
Invoke-RestMethod -Uri "http://localhost:8787/api/auth/register" `
  -Method POST `
  -Body '{"email":"test@example.com","password":"short","name":"Test"}' `
  -ContentType "application/json"

# SQL injection attempt (should be sanitized)
Invoke-RestMethod -Uri "http://localhost:8787/api/users" `
  -Method POST `
  -Body '{"email":"admin@test.com","password":"test123456","name":"Robert'\'' DROP TABLE users--"}' `
  -ContentType "application/json" `
  -Headers @{ "Authorization" = "Bearer $token" }
```

### D. Test Security Headers

```powershell
# Check headers
$response = Invoke-WebRequest -Uri "http://localhost:8787/health" -Method GET
$response.Headers | Format-Table
```

Expected headers:
- `Cache-Control: no-store`
- `X-Content-Type-Options: nosniff`
- `X-Frame-Options: DENY`
- `Content-Security-Policy: frame-ancestors 'none'`
- `Referrer-Policy: no-referrer`

### E. Test Password Hashing

```powershell
# Register user with new PBKDF2 hashing
$registerResponse = Invoke-RestMethod -Uri "http://localhost:8787/api/auth/register" `
  -Method POST `
  -Body '{"email":"newuser@example.com","password":"SecurePassword123!","name":"New User"}' `
  -ContentType "application/json"

# Should be able to login immediately
$loginResponse = Invoke-RestMethod -Uri "http://localhost:8787/api/auth/login" `
  -Method POST `
  -Body '{"email":"newuser@example.com","password":"SecurePassword123!"}' `
  -ContentType "application/json"

Write-Host "Token expiration: $($loginResponse.data.expiresIn)"  # Should be "4h"
```

## Step 4: Database Migration (Optional)

If you want to add password version tracking:

```sql
-- Add column for password version
ALTER TABLE users ADD COLUMN password_version INTEGER DEFAULT 1;

-- Add index for better performance
CREATE INDEX idx_users_password_version ON users(password_version);
```

Then update your login logic to detect old passwords:

```typescript
// In auth.ts login endpoint
if (user.password.startsWith('pbkdf2$')) {
  // New format, all good
} else {
  // Old SHA-256 format
  // Verify and prompt for password reset
  if (await verifyPassword(password, user.password)) {
    // Login successful but...
    return c.json({
      success: true,
      message: "Login successful. Please update your password.",
      requirePasswordReset: true,
      data: { token, user }
    });
  }
}
```

## Step 5: Monitor Security Logs

Add an endpoint to view security logs (admin only):

```typescript
// In a new admin routes file
import { securityLogger } from "../lib/securityLogger";

app.get("/security/logs", authMiddleware, requireRole("superadmin"), async (c) => {
  const { limit = 100, type, severity } = c.req.query();
  
  let logs = securityLogger.getRecentLogs(Number(limit));
  
  if (type) {
    logs = securityLogger.getLogsByType(type as any, Number(limit));
  }
  
  if (severity) {
    logs = securityLogger.getLogsBySeverity(severity as any, Number(limit));
  }
  
  return c.json({
    success: true,
    data: logs,
    count: logs.length
  });
});
```

## Step 6: Production Deployment

Before deploying to production:

1. **Environment Variables**:
   ```bash
   wrangler secret put JWT_SECRET
   wrangler secret put DATABASE_URL
   wrangler secret put ENVIRONMENT
   ```

2. **CORS Origins**:
   Update `secureCORS()` to use production domains

3. **Rate Limits**:
   Consider lowering limits for production

4. **Monitoring**:
   Set up Cloudflare Workers Analytics
   Configure error tracking (Sentry, etc.)

5. **Backup**:
   Ensure database backups are configured

---

## Common Issues & Solutions

### Issue: "Too many requests" on development

**Solution**: Rate limits are aggressive. Clear in-memory store or increase limits for development:

```typescript
// In rateLimit.ts for development
const MAX_REQUESTS = process.env.ENVIRONMENT === 'development' ? 1000 : 5;
```

### Issue: CORS errors in browser

**Solution**: Ensure your frontend origin is in the `secureCORS()` allowed list:

```typescript
app.use("*", secureCORS([
  "http://localhost:5173",  // ← Add your frontend dev server
  "https://tagsakay.com"
]));
```

### Issue: Existing users can't log in

**Solution**: Old passwords use SHA-256. The `verifyPassword()` function has backward compatibility built-in, but you may need to prompt users to reset passwords.

---

## Summary

You now have:
- ✅ All security middleware created
- ✅ Input validation library
- ✅ Security logging system
- ✅ Enhanced authentication
- ✅ Rate limiting & account lockout
- ✅ OWASP-compliant headers

**Next**: Apply the same patterns to remaining routes (RFID, Device, API Keys) and integrate into `index.ts`.

**Estimated Time**: 2-3 hours to integrate everything

---

*Need help? Check SECURITY_ENHANCEMENTS.md for detailed information about each security feature.*
