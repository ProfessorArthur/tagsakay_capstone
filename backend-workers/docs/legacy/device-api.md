/\*\*

- API documentation for the Device Management API
- Includes endpoints for device registration, configuration, and status updates
-
- NOTE: This is archived documentation from the legacy Express.js backend.
- The backend-workers implementation may have similar but updated endpoints.
  \*/

# Device Management API Documentation (Legacy)

## Overview

The Device Management API provides endpoints for registering and managing RFID scanning devices in the TagSakay system.
Devices are identified by their MAC addresses and use API keys for authentication.

## Authentication

All device endpoints require authentication except the device heartbeat endpoint.

## Endpoints

### 1. Register Device

**Endpoint:** `POST /api/devices`

**Description:** Registers a new RFID scanning device in the system.

**Request Body:**

```json
{
  "macAddress": "00:11:22:33:44:55",
  "name": "Entrance Gate",
  "location": "Main Building"
}
```

**Response:**

```json
{
  "success": true,
  "message": "Device registered successfully",
  "data": {
    "device": {
      "id": 1,
      "deviceId": "001122334455",
      "macAddress": "00:11:22:33:44:55",
      "name": "Entrance Gate",
      "location": "Main Building",
      "apiKey": "dev_a1b2c3d4...",
      "isActive": true,
      "createdAt": "2023-06-15T08:00:00.000Z",
      "updatedAt": "2023-06-15T08:00:00.000Z"
    }
  }
}
```

### 2. Get All Devices

**Endpoint:** `GET /api/devices`

**Description:** Retrieves a list of all registered devices.

**Response:**

```json
{
  "success": true,
  "data": {
    "devices": [
      {
        "id": 1,
        "deviceId": "001122334455",
        "macAddress": "00:11:22:33:44:55",
        "name": "Entrance Gate",
        "location": "Main Building",
        "isActive": true,
        "lastSeen": "2023-06-15T08:30:00.000Z"
      }
    ]
  }
}
```

### 3. Get Active Devices

**Endpoint:** `GET /api/devices/active`

**Description:** Retrieves a list of currently active devices (devices that have sent a heartbeat within the last 15 minutes).

**Response:**

```json
{
  "success": true,
  "data": {
    "devices": [
      {
        "id": 1,
        "deviceId": "001122334455",
        "name": "Entrance Gate",
        "lastActive": "2023-06-15T08:30:00.000Z",
        "status": "online",
        "location": "Main Building"
      }
    ]
  }
}
```

### 4. Get Registration Mode Status

**Endpoint:** `GET /api/devices/:deviceId/registration-mode`

**Description:** Checks if a device is in registration mode.

**Response:**

```json
{
  "success": true,
  "data": {
    "registrationMode": true,
    "tagId": "04A5B6C7",
    "scanMode": false
  }
}
```

### 5. Set Registration Mode

**Endpoint:** `POST /api/devices/:deviceId/registration-mode`

**Description:** Sets a device to registration mode. In registration mode, the device will associate scanned RFID tags with the specified tag ID or register new tags.

**Request Body:**

```json
{
  "registrationMode": true,
  "pendingRegistrationTagId": "04A5B6C7",
  "scanMode": false
}
```

**Response:**

```json
{
  "success": true,
  "message": "Device status updated",
  "data": {
    "device": {
      "id": 1,
      "deviceId": "001122334455",
      "registrationMode": true,
      "pendingRegistrationTagId": "04A5B6C7",
      "scanMode": false,
      "updatedAt": "2023-06-15T09:00:00.000Z"
    }
  }
}
```

### 6. Device Heartbeat

**Endpoint:** `POST /api/devices/:deviceId/heartbeat`

**Description:** Updates the last seen timestamp for a device and provides device configuration.

**Request Body:**

```json
{
  "macAddress": "00:11:22:33:44:55",
  "statusData": {
    "freeMemory": 43256,
    "uptime": 3600,
    "scanCount": 15
  }
}
```

**Response:**

```json
{
  "success": true,
  "message": "Heartbeat received",
  "data": {
    "config": {
      "registrationMode": false,
      "pendingRegistrationTagId": "",
      "scanMode": false,
      "ledBrightness": 100,
      "scanInterval": 1000
    }
  }
}
```

## Error Responses

**Device Not Found:**

```json
{
  "success": false,
  "message": "Device not found"
}
```

**Invalid Request:**

```json
{
  "success": false,
  "message": "Missing required fields"
}
```

**Server Error:**

```json
{
  "success": false,
  "message": "Error updating device status",
  "error": "Database connection error"
}
```
