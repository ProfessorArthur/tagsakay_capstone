# Environment Configuration Guide

This project uses a standardized environment configuration system to manage all environment variables centrally. This makes it easy to update values and ensures consistency across the application.

## Backend Configuration

### Centralized Config File

All environment variables are managed through `backend/src/config/env.js`. This file:

- Loads and validates environment variables
- Provides default values where appropriate
- Exports typed configuration objects
- Validates required environment variables on startup

### Configuration Sections

#### Server Configuration

```javascript
import { SERVER_CONFIG } from "./config/env.js";
// Access: SERVER_CONFIG.PORT, SERVER_CONFIG.NODE_ENV
```

#### Database Configuration

```javascript
import { DB_CONFIG } from "./config/env.js";
// Access: DB_CONFIG.HOST, DB_CONFIG.PORT, DB_CONFIG.USER, etc.
```

#### JWT Configuration

```javascript
import { JWT_CONFIG } from "./config/env.js";
// Access: JWT_CONFIG.SECRET, JWT_CONFIG.EXPIRES_IN
```

#### API Configuration

```javascript
import { API_CONFIG } from "./config/env.js";
// Access: API_CONFIG.URL
```

### Usage Example

Instead of using `process.env.DB_HOST` directly:

```javascript
// ❌ Old way - scattered env usage
const host = process.env.DB_HOST;
const port = process.env.DB_PORT || 5432;

// ✅ New way - centralized config
import { DB_CONFIG } from "./config/env.js";
const host = DB_CONFIG.HOST;
const port = DB_CONFIG.PORT;
```

## Frontend Configuration

### Centralized Config File

All environment variables are managed through `frontend/src/config/env.ts`. This file:

- Loads Vite environment variables
- Provides TypeScript types
- Exports configuration objects

### Usage Example

Instead of using `import.meta.env.VITE_API_URL` directly:

```typescript
// ❌ Old way
const apiUrl = import.meta.env.VITE_API_URL || "http://localhost:3000/api";

// ✅ New way
import { API_CONFIG } from "./config/env";
const apiUrl = API_CONFIG.BASE_URL;
```

## Environment Files

### Backend (.env)

```bash
# Server Configuration
PORT=3000

# Database Configuration
DB_HOST=localhost
DB_PORT=5432
DB_USER=postgres
DB_PASSWORD=Postgre4017
DB_NAME=tagsakay_db

# JWT Configuration
JWT_SECRET=your-secret-key
JWT_EXPIRES_IN=1d
```

### Frontend (.env)

```bash
VITE_API_URL=http://localhost:3000/api
VITE_API_TIMEOUT=10000
```

## Benefits

1. **Single Source of Truth**: All environment variable access is centralized
2. **Validation**: Required variables are validated on startup
3. **Type Safety**: TypeScript support for configuration objects
4. **Default Values**: Consistent fallback values
5. **Easy Refactoring**: Change variable names in one place
6. **Documentation**: Clear configuration structure

## Migration Guide

If you need to add a new environment variable:

1. Add it to the appropriate `.env` file
2. Add it to the centralized config file (`env.js` or `env.ts`)
3. Add validation if it's required
4. Import and use the config object instead of accessing `process.env` directly

## Important Notes

- Never access `process.env` or `import.meta.env` directly in application code
- Always use the centralized configuration objects
- Required variables will cause the application to fail fast if missing
- Environment variables are loaded once at startup for better performance
