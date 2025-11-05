# TagSakay RFID System - AI Coding Agent Instructions

## System Architecture Overview

TagSakay is an RFID tricycle queue management system with three core components:

- **Backend**: Cloudflare Workers + Neon PostgreSQL with OWASP security (`backend-workers/`)
- **Frontend**: Vue.js + TypeScript admin interface (`frontend/`)
- **ESP32**: RFID scanner firmware (`esp32.ino`) for physical devices

### Key Data Flow Patterns

1. **RFID Scanning**: ESP32 → Backend API → Database → Frontend Live Updates
2. **Device Management**: Frontend → Backend → Device Registration/Status
3. **User Authentication**: JWT-based with role-based access (SuperAdmin, Admin, Driver)
4. **Security**: OWASP-compliant rate limiting, password hashing, input validation

## Critical Development Workflows

### Backend (Cloudflare - OWASP Compliant)

```bash
# Navigate to backend-workers
cd backend-workers

# Development server
npm run dev              # Start Wrangler dev server on port 8787

# Database operations (Drizzle ORM)
npm run db:generate      # Generate migrations from schema
npm run db:migrate       # Run migrations against Neon database
npm run db:push          # Push schema changes directly (development only)
npm run db:studio        # Open Drizzle Studio for database inspection

# Testing
node tests/rate-limit-test.js        # Test authentication rate limiting
node tests/password-strength-test.js # Test password validation

# Deployment
npm run deploy           # Deploy to Cloudflare Workers
```

### Device Management

```bash
npm run device:register 00:11:22:33:44:55 "Gate Name" "Location"
npm run device:list    # View all registered devices
```

### API Testing

```bash
npm run test:api                                    # Show available endpoints
npm run test:api login                             # Test authentication
npm run test:api scanRfid '{"tagId":"ABC123","deviceId":"001122334455"}'
```

## Project-Specific Conventions

### Backend (Cloudflare) Conventions

**OWASP Security Implementation:**

- **Password Hashing**: PBKDF2-SHA256 with 600,000 iterations (Web Crypto API)
- **JWT Security**: 4-hour expiration, full claims (iss, aud, sub, iat, exp, nbf, jti)
- **Rate Limiting**: Tiered (auth 5/min, API 100/min, device 3/hr), exponential backoff
- **Account Lockout**: 5 failed attempts → 15-minute lock, account-based tracking
- **Input Validation**: RFC 5321 email, RFID 4-32 alphanumeric, MAC address format
- **Security Headers**: 10 OWASP-recommended headers (CSP, HSTS, X-Frame-Options, etc.)
- **Security Logging**: 15 event types, 4 severity levels, structured JSON logs

**Drizzle ORM Patterns:**

```typescript
// Schema definition in src/db/schema.ts
export const users = pgTable("users", {
  id: serial("id").primaryKey(),
  email: varchar("email", { length: 255 }).notNull().unique(),
  password: text("password").notNull(),
  role: varchar("role", { length: 20 }).notNull().default("driver"),
  isActive: boolean("is_active").notNull().default(true),
  createdAt: timestamp("created_at").notNull().defaultNow(),
});

// Query patterns
const [user] = await db
  .select()
  .from(users)
  .where(eq(users.email, email))
  .limit(1);
await db.insert(users).values({ name, email, password }).returning();
await db.update(users).set({ isActive: false }).where(eq(users.id, userId));
```

**Hono Framework Patterns:**

```typescript
// Route definition
app.post("/api/auth/login", authRateLimit, async (c) => {
  const db = c.get("db");
  const body = await c.req.json();
  // ... validation and logic
  return c.json({ success: true, data: { token } }, 201);
});

// Middleware usage
app.use("/api/users/*", authMiddleware, requireRole("admin"));
```

**Environment Configuration:**

- Uses `.dev.vars` for local development (NOT committed to git)
- Uses `.env.example` as template (safe to commit)
- Uses `wrangler.toml` for Cloudflare configuration
- Database: Neon PostgreSQL (serverless, connection pooling)

### Environment Configuration

- **Backend**: Uses `.dev.vars` (secrets) + `wrangler.toml` (config)
- **Frontend**: Uses `VITE_API_URL` environment variable (port 8787)
- **ESP32**: Hardcoded config in firmware - modify before flashing

