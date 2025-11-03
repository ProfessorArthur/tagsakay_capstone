# TagSakay ESP32 Firmware v3.0 - WebSocket Edition

## 🚀 What's New

This firmware now supports **real-time WebSocket connections** for 5-10x faster RFID scan responses!

### Features

- ⚡ **WebSocket Support** - Real-time bidirectional communication (20-100ms latency)
- 🔄 **HTTP Fallback** - Automatically uses HTTP if WebSocket unavailable
- 🛡️ **Duplicate Prevention** - Server-side enforcement via Durable Objects
- 📦 **Offline Buffering** - Scans queued when backend unavailable
- 🎯 **Auto-reconnection** - WebSocket automatically reconnects if disconnected
- 💓 **Heartbeat** - Keeps connection alive, server tracks device status

---

## 📦 Required Libraries

Install via Arduino IDE Library Manager:

1. **WebSockets by Markus Sattler** (v2.4.0+)

   - Sketch → Include Library → Manage Libraries
   - Search: "WebSockets"
   - Install: "WebSockets by Markus Sattler"

2. **ArduinoJson** (v6.x or v7.x)
   - Usually auto-installed with WebSockets
   - If not: Search "ArduinoJson" and install

---

## 🔧 Configuration

### WiFi & Server Settings

Edit `Config.h`:

```cpp
// WebSocket Configuration (Primary)
#define WS_HOST "YOUR_SERVER_IP"       // Your server IP (e.g., "192.168.1.100")
#define WS_PORT 8787                   // Cloudflare Workers port
#define WS_PATH "/ws/device"           // WebSocket endpoint

// HTTP Fallback
#define API_BASE_URL "http://YOUR_SERVER_IP:8787"

// Enable/Disable WebSocket
#define WS_ENABLED true                // Set false to use HTTP only
```

### Device ID

The device ID is automatically set from the ESP32 MAC address. No configuration needed!

---

## 📁 File Structure

```
TagSakay_Fixed_Complete/
├── TagSakay_Fixed_Complete.ino  # Main sketch (updated for WebSocket)
├── Config.h                      # Configuration (WebSocket settings added)
├── WebSocketModule.h             # NEW: WebSocket handler
├── WebSocketModule.cpp           # NEW: WebSocket implementation
├── ApiModule.h/cpp               # HTTP fallback API
├── NetworkModule.h/cpp           # WiFi management
├── RFIDModule.h/cpp              # PN532 RFID reader
├── DisplayModule.h/cpp           # TFT display
├── KeypadModule.h/cpp            # 4x4 keypad
└── UARTModule.h/cpp              # LED matrix communication
```

---

## 🚀 How It Works

### Connection Flow

```
1. ESP32 boots
   ↓
2. Connects to WiFi
   ↓
3. Initializes WebSocket connection
   ↓
4. Connects to ws://server:8787/ws/device?deviceId={MAC}
   ↓
5. Receives welcome message from server
   ↓
6. Ready to scan!
```

### Scan Flow (WebSocket)

```
1. RFID tag detected
   ↓
2. Send to WebSocket: {"action":"scan","tagId":"ABC123"}
   ↓
3. Server processes (20-100ms)
   ↓
4. Receive response: {"success":true,"user":{"name":"Juan"}}
   ↓
5. Display result on screen
```

### Scan Flow (HTTP Fallback)

```
1. WebSocket not connected
   ↓
2. Fall back to HTTP POST /api/rfid/scan
   ↓
3. Same functionality, slower (200-500ms)
```

---

## 🧪 Testing

### Serial Monitor Output (Expected)

```
================================
  TagSakay RFID Scanner v3.0
================================

[1/7] Initializing Display...
[2/7] Initializing UART...
[3/7] Initializing Keypad...
[4/7] Initializing Network...
[NETWORK] Device ID (MAC): 001122334455
[NETWORK] IP: 192.168.1.100  // Your ESP32's IP (example)
[5/7] Initializing RFID...
[RFID] PN532 Firmware 1.6
[6/7] Initializing API...
[7/7] Initializing WebSocket...
[WS] Initializing WebSocket...
[WS] Connecting to: YOUR_SERVER_IP:8787/ws/device?deviceId=001122334455
[WS] Connected
[WS] Message received: {"success":true,"message":"Connected to TagSakay API"}
🔌 WebSocket connected - real-time mode active

[SYSTEM] All modules initialized successfully
[SYSTEM] System ready for operation

[RFID] Scanned: ABC123
[WS] Scan sent: ABC123
[WS] Message received: {"success":true,"scan":{"tagId":"ABC123"},"user":{"name":"Juan Dela Cruz"}}
✅ Registered: Juan Dela Cruz (driver)
```

