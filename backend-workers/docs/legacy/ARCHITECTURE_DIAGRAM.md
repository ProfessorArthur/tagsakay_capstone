# 🏗️ TagSakay Architecture Diagram

> **NOTE:** This document contains the legacy Express.js architecture for historical reference. The Express backend has been removed. The project now exclusively uses the Cloudflare Workers architecture shown below.

---

## Current Architecture (Cloudflare Workers)

---

## Legacy Architecture (Express Backend - REMOVED)

> **This section is for historical reference only. The Express backend was removed in January 2024.**

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 RFID Devices                        │
│  (TagSakay_Fixed_Complete.ino + TagSakay_LED_Matrix)        │
└────────────┬────────────────────────────────────────────────┘
             │ HTTP + API Key Auth
             ▼
┌─────────────────────────────────────────────────────────────┐
│                  Express.js Backend                          │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ src/                                                 │    │
│  │  ├── controllers/  (Business logic)                 │    │
│  │  ├── models/       (Sequelize ORM)                  │    │
│  │  ├── routes/       (Express routes)                 │    │
│  │  ├── middleware/   (Auth, device auth)              │    │
│  │  └── app.js        (Express app)                    │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  Dependencies:                                               │
│  - Express.js                                                │
│  - Sequelize ORM                                             │
│  - PostgreSQL                                                │
│  - JWT (jsonwebtoken)                                        │
│  - bcryptjs                                                  │
└────────────┬────────────────────────────────────────────────┘
             │
             ▼
┌─────────────────────────────────────────────────────────────┐
│              PostgreSQL Database (Local)                     │
│  Tables: Users, Rfids, RfidScans, Devices, ApiKeys         │
└─────────────────────────────────────────────────────────────┘
             ▲
             │
┌────────────┴────────────────────────────────────────────────┐
│                  Vue.js Frontend                             │
│  (Vite + TypeScript + Tailwind CSS)                         │
└─────────────────────────────────────────────────────────────┘
```

**Issues:**

- ❌ Hosting costs $5-10/month
- ❌ Server maintenance required
- ❌ Single geographic location
- ❌ Manual scaling needed
- ❌ **Status: Backend removed in January 2024**

---

## New Architecture (Cloudflare Workers) - CURRENT

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32 RFID Devices                        │
│  (TagSakay_Fixed_Complete.ino + TagSakay_LED_Matrix)        │
└────────────┬────────────────────────────────────────────────┘
             │ HTTPS + API Key Auth
             │ (api.tagsakay.com)
             ▼
┌─────────────────────────────────────────────────────────────┐
│           Cloudflare Edge Network (Global CDN)               │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Cloudflare Workers (Serverless Functions)           │  │
│  │  ┌─────────────────────────────────────────────┐     │  │
│  │  │ src/                                        │     │  │
│  │  │  ├── index.ts      (Hono app)               │     │  │
│  │  │  ├── routes/       (Hono routes)            │     │  │
│  │  │  ├── middleware/   (Auth, CORS, logging)    │     │  │
│  │  │  ├── lib/          (Auth utilities)         │     │  │
│  │  │  └── db/           (Drizzle schema)         │     │  │
│  │  └─────────────────────────────────────────────┘     │  │
│  │                                                       │  │
│  │  Technologies:                                        │  │
│  │  - Hono (Web framework)                              │  │
│  │  - Drizzle ORM                                       │  │
│  │  - Jose (JWT)                                        │  │
│  │  - Web Crypto API (hashing)                          │  │
│  └───────────────────────────────────────────────────────┘  │
└────────────┬────────────────────────────────────────────────┘
             │ WebSocket over HTTP
             │ (@neondatabase/serverless)
             ▼
┌─────────────────────────────────────────────────────────────┐
│         Neon PostgreSQL (Serverless Database)                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Same Schema:                                          │  │
│  │  - Users, Rfids, RfidScans, Devices, ApiKeys        │  │
│  │                                                       │  │
│  │ Features:                                             │  │
│  │  - Auto-scaling                                       │  │
│  │  - Auto-suspend (cost saving)                        │  │
│  │  - Branch databases (dev/staging/prod)               │  │
│  │  - Connection pooling                                │  │
│  └───────────────────────────────────────────────────────┘  │
└────────────┬────────────────────────────────────────────────┘
             ▲
             │ HTTPS API calls
             │
┌────────────┴────────────────────────────────────────────────┐
│              Vue.js Frontend (Cloudflare Pages)              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │ Same frontend code, just different API URL:          │  │
│  │ VITE_API_URL=https://api.tagsakay.com                │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

**Benefits:**

- ✅ **$0/month** on free tiers
- ✅ **Global edge network** (faster worldwide)
- ✅ **Auto-scaling** (handles traffic spikes)
- ✅ **No server maintenance** (fully managed)
- ✅ **Built-in DDoS protection**
- ✅ **Auto-SSL** (HTTPS everywhere)

---

## Data Flow Comparison

### Current Flow (Express)

```
1. ESP32 scans RFID tag
   ↓
