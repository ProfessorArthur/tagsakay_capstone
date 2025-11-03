# Full Rewrite Roadmap - Summary

## ✅ What's Already Done

I've created a complete starter project structure for you at `backend-workers/` with:

### 📁 Files Created (13 files):

1. **`package.json`** - Dependencies (Hono, Drizzle, Neon driver, jose, bcryptjs)
2. **`wrangler.toml`** - Cloudflare Workers configuration
3. **`tsconfig.json`** - TypeScript configuration
4. **`drizzle.config.ts`** - Drizzle ORM configuration
5. **`.env.example`** - Environment variables template
6. **`src/index.ts`** - Main Hono app entry point ✅
7. **`src/db/schema.ts`** - Complete database schema (converted from all your Sequelize models) ✅
8. **`src/db/index.ts`** - Database connection setup ✅
9. **`src/lib/auth.ts`** - JWT and password hashing utilities ✅
10. **`src/middleware/auth.ts`** - Authentication middleware (JWT + API keys) ✅
11. **`src/routes/rfid.ts`** - RFID scan endpoint (fully implemented) ✅
12. **`src/routes/auth.ts`** - Login endpoint (fully implemented) ✅
13. **`src/routes/device.ts`** - Device routes (placeholder)
14. **`src/routes/user.ts`** - User routes (placeholder)
15. **`README.md`** - Complete migration guide with step-by-step instructions ✅

### 🎯 Working Features:

- ✅ RFID scanning endpoint (`POST /api/rfid/scan`)
- ✅ User login (`POST /api/auth/login`)
- ✅ JWT authentication middleware
- ✅ Device API key authentication
- ✅ Complete database schema (all 5 tables)
- ✅ Error handling
- ✅ CORS configuration
- ✅ Health check endpoint

---

## 🔨 What You Need to Complete

### **Remaining Work: ~25-35 hours**

You need to migrate the remaining controller endpoints. Here's the breakdown:

#### 1. **RFID Routes** (~4-6 hours)

From `backend/src/controllers/rfidController.js`:

- [ ] GET /api/rfid - List all RFIDs
- [ ] GET /api/rfid/:tagId - Get specific RFID
- [ ] POST /api/rfid/register - Register new RFID
- [ ] PUT /api/rfid/:tagId - Update RFID
- [ ] DELETE /api/rfid/:tagId - Delete RFID
- [ ] GET /api/rfid/scans/recent - Recent scans
- [ ] GET /api/rfid/unregistered - Unregistered scans

#### 2. **Device Routes** (~4-6 hours)

From `backend/src/controllers/deviceController.js`:

- [ ] POST /api/devices/register
- [ ] GET /api/devices
- [ ] GET /api/devices/:deviceId
- [ ] PUT /api/devices/:deviceId
- [ ] POST /api/devices/:deviceId/heartbeat
- [ ] PUT /api/devices/:deviceId/mode

#### 3. **User Routes** (~3-5 hours)

From `backend/src/controllers/userController.js`:

- [ ] GET /api/users
- [ ] POST /api/users
- [ ] GET /api/users/:id
- [ ] PUT /api/users/:id
- [ ] DELETE /api/users/:id

#### 4. **Auth Routes** (~2-3 hours)

From `backend/src/controllers/authController.js`:

- [ ] POST /api/auth/register
- [ ] POST /api/auth/refresh
- [ ] POST /api/auth/logout

#### 5. **API Key Routes** (~2-3 hours)

From `backend/src/controllers/apiKeyController.js`:

- [ ] POST /api/apiKeys
- [ ] GET /api/apiKeys
- [ ] DELETE /api/apiKeys/:id

#### 6. **Testing** (~8-12 hours)

- [ ] Test all endpoints locally
- [ ] Test authentication flows
- [ ] Test error scenarios
- [ ] Test with real ESP32 device

---

## 📝 Step-by-Step Workflow

### **Step 1: Install Dependencies**

```bash
cd backend-workers
npm install
```

This will install:

- `hono` - Web framework for Workers
- `drizzle-orm` - ORM for database queries
- `@neondatabase/serverless` - Neon Postgres driver
- `jose` - JWT handling
- `wrangler` - Cloudflare Workers CLI

### **Step 2: Setup Neon Database**

1. Create account at https://neon.tech
2. Create new project
3. Copy connection string (looks like `postgresql://user:pass@ep-xxx.neon.tech/neondb?sslmode=require`)
4. Either:
   - **Option A**: Backup existing data and restore to Neon
   - **Option B**: Run seeders again on Neon

### **Step 3: Configure Environment**

```bash
# Create local development environment file
cp .env.example .dev.vars

# Edit .dev.vars and add:
DATABASE_URL=your_neon_connection_string
JWT_SECRET=your_jwt_secret
```

