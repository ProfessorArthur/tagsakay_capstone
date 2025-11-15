Date: 2025-11-06
Error Encountered: Drizzle migration to the production Neon database failed with `error: type "event_type" already exists`, blocking `npm run db:migrate` and leaving new columns unapplied.
Date of Fix: 2025-11-06
Details: Discovered the target database was missing the `drizzle_migrations` tracking tables, so Drizzle kept replaying the initial enum creation. Added instructions to create `drizzle_migrations` / `drizzle_migrations_lock`, outlined manual column ALTERs (using `text` for hashed codes), and documented the need to seed baseline hashes before future runs.

---

Date: 2025-11-06
Error Encountered: Drizzle migration registry drift routinely reappeared after database resets, requiring manual hash seeding before every deploy.
Date of Fix: 2025-11-06
Details: Built `src/db/syncMigrations.ts` to hash each migration file, backfill missing rows in `_drizzle_migrations`, and warn on hash mismatches. Added npm script `npm run db:sync-migrations` so environments can self-heal before running `npm run db:migrate`.

---

Date: 2025-11-06
Error Encountered: Email verification codes were hashed before storage, but the schema and migration still defined `verificationCode` as `varchar(6)`.
Date of Fix: 2025-11-06
Details: Updated `src/db/schema.ts` and `drizzle/0001_faulty_shinko_yamashiro.sql` to declare the column as `text`, then marked seeded users as verified to keep login smoke tests working.

---

Date: 2025-11-06
Error Encountered: ESP32 scanner showed a white screen immediately after initialization.
Date of Fix: 2025-11-06
Details: Traced the regression to a keypad pin map (`{4,2,15,5}`) that conflicted with the TFT_eSPI SPI lines. Restored the proven mapping `{5,19,21,22}` for columns and `{25,26,32,33}` for rows, matching the successful diagnostic sketch.

---

Date: 2025-11-06
Error Encountered: Production device registration returned HTTP 500 errors while local registration worked correctly.
Date of Fix: 2025-11-07
Details: Investigation revealed two critical issues: (1) Production database schema still had `varchar(64)` for API key columns, unable to store PBKDF2 hashes (~111 chars); (2) Device authentication middleware was re-hashing incoming keys instead of verifying against stored hashes. Fixed by updating schema to use `text` columns, implementing `verifyApiKey()` for proper hash verification, creating `syncMigrations.ts` for automatic migration registry synchronization, and updating `clean-db.ts` to drop enums during cleanup. Generated migration `0002_happy_mattie_franklin.sql` to apply column type changes.

---

Date: 2025-11-07
Error Encountered: Frontend device management page showed empty device rows despite successful API responses containing device data.
Date of Fix: 2025-11-07
Details: Backend API returns devices in nested structure `{ success: true, data: { devices: [...], total: n } }`, but frontend service was accessing `response.data` directly instead of `response.data.devices`. Updated `getAllDevices()` in `frontend/src/services/device.ts` to correctly access the nested devices array.

---

Date: 2025-11-07
Error Encountered: Device registration on production returned 409 Conflict errors, appearing as registration failure.
Date of Fix: 2025-11-07
Details: Not a bug—correct behavior. HTTP 409 indicates device with that MAC address already exists in database (duplicate prevention). Users attempting to re-register existing devices should either use a different MAC address, delete the existing device first, or use the existing device. Verified production database contains three registered devices (001122334455, AABBCCDDEEFF, 80F3DA4C46A4) from seed data and prior registrations.

---

Date: 2025-11-07
Error Encountered: Frontend device deletion returned 404 "Route not found" despite correct API URL (`DELETE /api/devices/:deviceId`).
Date of Fix: 2025-11-07
Details: DELETE route handler was incorrectly nested inside the PUT handler in `backend-workers/src/routes/device.ts`, preventing Hono from registering it at startup. Moved `app.delete("/:deviceId", ...)` block to module level (after PUT handler) so the route is properly registered. DELETE now responds with 200 success or 404 if device doesn't exist.

---

