# Legacy Backend Archive

This directory contains archived documentation from the Express.js/Sequelize backend that was removed on November 2, 2025.

## Why Archived?

The project transitioned from a traditional Express.js backend to Cloudflare Workers with OWASP-compliant security features. The legacy backend was removed to:

1. **Reduce Complexity**: Eliminate duplicate backend implementations
2. **Improve Security**: Focus on the OWASP-compliant backend-workers
3. **Simplify Maintenance**: Single source of truth for API implementation
4. **Clean Architecture**: Streamline the monorepo structure

## What Was the Legacy Backend?

- **Framework**: Express.js + Sequelize ORM
- **Database**: PostgreSQL (local)
- **Port**: 3000
- **Features**:
  - User management (JWT auth)
  - RFID tag management
  - Device registration
  - API key system
  - Database migrations/seeding scripts

## Migration to Backend-Workers

The backend-workers implementation provides all the same functionality with enhanced security:

- **Framework**: Hono (Cloudflare Workers)
- **ORM**: Drizzle (type-safe, schema-first)
- **Database**: Neon PostgreSQL (serverless)
- **Port**: 8787 (development)
- **Enhanced Features**:
  - OWASP Top 10 2021 compliance
  - PBKDF2 password hashing (600k iterations)
  - Rate limiting (5/min auth, 100/min API)
  - Account lockout (5 attempts, 15 min)
  - Security headers (10 OWASP-recommended)
  - Security logging (15 event types)
  - Input validation (RFC 5321 email, RFID, MAC)

## Archived Documentation

- **device-api.md**: Device management API endpoints (legacy format)

## Need the Legacy Code?

The legacy backend code exists in git history. To access it:

```bash
# View git history before removal
git log --all --full-history -- backend/

# Checkout a specific commit that had the backend
git checkout <commit-hash> -- backend/
```

## Frontend Migration

The frontend was migrated from port 3000 to port 8787. Update your `.env`:

```env
# Old (Legacy)
VITE_API_URL=http://localhost:3000/api

# New (Backend-Workers)
VITE_API_URL=http://localhost:8787/api
```

## Database Migration

If you have data in the legacy PostgreSQL database that needs to be migrated:

1. Export data from legacy database
2. Transform to match new Drizzle schema (see `backend-workers/src/db/schema.ts`)
3. Import into Neon PostgreSQL database

Contact the development team for migration scripts if needed.

## Questions?

- For backend-workers documentation: See `/backend-workers/README.md`
- For API reference: See `/backend-workers/docs/`
- For test results: See `/backend-workers/tests/TEST_RESULTS.md`
- For migration guide: See `/backend-workers/MIGRATION_SUMMARY.md`
