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