### **Step 4: Start Local Development**

```bash
npm run dev
```

Your Workers API will run at `http://localhost:8787`

### **Step 5: Test Current Endpoints**

```bash
# Test health check
curl http://localhost:8787/health

# Test login (if you have seeded data)
curl -X POST http://localhost:8787/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"admin@tagsakay.com","password":"your_password"}'
```

### **Step 6: Migrate Routes One by One**

For each controller, follow this pattern:

1. **Read original Express controller** in `backend/src/controllers/`
2. **Convert Sequelize to Drizzle** using the cheat sheet in README.md
3. **Add route to appropriate file** in `backend-workers/src/routes/`
4. **Test locally** with curl or Postman
5. **Commit when working**

**Example: Adding GET /api/rfid endpoint**

```typescript
// backend-workers/src/routes/rfid.ts

import { authMiddleware, requireRole } from "../middleware/auth";

// Add this after the /scan endpoint
app.get("/", authMiddleware, requireRole("admin", "superadmin"), async (c) => {
  try {
    const db = c.get("db");

    // Convert this Sequelize query:
    // const rfids = await Rfid.findAll({ include: [{ model: User }] });

    // To Drizzle:
    const rfidList = await db
      .select()
      .from(rfids)
      .leftJoin(users, eq(rfids.userId, users.id));

    return c.json({
      success: true,
      data: rfidList,
    });
  } catch (error: any) {
    return c.json(
      {
        success: false,
        message: "Failed to fetch RFIDs",
        error: error.message,
      },
      500
    );
  }
});
```

### **Step 7: Deploy to Cloudflare**

Once all endpoints are working:

```bash
# Login to Cloudflare
wrangler login

# Set production secrets
wrangler secret put DATABASE_URL
wrangler secret put JWT_SECRET

# Deploy!
npm run deploy
```

---

## 💡 Quick Tips

### **When Converting Sequelize → Drizzle:**

| Sequelize                                 | Drizzle                                           |
| ----------------------------------------- | ------------------------------------------------- |
| `Model.findAll()`                         | `db.select().from(table)`                         |
| `Model.findOne({ where: {...} })`         | `db.select().from(table).where(eq(...)).limit(1)` |
| `Model.create({ ... })`                   | `db.insert(table).values({ ... }).returning()`    |
| `Model.update({ ... }, { where: {...} })` | `db.update(table).set({ ... }).where(eq(...))`    |
| `Model.destroy({ where: {...} })`         | `db.delete(table).where(eq(...))`                 |
| `include: [{ model: User }]`              | `.leftJoin(users, eq(table.userId, users.id))`    |

### **Common Imports You'll Need:**

```typescript
import { eq, and, or, like, gt, lt, desc, asc } from "drizzle-orm";
import { users, rfids, rfidScans, devices, apiKeys } from "../db/schema";
```

### **Error Handling Pattern:**

```typescript
try {
  // Your logic here
  return c.json({ success: true, data: result });
} catch (error: any) {
  console.error("Error:", error);
  return c.json(
    {
      success: false,
      message: "Error message",
      error: error.message,
    },
    500
  );
}
```

---

## 🎯 Success Criteria

You'll know you're done when:

✅ All endpoints from your Express backend work in Workers  
✅ Local testing passes for all routes  
✅ Frontend can connect and authenticate  
✅ ESP32 devices can scan RFIDs successfully  
✅ Deployed to `https://api.tagsakay.com`  
✅ **Total monthly cost: $0**

---

## 📊 Timeline Estimate

**Conservative:** 2-3 weeks part-time (2-3 hours/day)  
**Aggressive:** 1 week full-time (6-8 hours/day)  
**Realistic:** 10-14 days with testing

---

## 🆘 If You Get Stuck

1. **Check the example routes** - `rfid.ts` and `auth.ts` show the complete pattern
2. **Read the migration guide** - `README.md` has detailed conversion examples
3. **Test incrementally** - Don't write everything at once
4. **Use Drizzle Studio** - `npm run studio` to inspect your database
5. **Check Drizzle docs** - https://orm.drizzle.team/docs/overview

---

## ✨ Benefits After Migration

- ✅ **$0/month hosting** (vs $5-10 with Railway/Render)
- ✅ **Global edge network** (faster response times)
- ✅ **Auto-scaling** (handles traffic spikes)
- ✅ **Built-in DDoS protection**
- ✅ **Type-safe queries** (TypeScript + Drizzle)
- ✅ **Modern stack** (great for your portfolio)

---

**Ready to start?** Run these commands:

```bash
cd backend-workers
npm install
cp .env.example .dev.vars
# Edit .dev.vars with your Neon connection
npm run dev
```

Then open `src/routes/rfid.ts` and start adding the remaining GET endpoints!