Date: 2025-11-10
Error Encountered: ESP32 diagnostics firmware attempted WebSocket connection to `/ws/device` endpoint but received HTTP 200 OK JSON response instead of WebSocket upgrade (101 Switching Protocols).
Date of Fix: 2025-11-11
Details: Backend `/ws/device` endpoint was implemented as standard HTTP GET handler returning JSON, not performing WebSocket handshake. Since Cloudflare Workers free tier doesn't support native WebSockets without Durable Objects (paid feature), migrated architecture from WebSocket to HTTP polling. Changes: (1) Backend: Added `GET /api/devices/:deviceId/commands` endpoint returning pending commands (registration mode, scan mode, etc.) in JSON format; (2) ESP32: Removed `WebSocketsClient` library dependency, replaced `webSocket.loop()` with `pollCommands()` calling backend every 5 seconds (`COMMAND_POLL_INTERVAL`), replaced `sendWebSocketHeartbeat()` with existing `testAPIHeartbeat()` every 30 seconds (`HEARTBEAT_INTERVAL`); (3) Registration flow: Changed from bidirectional WebSocket messages to server-controlled via polling—admin enables registration in panel, device polls `/commands` endpoint, receives `enable_registration` action with expected tag ID, validates scanned cards against server-provided ID; (4) UI: Renamed "WebSocket Test" stage to "Command Poll Test", updated menu option "8: WebSocket" → "8: Cmd Poll", replaced `drawWebSocketTest()` with `drawCommandPollTest()` showing poll count and HTTP status codes; (5) Config: Removed `WS_HOST`, `WS_PORT`, `WS_PATH`, `USE_SECURE_WS`, `WS_RECONNECT_INTERVAL`, `WS_PING_INTERVAL` from `Config.h`, added `COMMAND_POLL_INTERVAL` (5000ms) and `HEARTBEAT_INTERVAL` (30000ms). System now fully functional on Cloudflare Workers free tier with HTTP polling architecture replacing WebSocket real-time communication.

---

Date: 2025-11-11
Error Encountered: Frontend admin panels displayed “No online RFID devices found” even while diagnostics firmware reported the device active, due to backend returning raw device rows without online/offline metadata and the frontend assuming nested `data.devices` responses.
Date of Fix: 2025-11-11
Details: Updated backend `/api/devices`, `/api/devices/active`, heartbeat, and mode endpoints to enrich every device payload with `status`, ISO `lastSeen`, and `lastSeenAgoSeconds`, treating missing heartbeats as offline. Frontend services now normalize those responses, filter stale records, and map the enriched fields into dashboard and RFID card views, restoring accurate device presence indicators across the UI.

---

Date: 2025-11-11
Error Encountered: RFID card registration “fiasco” — ESP32 scans reached the worker logs, but neither the backend nor the Vue admin panel surfaced the tag for assignment, so cards could not be registered.
Date of Fix: 2025-11-12
Details: Discovered three compounding issues: (1) `rfid.ts` stored device metadata as `null`, causing `JSON.parse` failures on the frontend; (2) API responses returned nested structures that the Vue services weren’t normalizing, so unregistered scans never appeared in the tables; (3) Registration mode expected WebSocket acknowledgements that no longer existed after moving to HTTP polling. Fixed by defaulting metadata to `{}` in the worker, normalizing RFID scan payloads (flattening device info and counts), updating Vue services/components to consume the new shape, and wiring the registration modal to the HTTP command-poll workflow. After redeploying firmware and frontend, scanned cards now surface immediately for registration and persist correctly in the database.

---

Date: 2025-11-12
Error Encountered: User Management and RFID card assignment views rendered empty tables even though `/api/users` returned data.
Date of Fix: 2025-11-12
Details: Axios interceptor already unwraps the `{ success, data }` envelope, but `userService.getUsers()` still expected a nested `response.data` object, returning `undefined`. Normalized the service to output an array and updated consuming views (`UserManagement.vue`, `RfidCardManagement.vue`, dashboard stats) to read the shared `User` type, restoring user listings across the UI.

---

Date: 2025-11-16
Error Encountered: RFID registration modal stalled on "Continue & Wait for Tap" even after the confirmation tap, leaving cards stuck in the unregistered list.
Date of Fix: 2025-11-16
Details: Backend lacked a way to verify the follow-up scan, so the frontend never received a success signal. Added `/api/rfid/check-recent-scan/:tagId` to validate confirmation taps, filtered registered tags out of the unregistered scan feed, and refactored `RfidCardManagement.vue` to call the new endpoint before completing registration. Frontend service now normalizes the response and closes the loop so cards move to the registered list immediately after the confirmation tap.

---
