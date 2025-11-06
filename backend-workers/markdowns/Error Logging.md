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
