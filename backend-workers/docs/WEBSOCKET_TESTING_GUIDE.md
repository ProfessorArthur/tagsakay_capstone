# WebSocket Testing Guide

## 🧪 Testing WebSocket Implementation

This guide covers how to test the WebSocket functionality for your TagSakay RFID system.

---

## 🛠️ **Prerequisites**

### Install Testing Tools

```bash
# WebSocket command-line client
npm install -g wscat

# Or use websocat (Rust-based, more features)
# Download from: https://github.com/vi/websocat
```

---

## 🚀 **Local Development Testing**

### Step 1: Start Backend Server

```bash
cd backend-workers
npm run dev
```

**Expected output:**

```
⛅️ wrangler 3.x.x
------------------
⎔ Starting local server...
[wrangler:inf] Ready on http://localhost:8787
```

### Step 2: Test WebSocket Connection

```bash
# Connect to WebSocket endpoint
wscat -c "ws://localhost:8787/ws/device?deviceId=TEST001"
```

**Expected response:**

```json
Connected (press CTRL+C to quit)
< {"success":true,"message":"Connected to TagSakay API","deviceId":"TEST001","timestamp":1699000000000}
```

### Step 3: Send Test Scan

```bash
# Type this message:
> {"action":"scan","tagId":"RFID123","location":"Main Gate"}
```

**Expected response:**

```json
< {"success":true,"scan":{"id":"uuid","tagId":"RFID123","timestamp":1699000000,"isRegistered":false,"status":"failed"},"user":null,"device":{"name":null,"location":null}}
```

### Step 4: Send Heartbeat

```bash
> {"action":"heartbeat","timestamp":1699000000}
```

**Expected response:**

```json
< {"success":true,"action":"heartbeat_ack","timestamp":1699000001,"scanCount":1}
```

### Step 5: Update Configuration

```bash
> {"action":"config","registrationMode":true}
```

**Expected response:**

```json
< {"success":true,"message":"Configuration updated","config":{"registrationMode":true}}
```

---

## 🔍 **Testing Scenarios**

### Scenario 1: Duplicate Scan Prevention

**Test:** Send two scans within 1 second

```bash
# First scan
> {"action":"scan","tagId":"TEST123"}
< {"success":true,...}

# Immediate second scan (< 1 second)
> {"action":"scan","tagId":"TEST123"}
< {"success":false,"error":"Duplicate scan - please wait 1 second between scans","timeSinceLastScan":500}
```

✅ **Expected:** Second scan rejected with error message

---

### Scenario 2: Multiple Devices (Different Durable Objects)

**Terminal 1:**

```bash
wscat -c "ws://localhost:8787/ws/device?deviceId=DEVICE001"
> {"action":"scan","tagId":"TAG001"}
```

**Terminal 2:**

```bash
wscat -c "ws://localhost:8787/ws/device?deviceId=DEVICE002"
> {"action":"scan","tagId":"TAG002"}
```

✅ **Expected:** Both devices maintain separate connections and state

---

### Scenario 3: Same Device, Multiple Connections

**Terminal 1:**

```bash
wscat -c "ws://localhost:8787/ws/device?deviceId=DEVICE001"
< {"success":true,"message":"Connected..."}
```

**Terminal 2 (same deviceId):**

```bash
wscat -c "ws://localhost:8787/ws/device?deviceId=DEVICE001"
< HTTP/1.1 409 Conflict
```

✅ **Expected:** Second connection rejected (one connection per device)

---

### Scenario 4: Reconnection & State Persistence

**Test:**

1. Connect and send a scan
2. Disconnect (CTRL+C)
3. Reconnect with same deviceId
4. Check scanCount persisted

```bash
# First connection
wscat -c "ws://localhost:8787/ws/device?deviceId=TEST001"
> {"action":"scan","tagId":"TAG001"}
< {"success":true,...}
> {"action":"heartbeat","timestamp":1699000000}
< {"scanCount":1}
^C  # Disconnect

# Second connection
wscat -c "ws://localhost:8787/ws/device?deviceId=TEST001"
> {"action":"heartbeat","timestamp":1699000000}
< {"scanCount":1}  # ✅ State persisted!
```

---

### Scenario 5: Offline Scan Buffering

**Manual Test:**

1. Modify DeviceConnection to simulate database failure
2. Send scans while "offline"
3. Verify scans are buffered
4. Reconnect and check scans are processed

---

## 📊 **Monitoring & Debugging**

### Check Durable Object State

```bash
# HTTP endpoint to view device state
curl http://localhost:8787/ws/device?deviceId=TEST001
```

**Response:**

```json
{
  "success": true,
  "deviceId": "TEST001",
  "state": {
    "deviceId": "TEST001",
    "lastScanTime": 1699000000,
    "lastHeartbeat": 1699000000,
    "scanCount": 5,
    "offlineScans": [],
    "registrationMode": false,
    "isConnected": true
  }
}
```

---

## 🧪 **Automated Testing Script**

Create `test-websocket.js`:

```javascript
import WebSocket from "ws";

const deviceId = "TEST001";
const ws = new WebSocket(`ws://localhost:8787/ws/device?deviceId=${deviceId}`);

