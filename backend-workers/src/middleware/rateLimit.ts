import { Context, Next } from "hono";

// ============================================================================
// RATE LIMITING MIDDLEWARE - OWASP Compliant
// ============================================================================
// Implements in-memory rate limiting (for production, use KV or Durable Objects)
// Prevents brute-force and DoS attacks

interface RateLimitStore {
  [key: string]: {
    count: number;
    resetAt: number;
    lockedUntil?: number;
  };
}

const rateLimitStore: RateLimitStore = {};
let lastCleanup = Date.now();

// Manual cleanup - runs periodically when accessed (Cloudflare Workers compatible)
function cleanupRateLimitStore() {
  const now = Date.now();

  // Only cleanup every 5 minutes
  if (now - lastCleanup < 5 * 60 * 1000) {
    return;
  }

  lastCleanup = now;
  Object.keys(rateLimitStore).forEach((key) => {
    if (
      rateLimitStore[key].resetAt < now &&
      (!rateLimitStore[key].lockedUntil ||
        rateLimitStore[key].lockedUntil! < now)
    ) {
      delete rateLimitStore[key];
    }
  });
}

export interface RateLimitOptions {
  maxRequests: number;
  windowMs: number;
  keyPrefix?: string;
  skipSuccessfulRequests?: boolean;
  message?: string;
}

export function rateLimit(options: RateLimitOptions) {
  return async (c: Context, next: Next) => {
    // Run cleanup periodically
    cleanupRateLimitStore();

    const ip =
      c.req.header("CF-Connecting-IP") ||
      c.req.header("X-Forwarded-For") ||
      c.req.header("X-Real-IP") ||
      "unknown";

    const key = `${options.keyPrefix || "rl"}:${ip}:${c.req.path}`;
    const now = Date.now();

    // Initialize or get existing entry
    if (!rateLimitStore[key] || rateLimitStore[key].resetAt < now) {
      rateLimitStore[key] = {
        count: 0,
        resetAt: now + options.windowMs,
      };
    }

    const entry = rateLimitStore[key];

    // Check if account is locked
    if (entry.lockedUntil && entry.lockedUntil > now) {
      const retryAfter = Math.ceil((entry.lockedUntil - now) / 1000);

      return c.json(
        {
          success: false,
          message: "Too many requests. Account temporarily locked.",
          retryAfter: `${retryAfter} seconds`,
        },
        429,
        {
          "Retry-After": retryAfter.toString(),
          "X-RateLimit-Limit": options.maxRequests.toString(),
          "X-RateLimit-Remaining": "0",
          "X-RateLimit-Reset": entry.lockedUntil.toString(),
        }
      );
    }

    // Increment request count
    entry.count++;

    // Check if rate limit exceeded
    if (entry.count > options.maxRequests) {
      // Lock account for increasing duration (exponential backoff)
      const lockDurationMs = Math.min(
        options.windowMs *
          Math.pow(2, Math.floor(entry.count / options.maxRequests) - 1),
        60 * 60 * 1000 // Max 1 hour
      );

      entry.lockedUntil = now + lockDurationMs;

      const retryAfter = Math.ceil(lockDurationMs / 1000);

      return c.json(
        {
          success: false,
          message:
            options.message || "Too many requests. Please try again later.",
          retryAfter: `${retryAfter} seconds`,
        },
        429,
        {
          "Retry-After": retryAfter.toString(),
          "X-RateLimit-Limit": options.maxRequests.toString(),
          "X-RateLimit-Remaining": "0",
          "X-RateLimit-Reset": entry.lockedUntil.toString(),
        }
      );
    }

    // Set rate limit headers
    c.res.headers.set("X-RateLimit-Limit", options.maxRequests.toString());
    c.res.headers.set(
      "X-RateLimit-Remaining",
      (options.maxRequests - entry.count).toString()
    );
    c.res.headers.set("X-RateLimit-Reset", entry.resetAt.toString());

    await next();

    // If request was successful and we should skip counting successful requests
    if (options.skipSuccessfulRequests && c.res.status < 400) {
      entry.count--;
    }
  };
}