2. HTTP POST to backend.example.com/api/rfid/scan
   ↓
3. Express route handler
   ↓
4. Sequelize query to PostgreSQL
   ↓
5. Response back to ESP32
   ↓
6. ESP32 displays result on LED matrix
```

**Latency:** ~200-500ms (depends on server location)

### New Flow (Workers)

```
1. ESP32 scans RFID tag
   ↓
2. HTTPS POST to api.tagsakay.com/api/rfid/scan
   ↓
3. Cloudflare Edge (routes to nearest data center)
   ↓
4. Worker function processes request
   ↓
5. Drizzle query to Neon (via WebSocket)
   ↓
6. Response from edge location (cached if possible)
   ↓
7. ESP32 displays result on LED matrix
```

**Latency:** ~50-150ms (edge computing + connection pooling)

---

## Request Flow Details

### Authentication Flow

```
┌──────────────┐
│ Client/ESP32 │
└──────┬───────┘
       │ 1. Login request
       ▼
┌──────────────────────────────┐
│ Cloudflare Workers           │
│  ┌────────────────────────┐  │
│  │ POST /api/auth/login   │  │
│  └──────────┬─────────────┘  │
│             │ 2. Verify credentials
│             ▼                 │
│  ┌────────────────────────┐  │
│  │ Query user from DB     │  │
│  └──────────┬─────────────┘  │
│             │ 3. Generate JWT
│             ▼                 │
│  ┌────────────────────────┐  │
│  │ Return token           │  │
│  └────────────────────────┘  │
└──────────┬───────────────────┘
           │ 4. JWT token
           ▼
┌──────────────┐
│ Client       │ Stores token for future requests
└──────────────┘
```

### RFID Scan Flow

```
┌──────────────┐
│ ESP32 Device │
└──────┬───────┘
       │ 1. Scan RFID tag "TEST001"
       │ 2. POST /api/rfid/scan
       │    Headers: X-API-Key: device_key_123
       │    Body: { tagId: "TEST001", location: "Main Gate" }
       ▼
┌────────────────────────────────────────┐
│ Cloudflare Workers                     │
│  ┌──────────────────────────────────┐  │
│  │ Device Auth Middleware           │  │
│  │ - Verify API key                 │  │
│  │ - Load device info               │  │
│  └──────────┬───────────────────────┘  │
│             │ 3. Authenticated
│             ▼                           │
│  ┌──────────────────────────────────┐  │
│  │ POST /api/rfid/scan Handler      │  │
│  │ - Check if tag is registered     │  │
│  │ - Get user info if registered    │  │
│  │ - Create scan record             │  │
│  │ - Update device lastSeen         │  │
│  └──────────┬───────────────────────┘  │
│             │ 4. Query & Insert        │
│             ▼                           │
└─────────────┼───────────────────────────┘
              │
              ▼
┌────────────────────────────────────────┐
│ Neon PostgreSQL                        │
│  ┌──────────────────────────────────┐  │
│  │ Tables:                          │  │
│  │ - RfidScans (new record)         │  │
│  │ - Rfids (check registration)     │  │
│  │ - Users (get driver info)        │  │
│  │ - Devices (update lastSeen)      │  │
│  └──────────┬───────────────────────┘  │
│             │ 5. Return data            │
└─────────────┼───────────────────────────┘
              │
              ▼
┌────────────────────────────────────────┐
│ Response to ESP32                      │
│  {                                     │
│    success: true,                      │
│    data: {                             │
│      scan: { id, tagId, status },      │
│      isRegistered: true,               │
│      user: { name: "Juan Dela Cruz" }  │
│    }                                   │
│  }                                     │
└────────────────────────────────────────┘
```

---

## Technology Stack Comparison

### Express Backend (Old)

```
┌─────────────────────────────────┐
│ Runtime: Node.js                │
├─────────────────────────────────┤
│ Framework: Express.js           │
│ ORM: Sequelize                  │
│ Database: PostgreSQL (local)    │
│ Auth: jsonwebtoken + bcryptjs   │
│ Hosting: Railway/Render ($$$)   │
└─────────────────────────────────┘
```

### Workers Backend (New)

```
┌─────────────────────────────────┐
│ Runtime: V8 Isolate (Workers)   │
├─────────────────────────────────┤
│ Framework: Hono                 │
│ ORM: Drizzle                    │
│ Database: Neon (serverless)     │
│ Auth: Jose + Web Crypto API     │
│ Hosting: Cloudflare ($0!)       │
└─────────────────────────────────┘
```

---

## File Structure Mapping

### Express to Workers

```
backend/                          backend-workers/
├── src/                         ├── src/
│   ├── app.js                   │   ├── index.ts ✅ (Hono app)
│   ├── controllers/             │   ├── routes/ ✅ (Route handlers)
│   │   ├── authController.js    │   │   ├── auth.ts
│   │   ├── userController.js    │   │   ├── user.ts
│   │   ├── rfidController.js    │   │   ├── rfid.ts
│   │   └── deviceController.js  │   │   └── device.ts
│   ├── models/                  │   ├── db/
│   │   ├── index.js             │   │   ├── index.ts ✅ (DB connection)
│   │   ├── User.js              │   │   └── schema.ts ✅ (All models)
│   │   ├── Rfid.js              │   │
│   │   └── Device.js            │   │
│   ├── middleware/              │   ├── middleware/
│   │   ├── auth.js              │   │   └── auth.ts ✅
│   │   └── deviceAuth.js        │   │
│   └── routes/                  │   └── lib/
│       └── index.js             │       └── auth.ts ✅ (Utilities)
├── config/                      ├── migrate.ts ✅ (Migrations)
│   └── config.json              ├── seed.ts ✅ (Seeding)
├── migrations/                  ├── drizzle/ (Generated)
├── seeders/                     └── .dev.vars ✅ (Environment)
└── scripts/
    └── db-manager.js