ws.on("open", () => {
  console.log("✅ Connected");

  // Test 1: Send scan
  console.log("\n📡 Test 1: Sending scan...");
  ws.send(
    JSON.stringify({
      action: "scan",
      tagId: "TEST123",
      location: "Main Gate",
    })
  );
});

ws.on("message", (data) => {
  const message = JSON.parse(data);
  console.log("📥 Received:", JSON.stringify(message, null, 2));

  // Test 2: Send heartbeat after first response
  if (message.scan) {
    console.log("\n💓 Test 2: Sending heartbeat...");
    setTimeout(() => {
      ws.send(
        JSON.stringify({
          action: "heartbeat",
          timestamp: Date.now(),
        })
      );
    }, 1000);
  }

  // Test 3: Update config
  if (message.action === "heartbeat_ack") {
    console.log("\n⚙️  Test 3: Updating config...");
    setTimeout(() => {
      ws.send(
        JSON.stringify({
          action: "config",
          registrationMode: true,
        })
      );
    }, 1000);
  }

  // Test 4: Duplicate scan
  if (message.message === "Configuration updated") {
    console.log("\n🔄 Test 4: Testing duplicate scan prevention...");
    ws.send(
      JSON.stringify({
        action: "scan",
        tagId: "DUP123",
      })
    );

    // Immediate duplicate
    setTimeout(() => {
      ws.send(
        JSON.stringify({
          action: "scan",
          tagId: "DUP123",
        })
      );
    }, 500); // Less than 1 second

    // Close after tests
    setTimeout(() => {
      console.log("\n✅ All tests complete");
      ws.close();
    }, 3000);
  }
});

ws.on("error", (error) => {
  console.error("❌ Error:", error.message);
});

ws.on("close", () => {
  console.log("🔌 Disconnected");
  process.exit(0);
});
```

**Run tests:**

```bash
node test-websocket.js
```

---

## 🎯 **Production Testing**

### Step 1: Deploy to Cloudflare

```bash
cd backend-workers
npm run deploy
```

### Step 2: Test Production Endpoint

```bash
# Replace with your Workers URL
wscat -c "wss://tagsakay-api.your-subdomain.workers.dev/ws/device?deviceId=PROD001"
```

**Note:** Use `wss://` (secure WebSocket) for production!

---

## 📈 **Performance Benchmarks**

### Latency Test

```bash
# Create latency-test.js
import WebSocket from 'ws';

const ws = new WebSocket('ws://localhost:8787/ws/device?deviceId=LATENCY');

ws.on('open', () => {
  const start = Date.now();

  ws.send(JSON.stringify({
    action: 'scan',
    tagId: 'LATENCY_TEST'
  }));

  ws.on('message', () => {
    const latency = Date.now() - start;
    console.log(`⚡ Latency: ${latency}ms`);
    ws.close();
  });
});
```

**Expected Results:**

- Local: 10-50ms
- Production (Cloudflare): 50-150ms
- HTTP (comparison): 200-500ms

---

## 🐛 **Common Issues & Solutions**

### Issue: "Connection refused"

**Solution:**

- Check backend is running (`npm run dev`)
- Verify port 8787 is correct
- Check firewall settings

### Issue: "Device already connected (409)"

**Solution:**

- Close existing connection first
- Each device can only have one WebSocket
- Check if another terminal/ESP32 is connected

### Issue: "JSON parse error"

**Solution:**

- Validate JSON syntax
- Use online JSON validator
- Check for trailing commas

### Issue: "Database connection error"

**Solution:**

- Verify `DATABASE_URL` in `.dev.vars`
- Check Neon database is accessible
- Test with HTTP endpoints first

---

## ✅ **Testing Checklist**

Before deploying to ESP32 devices:

- [ ] WebSocket connection establishes successfully
- [ ] Scan messages processed correctly
- [ ] Heartbeat acknowledged
- [ ] Configuration updates work
- [ ] Duplicate scan prevention works
- [ ] Multiple devices can connect simultaneously
- [ ] Single device limited to one connection
- [ ] State persists across reconnections
- [ ] Offline scans buffered (if database fails)
- [ ] HTTP fallback still works
- [ ] Production deployment tested
- [ ] Latency within acceptable range (<150ms)

---

## 📞 **Troubleshooting**

### Enable Debug Logging

In `DeviceConnection.ts`, add console.logs:

```typescript
console.log(`[DEBUG] Message received:`, message);
console.log(`[DEBUG] Current state:`, this.state);
```

### Monitor Durable Object Logs

Check Wrangler output for errors:

```bash
npm run dev
# Watch for error messages in terminal
```

### Test Database Connectivity

```bash
# Test Neon connection
npm run db:studio
# Should open Drizzle Studio
```

---

## 🎉 **Success Criteria**

Your WebSocket implementation is ready when:

1. ✅ All test scenarios pass
2. ✅ Latency < 150ms
3. ✅ No connection errors
4. ✅ State persists correctly
5. ✅ HTTP fallback works
6. ✅ Production deployment successful

**Now you're ready to upgrade your ESP32 devices!** 🚀

---

**Next:** Follow [ESP32_WEBSOCKET_GUIDE.md](./ESP32_WEBSOCKET_GUIDE.md) to update firmware.