// ============================================================================
// PRESET RATE LIMITERS
// ============================================================================

// Strict rate limit for authentication endpoints (5 requests per minute)
export const authRateLimit = rateLimit({
  maxRequests: 5,
  windowMs: 60 * 1000, // 1 minute
  keyPrefix: "auth",
  skipSuccessfulRequests: true,
  message: "Too many authentication attempts. Please try again later.",
});

// Standard rate limit for general API endpoints (100 requests per minute)
export const apiRateLimit = rateLimit({
  maxRequests: 100,
  windowMs: 60 * 1000, // 1 minute
  keyPrefix: "api",
});

// Strict rate limit for device registration (3 requests per hour)
export const deviceRegisterRateLimit = rateLimit({
  maxRequests: 3,
  windowMs: 60 * 60 * 1000, // 1 hour
  keyPrefix: "dev-reg",
  message: "Too many device registration attempts. Please contact support.",
});

// ============================================================================
// LOGIN ATTEMPT TRACKING (Account-based, not IP-based)
// ============================================================================

interface LoginAttemptStore {
  [username: string]: {
    attempts: number;
    lockedUntil?: number;
    lastAttempt: number;
  };
}

const loginAttempts: LoginAttemptStore = {};
let lastLoginCleanup = Date.now();

// Manual cleanup for login attempts - Cloudflare Workers compatible
function cleanupLoginAttempts() {
  const now = Date.now();

  // Only cleanup every 10 minutes
  if (now - lastLoginCleanup < 10 * 60 * 1000) {
    return;
  }

  lastLoginCleanup = now;
  const maxAge = 60 * 60 * 1000; // 1 hour

  Object.keys(loginAttempts).forEach((username) => {
    const entry = loginAttempts[username];
    if (
      entry.lastAttempt + maxAge < now &&
      (!entry.lockedUntil || entry.lockedUntil < now)
    ) {
      delete loginAttempts[username];
    }
  });
}

export function recordFailedLogin(username: string): {
  locked: boolean;
  attemptsRemaining: number;
  lockedUntil?: number;
} {
  // Run cleanup periodically
  cleanupLoginAttempts();

  const MAX_ATTEMPTS = 5;
  const LOCK_DURATION = 15 * 60 * 1000; // 15 minutes
  const now = Date.now();

  if (!loginAttempts[username]) {
    loginAttempts[username] = {
      attempts: 0,
      lastAttempt: now,
    };
  }

  const entry = loginAttempts[username];

  // Reset attempts if last attempt was > 1 hour ago
  if (now - entry.lastAttempt > 60 * 60 * 1000) {
    entry.attempts = 0;
    delete entry.lockedUntil;
  }

  entry.attempts++;
  entry.lastAttempt = now;

  // Lock account if max attempts exceeded
  if (entry.attempts >= MAX_ATTEMPTS) {
    entry.lockedUntil = now + LOCK_DURATION;

    return {
      locked: true,
      attemptsRemaining: 0,
      lockedUntil: entry.lockedUntil,
    };
  }

  return {
    locked: false,
    attemptsRemaining: MAX_ATTEMPTS - entry.attempts,
  };
}

export function checkAccountLock(username: string): {
  locked: boolean;
  lockedUntil?: number;
} {
  // Run cleanup periodically
  cleanupLoginAttempts();

  const entry = loginAttempts[username];

  if (!entry || !entry.lockedUntil) {
    return { locked: false };
  }

  const now = Date.now();

  if (entry.lockedUntil > now) {
    return {
      locked: true,
      lockedUntil: entry.lockedUntil,
    };
  }

  // Lock expired, reset
  entry.attempts = 0;
  delete entry.lockedUntil;

  return { locked: false };
}

export function resetLoginAttempts(username: string): void {
  delete loginAttempts[username];
}

export function getLoginAttempts(username: string): number {
  return loginAttempts[username]?.attempts || 0;
}