---

## 🔄 Migration from v2.0

### What Changed

**Added:**

- `WebSocketModule.h` and `.cpp`
- WebSocket callbacks in main sketch
- WS configuration in `Config.h`

**Updated:**

- `TagSakay_Fixed_Complete.ino` - WebSocket integration
- `Config.h` - WebSocket settings
- Initialization sequence (6 steps → 7 steps)

**Backward Compatible:**

- HTTP API still works as fallback
- Same RFID scanning logic
- Same display/keypad functions

### Upgrade Steps

1. **Backup** your current firmware
2. **Install** WebSockets library
3. **Flash** new firmware
4. **Test** on one device first
5. **Monitor** serial output
6. **Verify** WebSocket connection works
7. **Roll out** to other devices

---

## 🐛 Troubleshooting

### WebSocket Connection Failed

**Symptoms:**

```
[WS] Connecting to: ...
[WS] Disconnected
[WS] Attempting to reconnect...
```

**Solutions:**

- Check backend is running: `cd backend-workers && npm run dev`
- Verify server IP/port in `Config.h`
- Check WiFi connection
- Ensure firewall allows port 8787

### Falls Back to HTTP

**Symptoms:**

```
[HTTP] Sending scan via HTTP (WebSocket unavailable)
```

**This is normal if:**

- WebSocket still connecting
- Backend temporarily unavailable
- `WS_ENABLED` is false

**HTTP fallback works perfectly!** No data loss.

### Compile Errors

**"WebSocketsClient.h: No such file"**

- Install WebSockets library (see above)

**"JsonDocument was not declared"**

- Install/update ArduinoJson library

---

## 📊 Performance Comparison

| Feature         | v2.0 (HTTP)   | v3.0 (WebSocket) |
| --------------- | ------------- | ---------------- |
| **Latency**     | 200-500ms     | 20-100ms ⚡      |
| **Connection**  | New each time | Persistent 🔄    |
| **Bandwidth**   | ~1KB/scan     | ~200 bytes 💾    |
| **Server Push** | ❌ No         | ✅ Yes           |
| **Real-time**   | ❌ No         | ✅ Yes           |

---

## 🎯 Configuration Options

### Disable WebSocket (HTTP Only)

In `Config.h`:

```cpp
#define WS_ENABLED false  // Use HTTP only
```

### Adjust Heartbeat Interval

```cpp
#define WS_PING_INTERVAL 30000  // 30 seconds (default)
```

### Adjust Reconnect Interval

```cpp
#define WS_RECONNECT_INTERVAL 5000  // 5 seconds (default)
```

---

## 📞 Support

### Backend Running?

```bash
cd backend-workers
npm run dev
# Should show: Ready on http://127.0.0.1:8787
```

### Test WebSocket Endpoint

```bash
npm install -g wscat
wscat -c "ws://YOUR_SERVER_IP:8787/ws/device?deviceId=TEST001"
# Should connect and show welcome message
```

### Check ESP32 Logs

- Open Serial Monitor (115200 baud)
- Look for "[WS]" messages
- Check connection status

---

## 🎉 Success Criteria

Your ESP32 is working correctly when you see:

```
✅ [WS] Connected
✅ 🔌 WebSocket connected - real-time mode active
✅ [WS] Scan sent: {tagId}
✅ ✅ Registered: {userName}
```

**Scan response in under 100ms!** ⚡

---

## 📝 Version History

- **v3.0.0** - WebSocket support, Durable Objects integration
- **v2.0.0** - Modular architecture, HTTP API
- **v1.0.0** - Initial release

---

**Ready to flash!** 🚀

Connect your ESP32, select the correct board and port, and click Upload!
