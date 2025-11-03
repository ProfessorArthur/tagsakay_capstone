# ✅ TagSakay Workers - Installation Checklist

Use this checklist to track your setup progress.

---

## 📋 Phase 1: Initial Setup (15 minutes)

### Dependencies Installation

- [ ] Navigate to `backend-workers/` folder
- [ ] Run `npm install`
- [ ] Verify no errors in installation
- [ ] Check `node_modules/` folder exists

**Command:**

```bash
cd backend-workers
npm install
```

**Expected output:**

```
added XXX packages in XXs
```

---

## 📋 Phase 2: Neon Database Setup (10 minutes)

### Create Neon Account

- [ ] Go to https://console.neon.tech
- [ ] Create account (free tier)
- [ ] Create new project
- [ ] Name it "tagsakay-db" or similar

### Get Connection String

- [ ] Click on your project
- [ ] Go to "Connection Details"
- [ ] Copy the connection string
- [ ] Save it somewhere safe

**Format check:** Your connection string should look like:

```
postgresql://user:pass@ep-xxx-xxx.us-east-2.aws.neon.tech/neondb?sslmode=require
```

---

## 📋 Phase 3: Environment Configuration (5 minutes)

### Create Local Environment File

- [ ] Copy `.dev.vars.example` to `.dev.vars`
- [ ] Open `.dev.vars` in editor
- [ ] Paste your Neon connection string as `DATABASE_URL`
- [ ] Generate JWT secret (32+ characters)
- [ ] Save the file

**Commands:**

```bash
# Copy template
cp .dev.vars.example .dev.vars

# Generate JWT secret (PowerShell)
[Convert]::ToBase64String((1..32 | ForEach-Object { Get-Random -Minimum 0 -Maximum 256 }))
```

**Verify `.dev.vars` contains:**

```bash
DATABASE_URL=postgresql://your-actual-neon-string-here
JWT_SECRET=your-generated-secret-here
NODE_ENV=development
```

---

## 📋 Phase 4: Database Initialization (5 minutes)

### Run Migrations

- [ ] Run `npm run migrate`
- [ ] Verify "Migration complete!" message
- [ ] Check for any errors

**Command:**

```bash
npm run migrate
```

**Expected output:**

```
🔄 Starting database migration...
📦 Database: ep-xxx-xxx.us-east-2.aws.neon.tech/neondb
✅ Migration complete!
👍 Database is up to date
```

### Seed Test Data

- [ ] Run `npm run seed`
- [ ] Verify all tables seeded
- [ ] Note the test credentials displayed

**Command:**

```bash
npm run seed
```

**Expected output:**

```
🌱 Seeding database...
🗑️  Clearing existing data...
👤 Creating users...
   ✅ SuperAdmin: admin@tagsakay.com
   ✅ Admin: admin2@tagsakay.com
   ✅ Driver: driver@test.com
   ✅ Inactive Driver: inactive@test.com
🏷️  Creating RFID tags...
   ✅ RFID: TEST001 (Active, assigned to driver)
   ✅ RFID: TEST002 (Active, unassigned)
   ✅ RFID: TEST999 (Inactive)
📱 Creating devices...
   ✅ Device: Main Gate Scanner (001122334455)
   ✅ Device: Exit Gate Scanner (AABBCCDDEEFF)
🔑 Creating API keys...
   ✅ API Key for Main Gate
   ✅ API Key for Exit Gate
🎉 Database seeded successfully!
```

**Save these test credentials:**

- Admin: admin@tagsakay.com / admin123
- Driver: driver@test.com / driver123
- Device Key: test_device_key_main_gate

---

## 📋 Phase 5: Start Development Server (2 minutes)

### Launch Wrangler Dev Server

- [ ] Run `npm run dev`
- [ ] Wait for "Ready on http://localhost:8787"
- [ ] Verify no errors
- [ ] Keep terminal open

**Command:**

```bash
npm run dev
```

**Expected output:**

```
⛅️ wrangler dev

⎔ Starting local server...
[wrangler:inf] Ready on http://localhost:8787
```

---

## 📋 Phase 6: Test Endpoints (10 minutes)

### Test Health Check

- [ ] Open new terminal
- [ ] Run health check curl command
- [ ] Verify JSON response with success: true

**Command:**

```bash
curl http://localhost:8787/health
```

**Expected response:**

```json
{
  "success": true,
  "status": "healthy",
  "timestamp": "2024-11-02T..."
}
```

### Test Login Endpoint

- [ ] Run login curl command
- [ ] Verify you receive a JWT token
- [ ] Copy the token for next test

