import { Context, Next } from "hono";

// ============================================================================
// SECURITY HEADERS MIDDLEWARE - OWASP Compliant
// ============================================================================
// Implements all recommended security headers for REST APIs

export async function securityHeaders(c: Context, next: Next) {
  await next();

  // Cache-Control: Prevent caching of API responses
  c.res.headers.set(
    "Cache-Control",
    "no-store, no-cache, must-revalidate, private"
  );
  c.res.headers.set("Pragma", "no-cache");
  c.res.headers.set("Expires", "0");

  // Content-Security-Policy: Prevent framing
  c.res.headers.set(
    "Content-Security-Policy",
    "frame-ancestors 'none'; default-src 'none'"
  );

  // Strict-Transport-Security: Force HTTPS
  // Only add if request is over HTTPS
  if (c.req.url.startsWith("https://")) {
    c.res.headers.set(
      "Strict-Transport-Security",
      "max-age=31536000; includeSubDomains; preload"
    );
  }

  // X-Content-Type-Options: Prevent MIME sniffing
  c.res.headers.set("X-Content-Type-Options", "nosniff");

  // X-Frame-Options: Prevent clickjacking
  c.res.headers.set("X-Frame-Options", "DENY");

  // X-XSS-Protection: Enable browser XSS protection (legacy browsers)
  c.res.headers.set("X-XSS-Protection", "1; mode=block");

  // Referrer-Policy: Don't leak referrer information
  c.res.headers.set("Referrer-Policy", "no-referrer");

  // Permissions-Policy: Disable unnecessary browser features
  c.res.headers.set(
    "Permissions-Policy",
    "accelerometer=(), ambient-light-sensor=(), autoplay=(), battery=(), camera=(), " +
      "cross-origin-isolated=(), display-capture=(), document-domain=(), encrypted-media=(), " +
      "execution-while-not-rendered=(), execution-while-out-of-viewport=(), fullscreen=(), " +
      "geolocation=(), gyroscope=(), keyboard-map=(), magnetometer=(), microphone=(), " +
      "midi=(), navigation-override=(), payment=(), picture-in-picture=(), " +
      "publickey-credentials-get=(), screen-wake-lock=(), sync-xhr=(), usb=(), " +
      "web-share=(), xr-spatial-tracking=()"
  );

  // Content-Type: Ensure correct content type is set
  if (!c.res.headers.has("Content-Type")) {
    c.res.headers.set("Content-Type", "application/json; charset=utf-8");
  }

  // Remove potentially dangerous headers
  c.res.headers.delete("X-Powered-By");
  c.res.headers.delete("Server");
}

// ============================================================================
// CONTENT-TYPE VALIDATION MIDDLEWARE
// ============================================================================
// Validates that request Content-Type matches expected types

const ALLOWED_CONTENT_TYPES = [
  "application/json",
  "application/x-www-form-urlencoded",
  "multipart/form-data",
];

export async function validateContentType(c: Context, next: Next) {
  const method = c.req.method;

  // Only validate for methods that typically have a body
  if (["POST", "PUT", "PATCH"].includes(method)) {
    const contentType = c.req.header("Content-Type");
    const contentLength = c.req.header("Content-Length");

    // If Content-Length is 0, Content-Type is optional
    if (contentLength === "0") {
      await next();
      return;
    }

    if (!contentType) {
      return c.json(
        {
          success: false,
          message: "Content-Type header is required",
        },
        415
      );
    }

    // Extract base content type (remove charset, boundary, etc.)
    const baseContentType = contentType.split(";")[0].trim();

    // Check if content type is in allowed list
    const isAllowed = ALLOWED_CONTENT_TYPES.some(
      (allowed) => baseContentType === allowed
    );

    if (!isAllowed) {
      return c.json(
        {
          success: false,
          message: "Unsupported Media Type. Expected: application/json",
        },
        415
      );
    }
  }

  await next();
}

// ============================================================================
// REQUEST SIZE LIMIT MIDDLEWARE
// ============================================================================
// Prevents DoS attacks via large request bodies

const MAX_REQUEST_SIZE = 10 * 1024 * 1024; // 10MB

export async function requestSizeLimit(c: Context, next: Next) {
  const contentLength = c.req.header("Content-Length");

  if (contentLength) {
    const size = parseInt(contentLength, 10);

    if (isNaN(size) || size > MAX_REQUEST_SIZE) {
      return c.json(
        {
          success: false,
          message: "Request Entity Too Large. Maximum size: 10MB",
        },
        413
      );
    }
  }

  await next();
}

// ============================================================================
// CORS SECURITY MIDDLEWARE
// ============================================================================
// Implements secure CORS with specific origins

export function secureCORS(allowedOrigins: string[]) {
  return async (c: Context, next: Next) => {
    const origin = c.req.header("Origin");

    // Only set CORS headers if origin is in allowed list
    if (origin && allowedOrigins.includes(origin)) {
      c.res.headers.set("Access-Control-Allow-Origin", origin);
      c.res.headers.set(
        "Access-Control-Allow-Methods",
        "GET, POST, PUT, DELETE, OPTIONS"
      );
      c.res.headers.set(
        "Access-Control-Allow-Headers",
        "Content-Type, Authorization, X-API-Key"
      );
      c.res.headers.set("Access-Control-Allow-Credentials", "true");
      c.res.headers.set("Access-Control-Max-Age", "86400"); // 24 hours
    }

    // Handle preflight requests
    if (c.req.method === "OPTIONS") {
      return new Response(null, { status: 204 });
    }

    await next();
  };
}