### API Response Format (Enforce Consistency)

The backend uses the following response format:

```javascript
// Success response
res.json({
  success: true,
  message: "Operation completed",
  data: {
    /* actual data */
  },
});

// Error response (with validation errors)
res.status(400).json({
  success: false,
  message: "Validation failed",
  errors: ["Error 1", "Error 2"], // Optional array
});

// Rate limiting response (429)
res.status(429).json({
  success: false,
  message: "Too many requests",
  retryAfter: "60 seconds",
});
```

## Integration Points & External Dependencies

### Database Integration

- **Backend**: Neon PostgreSQL (serverless) with Drizzle ORM
  - Connection pooling via `@neondatabase/serverless`
  - Schema-first approach with TypeScript
  - Migration files in `src/db/migrations/`

### Frontend-Backend Communication

- **API Client**: `frontend/src/services/api.ts` with automatic JWT injection
- **Base URL**: `http://localhost:8787`
- **Error Handling**:
  - 401 responses trigger automatic logout/redirect
  - 429 responses show rate limiting UI with countdown timer
  - 400 responses display validation errors
- **Type Safety**: TypeScript `ApiResponse<T>` interface for all API calls
- **Real-time**: Currently polling-based, WebSocket integration planned

### ESP32 Integration

- **Authentication**: Uses API keys with device-specific prefixes
- **Scanning Modes**: Normal scanning vs registration mode for new tags
- **Heartbeat**: Devices must ping `/api/devices/:deviceId/heartbeat` regularly

## Common Development Patterns

### Backend Development

1. Update schema in `backend-workers/src/db/schema.ts`
2. Generate migration: `npm run db:generate`
3. Apply migration: `npm run db:migrate`
4. Create/update routes in `backend-workers/src/routes/`
5. Test with `npm run dev`

### Frontend Service Integration

1. Add service in `src/services/`
2. Import in components/views
3. Handle loading states and errors consistently
4. Update TypeScript interfaces

## Key Files for Context

- `backend-workers/src/index.ts` - Main Hono application entry point
- `backend-workers/src/db/schema.ts` - Drizzle database schema
- `backend-workers/src/middleware/rateLimit.ts` - Rate limiting and account lockout
- `backend-workers/src/lib/auth.ts` - Password hashing and JWT generation
- `backend-workers/tests/TEST_RESULTS.md` - Integration test results and security audit
- `frontend/src/services/api.ts` - API client configuration
- `frontend/src/views/Login.vue` - Login with rate limiting UI
- `frontend/src/views/Register.vue` - Registration with password strength indicator
- `TagSakay_Fixed_Complete/TagSakay_Fixed_Complete.ino` - Hardware integration reference

## Authentication & Security

### Role Hierarchy

- **SuperAdmin**: Full system access
- **Admin**: User/device/RFID management
- **Driver**: Limited dashboard access

### API Security (OWASP Compliant)

- **JWT tokens**: 4-hour expiration with full claim set (iss, aud, sub, iat, exp, nbf, jti)
- **Password hashing**: PBKDF2-SHA256, 600,000 iterations, 16-byte salt
- **Rate limiting**: 5 req/min (auth), 100 req/min (API), 3 req/hr (device registration)
- **Account lockout**: 5 failed attempts → 15-minute lock
- **Input validation**: RFC 5321 email, RFID 4-32 alphanumeric, MAC address format
- **Security headers**: CSP, HSTS, X-Frame-Options, X-Content-Type-Options, etc.
- **Security logging**: 15 event types, 4 severity levels (LOW, MEDIUM, HIGH, CRITICAL)
- **API keys**: SHA256 hashing for device authentication

### Frontend Security Features

- **Rate Limiting UI**: Countdown timer, disabled buttons during lockout
- **Password Strength Indicator**: Real-time validation with 5 levels (0-4), color-coded
- **Password Match Validation**: Visual feedback for confirm password field
- **Error Handling**: User-friendly messages for validation, rate limiting, account lockout
- **Client-Side Validation**: Matches backend OWASP rules exactly

Always verify user permissions before database operations and maintain separation between user roles in controller logic.

### Note

If a markdown is created, always put it inside the markdowns folder. Otherwise, stop creating markdowns when changing minute functions or small code snippets.