**Command:**

```bash
curl -X POST http://localhost:8787/api/auth/login -H "Content-Type: application/json" -d "{\"email\":\"admin@tagsakay.com\",\"password\":\"admin123\"}"
```

**Expected response:**

```json
{
  "success": true,
  "message": "Login successful",
  "data": {
    "user": {
      "id": 1,
      "name": "Super Admin",
      "email": "admin@tagsakay.com",
      "role": "superadmin"
    },
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
  }
}
```

### Test RFID Scan

- [ ] Run RFID scan curl command
- [ ] Verify scan is recorded
- [ ] Check response shows registered tag

**Command:**

```bash
curl -X POST http://localhost:8787/api/rfid/scan -H "X-API-Key: test_device_key_main_gate" -H "Content-Type: application/json" -d "{\"tagId\":\"TEST001\",\"location\":\"Main Gate\"}"
```

**Expected response:**

```json
{
  "success": true,
  "message": "Scan recorded successfully",
  "data": {
    "scan": {
      "id": "...",
      "rfidTagId": "TEST001",
      "deviceId": "001122334455",
      "status": "success"
    },
    "isRegistered": true,
    "user": {
      "id": 1,
      "name": "Juan Dela Cruz"
    }
  }
}
```

---

## 📋 Phase 7: Explore Database (Optional)

### Use Drizzle Studio

- [ ] Run `npm run studio`
- [ ] Open browser to http://localhost:4983
- [ ] Browse through tables
- [ ] Verify seeded data is there

**Command:**

```bash
npm run studio
```

---

## 📋 Troubleshooting Checklist

If something doesn't work, check these:

### Issue: npm install fails

- [ ] Check Node.js version (should be 18+)
- [ ] Delete `node_modules/` and `package-lock.json`
- [ ] Run `npm install` again

### Issue: Migration fails

- [ ] Check `.dev.vars` file exists
- [ ] Verify DATABASE_URL is correct
- [ ] Test Neon connection in Neon console
- [ ] Check for typos in connection string

### Issue: Seed fails

- [ ] Run `npm run migrate` first
- [ ] Check DATABASE_URL is correct
- [ ] Make sure you have internet connection

### Issue: Dev server won't start

- [ ] Check if port 8787 is already in use
- [ ] Close any other Wrangler instances
- [ ] Try `npx wrangler dev --port 8788`

### Issue: curl commands return errors

- [ ] Make sure dev server is running
- [ ] Check the URL is correct (localhost:8787)
- [ ] Verify your curl syntax (Windows vs Mac/Linux)
- [ ] Try using Postman or browser instead

### Issue: "Cannot find module 'dotenv'"

- [ ] Run `npm install` in `backend-workers/` folder
- [ ] Check `package.json` has dotenv in devDependencies

---

## 📋 Success Criteria

You've successfully set up the Workers backend when:

- [x] All dependencies installed
- [x] Neon database created and connected
- [x] Environment variables configured
- [x] Database migrations applied
- [x] Test data seeded
- [x] Dev server starts without errors
- [x] Health check returns 200 OK
- [x] Can login and receive JWT token
- [x] Can scan RFID with device auth
- [ ] (Optional) Drizzle Studio works

---

## 🎉 You're Done!

If all checkboxes are checked, congratulations! Your Workers backend is:

✅ **Installed** - All dependencies ready  
✅ **Configured** - Environment set up  
✅ **Database Ready** - Migrations + seed data  
✅ **Running Locally** - Dev server working  
✅ **Tested** - Core endpoints functional

---

## 📚 What's Next?

Now that your backend is running:

1. **Read the docs** → Start with [QUICKSTART.md](./QUICKSTART.md)
2. **Understand architecture** → Read [ARCHITECTURE_DIAGRAM.md](../ARCHITECTURE_DIAGRAM.md)
3. **Learn conversion patterns** → Study [CONVERSION_EXAMPLE.md](./CONVERSION_EXAMPLE.md)
4. **Start implementing** → Use [PROGRESS.md](./PROGRESS.md) to track

---

## 💾 Save This Checklist

Print or save this checklist. You can use it:

- When setting up on a new machine
- When onboarding new developers
- As a reference for deployment
- To troubleshoot issues

---

**Installation completed:** ******\_\_\_******  
**Time taken:** ******\_\_\_******  
**Issues encountered:** ******\_\_\_******

---

**Need help?** Check [IMPLEMENTATION_SUMMARY.md](../IMPLEMENTATION_SUMMARY.md) for support resources.
