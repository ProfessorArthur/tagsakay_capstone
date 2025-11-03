# 🚀 Quick Start Guide - Cloudflare Workers Setup

Follow these steps to get your TagSakay Workers backend running.

## Prerequisites

- Node.js 18+ installed
- A Neon database account (https://neon.tech - free)
- Cloudflare account (for deployment only)

## Step 1: Install Dependencies (2 minutes)

```bash
cd backend-workers
npm install
```

## Step 2: Setup Environment Variables (3 minutes)

### Create your local environment file:

```bash
cp .dev.vars.example .dev.vars
```

### Get your Neon connection string:

1. Go to https://console.neon.tech
2. Create a new project (if you don't have one)
3. Click "Connection Details"
4. Copy the connection string
5. It looks like: `postgresql://user:pass@ep-xxx.region.aws.neon.tech/db?sslmode=require`

### Edit `.dev.vars`:

```bash
# backend-workers/.dev.vars
DATABASE_URL=postgresql://YOUR_NEON_CONNECTION_STRING_HERE
JWT_SECRET=generate-a-secure-random-string-here-at-least-32-characters
```

**Generate JWT Secret:**

```bash
# On Mac/Linux:
openssl rand -base64 32

# On Windows (PowerShell):
[Convert]::ToBase64String((1..32 | ForEach-Object { Get-Random -Minimum 0 -Maximum 256 }))
```

## Step 3: Setup Database (5 minutes)

### Run migrations and seed data:

```bash
npm run db:setup
```

This will:

1. Create all database tables
2. Seed test data (users, RFIDs, devices)

### Expected output:

```
🔄 Starting database migration...
✅ Migration complete!
🌱 Seeding database...
✅ SuperAdmin: admin@tagsakay.com
✅ Driver: driver@test.com
✅ RFID: TEST001
✅ Device: Main Gate Scanner
🎉 Database seeded successfully!
```

## Step 4: Start Development Server (1 minute)

```bash
npm run dev
```

Server will start at: http://localhost:8787

## Step 5: Test Your API (2 minutes)

### Test health check:

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

### Test login:

```bash
curl -X POST http://localhost:8787/api/auth/login \
  -H "Content-Type: application/json" \
  -d "{\"email\":\"admin@tagsakay.com\",\"password\":\"admin123\"}"
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

### Test RFID scan (device authentication):

```bash
curl -X POST http://localhost:8787/api/rfid/scan \
  -H "X-API-Key: test_device_key_main_gate" \
  -H "Content-Type: application/json" \
  -d "{\"tagId\":\"TEST001\",\"location\":\"Main Gate\"}"
```

**Expected response:**

```json
{
  "success": true,
  "message": "Scan recorded successfully",
  "data": {
    "scan": { "id": "...", "rfidTagId": "TEST001", "status": "success" },
    "isRegistered": true,
    "user": { "id": 1, "name": "Juan Dela Cruz" }
  }
}
```

## Step 6: Explore the API (Optional)

### View all available endpoints:

Open http://localhost:8787 in your browser to see the API info.

### Use Drizzle Studio (Database GUI):

```bash
npm run studio
```

Opens a web interface at http://localhost:4983 to view and edit your database.

## 🎉 Success!

Your Workers backend is now running locally!

## Next Steps

### For Development:

1. **Read the conversion guide**: See `CONVERSION_EXAMPLE.md` for how to convert Express routes to Workers
2. **Implement remaining routes**: Check `PROGRESS.md` for what's left to do
3. **Test with your frontend**: Update frontend to point to `http://localhost:8787`
4. **Test with ESP32**: Update ESP32 firmware to use `http://localhost:8787` (or use ngrok for testing)

### For Production Deployment:

1. **Login to Cloudflare:**

   ```bash
   wrangler login
   ```

2. **Set production secrets:**

   ```bash
   wrangler secret put DATABASE_URL
   # Paste your Neon connection string

   wrangler secret put JWT_SECRET
   # Paste your JWT secret
   ```

3. **Deploy:**

   ```bash
   npm run deploy
   ```

4. **Your API is live!**
   - URL: `https://tagsakay-api.YOUR-SUBDOMAIN.workers.dev`
   - Setup custom domain in Cloudflare dashboard

## 🆘 Troubleshooting

### "Cannot find module 'dotenv'"

```bash
npm install
```

### "DATABASE_URL not found"

- Make sure `.dev.vars` exists (not `.dev.vars.example`)
- Check that DATABASE_URL is set correctly
- No quotes around the connection string

### "Migration failed"

- Check your Neon connection string is correct
- Make sure you can access Neon from your location
- Try running `npm run generate` first

### "Port 8787 already in use"

- Another Wrangler instance is running
- Kill it: `npx wrangler dev --port 8788` (use different port)

### "WebSocket connection failed"

- Normal in local dev, WebSocket is only used for Neon in production
- The `ws` package handles this for local development

## 📚 Resources

- **Main Guide**: See `CLOUDFLARE_REWRITE_GUIDE.md` for complete migration strategy
- **Progress Tracker**: Check `PROGRESS.md` for implementation status
- **Conversion Example**: Read `CONVERSION_EXAMPLE.md` for code patterns
- **Reference Repo**: https://github.com/neondatabase-labs/cloudflare-drizzle-neon

## Test Credentials (from seed data)

### Admin Login:

- Email: `admin@tagsakay.com`
- Password: `admin123`
- Role: SuperAdmin

### Driver Login:

- Email: `driver@test.com`
- Password: `driver123`
- Role: Driver
- RFID: TEST001

### Device API Keys:

- Main Gate: `test_device_key_main_gate`
- Exit Gate: `test_device_key_exit_gate`

### Test RFID Tags:

- `TEST001` - Active, assigned to driver
- `TEST002` - Active, unassigned
- `TEST999` - Inactive
- `UNKNOWN` - Not registered (will create unregistered scan)

---

**Ready to build!** 🚀