```

**Key Differences:**

- Models consolidated into single `schema.ts` file
- Controllers merged into route handlers
- Migrations handled by Drizzle Kit
- Seeders replaced by TypeScript seed script

---

## Database Schema (Unchanged!)

```
┌─────────────────┐
│     Users       │
├─────────────────┤
│ id (PK)         │
│ name            │
│ email           │
│ password        │
│ role            │◄─────┐
│ isActive        │      │
│ rfidTag         │      │
└────────┬────────┘      │
         │               │
         │ 1:N           │ 1:N (registeredBy)
         │               │
         ▼               │
┌─────────────────┐      │
│     Rfids       │      │
├─────────────────┤      │
│ id (PK)         │      │
│ tagId           │      │
│ userId (FK)     │──────┘
│ isActive        │
│ deviceId        │
│ registeredBy    │◄─────┐
│ metadata        │      │
└────────┬────────┘      │
         │               │
         │ 1:N           │
         │               │
         ▼               │
┌─────────────────┐      │
│   RfidScans     │      │
├─────────────────┤      │
│ id (PK)         │      │
│ rfidTagId (FK)  │──────┘
│ deviceId (FK)   │──────┐
│ userId (FK)     │      │
│ eventType       │      │
│ location        │      │
│ scanTime        │      │
│ status          │      │
│ metadata        │      │
└─────────────────┘      │
                         │
         ┌───────────────┘
         │
         ▼
┌─────────────────┐
│    Devices      │
├─────────────────┤
│ id (PK)         │
│ deviceId        │
│ macAddress      │
│ name            │
│ location        │
│ apiKey          │
│ isActive        │
│ registrationMode│
│ scanMode        │
│ lastSeen        │
└─────────────────┘

┌─────────────────┐
│    ApiKeys      │
├─────────────────┤
│ id (PK)         │
│ name            │
│ deviceId        │
│ key             │
│ prefix          │
│ permissions     │
│ isActive        │
│ createdBy (FK)  │──────┐
│ type            │      │
└─────────────────┘      │
                         │
         ┌───────────────┘
         │
         └──────► Users
```

**Good news:** Schema is identical! Just translated from Sequelize to Drizzle syntax.

---

## Development Workflow Comparison

### Express Workflow

```bash
# Start database
docker-compose up -d postgres

# Reset database
npm run db:reset

# Seed data
npm run db:seed

# Start backend
npm run dev

# Backend runs on http://localhost:3000
```

### Workers Workflow

```bash
# Setup environment
cp .dev.vars.example .dev.vars
# Edit with Neon connection string

# Setup database (migrate + seed)
npm run db:setup

# Start backend
npm run dev

# Backend runs on http://localhost:8787

# Open database GUI (optional)
npm run studio
```

**Simpler:** No Docker, no local Postgres, no manual seeding!

---

## Deployment Workflow Comparison

### Express Deployment

```bash
# Push to GitHub
git push

# Deploy to Railway/Render
# (via web dashboard or CLI)

# Set environment variables
# DATABASE_URL
# JWT_SECRET
# PORT

# Wait for build + deploy (~2-5 minutes)
```

**Monthly cost:** $5-10

### Workers Deployment

```bash
# Login to Cloudflare
wrangler login

# Set secrets
wrangler secret put DATABASE_URL
wrangler secret put JWT_SECRET

# Deploy
npm run deploy

# Live in ~30 seconds!
```

**Monthly cost:** $0 ✅

---

## Summary

### What Changes

- ✅ Hosting platform (Express → Workers)
- ✅ ORM (Sequelize → Drizzle)
- ✅ Framework (Express → Hono)
- ✅ Database location (Local → Neon)

### What Stays the Same

- ✅ Database schema (exact same tables)
- ✅ Business logic (same features)
- ✅ Authentication flow (JWT + API keys)
- ✅ Frontend (no changes needed!)
- ✅ ESP32 (just update API URL)

### The Win

- ✅ $0/month instead of $5-10/month
- ✅ Global edge network (faster)
- ✅ Auto-scaling (no limits)
- ✅ No server maintenance
- ✅ Modern tech stack
- ✅ Better developer experience

---

**Ready to migrate?** Start with [QUICKSTART.md](./backend-workers/QUICKSTART.md)
