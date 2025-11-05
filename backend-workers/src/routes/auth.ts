import { Hono } from "hono";
import {
  generateJWT,
  verifyPassword,
  hashPassword,
  verifyJWT,
  validatePasswordStrength,
} from "../lib/auth";
import { sendVerificationEmail } from "../lib/email";
import { users } from "../db/schema";
import { eq } from "drizzle-orm";
import type { Database } from "../db";
import { authMiddleware, requireRole } from "../middleware/auth";
import {
  authRateLimit,
  checkAccountLock,
  recordFailedLogin,
  resetLoginAttempts,
} from "../middleware/rateLimit";
import { validateRequestBody, validateEmail } from "../lib/validation";
import {
  logLoginSuccess,
  logLoginFailure,
  logAccountLocked,
  logPasswordChange,
  logValidationFailure,
  SecurityEventType,
  SeverityLevel,
  securityLogger,
} from "../lib/securityLogger";

type Env = {
  Bindings: {
    DATABASE_URL: string;
    JWT_SECRET: string;
    RESEND_API_KEY: string;
  };
  Variables: {
    db: Database;
    user?: any;
  };
};

const app = new Hono<Env>();

// Helper to get IP address
function getIpAddress(c: any): string {
  return (
    c.req.header("CF-Connecting-IP") ||
    c.req.header("X-Forwarded-For") ||
    c.req.header("X-Real-IP") ||
    "unknown"
  );
}

// POST /api/auth/login - Enhanced with rate limiting and account lockout
app.post("/login", authRateLimit, async (c) => {
  const db = c.get("db");
  const ipAddress = getIpAddress(c);
  const userAgent = c.req.header("User-Agent");

  // Parse and validate request body
  const body = await c.req.json();
  const { email, password } = body;

  // Input validation
  const emailValidation = validateEmail(email);
  if (!emailValidation.valid) {
    logValidationFailure(
      "/api/auth/login",
      [emailValidation.error!],
      ipAddress
    );
    return c.json(
      {
        success: false,
        message: "Invalid email or password", // Generic message
      },
      400
    );
  }

  if (!email || !password) {
    logValidationFailure("/api/auth/login", ["Missing credentials"], ipAddress);
    return c.json(
      {
        success: false,
        message: "Invalid email or password",
      },
      400
    );
  }

  // Check if account is locked due to failed attempts
  const lockStatus = checkAccountLock(email);
  if (lockStatus.locked) {
    const remainingTime = Math.ceil(
      (lockStatus.lockedUntil! - Date.now()) / 1000 / 60
    );
    logLoginFailure(email, "Account locked", ipAddress, userAgent);

    return c.json(
      {
        success: false,
        message: "Account temporarily locked. Please try again later.",
        retryAfter: `${remainingTime} minutes`,
      },
      429
    );
  }

  // Fetch user
  const [user] = await db
    .select()
    .from(users)
    .where(eq(users.email, email))
    .limit(1);

  if (!user) {
    // Record failed attempt (even for non-existent users to prevent enumeration)
    recordFailedLogin(email);
    logLoginFailure(email, "User not found", ipAddress, userAgent);

    return c.json(
      {
        success: false,
        message: "Invalid email or password", // Generic message
      },
      401
    );
  }

  // Verify password
  const isValid = await verifyPassword(password, user.password);

  if (!isValid) {
    const attemptResult = recordFailedLogin(email);
    logLoginFailure(email, "Invalid password", ipAddress, userAgent);

    if (attemptResult.locked) {
      logAccountLocked(email, 15 * 60 * 1000, ipAddress);

      return c.json(
        {
          success: false,
          message: "Too many failed attempts. Account temporarily locked.",
          retryAfter: "15 minutes",
        },
        429
      );
    }

    return c.json(
      {
        success: false,
        message: "Invalid email or password", // Generic message
      },
      401
    );
  }

  // Check if account is active
  if (!user.isActive) {
    logLoginFailure(email, "Account inactive", ipAddress, userAgent);
    return c.json(
      {
        success: false,
        message: "Account is inactive. Please contact support.",
      },
      403
    );
  }

  try {
    // Generate JWT with shorter expiration (4 hours)
    const token = await generateJWT(
      {
        id: user.id,
        email: user.email,
        role: user.role,
        name: user.name,
      },
      c.env.JWT_SECRET,
      "4h" // Reduced from 24h
    );

    // Reset failed login attempts on successful login
    resetLoginAttempts(email);

    // Log successful login
    logLoginSuccess(email, ipAddress, userAgent);

    return c.json({
      success: true,
      message: "Login successful",
      data: {
        token,
        expiresIn: "4h",
        user: {
          id: user.id,
          name: user.name,
          email: user.email,
          role: user.role,
          rfidTag: user.rfidTag,
        },
      },
    });
  } catch (error: any) {
    securityLogger.log({
      eventType: SecurityEventType.ERROR,
      severity: SeverityLevel.HIGH,
      username: email,
      ipAddress,
      message: "JWT generation failed during login",
      metadata: { error: error.message },
    });

    return c.json(
      {
        success: false,
        message: "Login failed. Please try again.",
      },
      500
    );
  }
});

// POST /api/auth/register - Enhanced with validation and password strength checking
app.post("/register", authRateLimit, async (c) => {
  const db = c.get("db");
  const ipAddress = getIpAddress(c);

  const body = await c.req.json();

  // Validate request body
  const validation = validateRequestBody(body, {
    name: { type: "string", required: true, minLength: 2, maxLength: 100 },
    email: { type: "email", required: true },
    password: { type: "string", required: true, minLength: 8, maxLength: 128 },
    role: {
      type: "enum",
      allowedValues: ["superadmin", "admin", "driver"] as const,
      required: false,
    },
    rfidTag: { type: "rfid", required: false },
  });

  if (!validation.valid) {
    logValidationFailure("/api/auth/register", validation.errors, ipAddress);
    return c.json(
      {
        success: false,
        message: "Validation failed",
        errors: validation.errors,
      },
      400
    );
  }

  const {
    name,
    email,
    password,
    role = "driver",
    rfidTag,
  } = validation.sanitized!;

  // Check password strength (without MFA, min 15 chars recommended)
  const passwordCheck = validatePasswordStrength(password, false);
  if (!passwordCheck.isValid) {
    logValidationFailure(
      "/api/auth/register",
      passwordCheck.errors,
      ipAddress,
      email
    );
    return c.json(
      {
        success: false,
        message: "Password does not meet security requirements",
        errors: passwordCheck.errors,
      },
      400
    );
  }

  // Check if email already exists
  const [existingUser] = await db
    .select()
    .from(users)
    .where(eq(users.email, email))
    .limit(1);

  if (existingUser) {
    // Don't log this as it's expected behavior
    return c.json(
      {
        success: false,
        message: "A link to activate your account has been emailed.", // Generic message per OWASP
      },
      400
    );
  }

  // Check if RFID tag already exists (if provided)
  if (rfidTag) {
    const [existingRfid] = await db
      .select()
      .from(users)
      .where(eq(users.rfidTag, rfidTag))
      .limit(1);

    if (existingRfid) {
      logValidationFailure(
        "/api/auth/register",
        ["RFID tag already assigned"],
        ipAddress,
        email
      );
      return c.json(
        {
          success: false,
          message: "Registration failed. Please try again or contact support.",
        },
        409
      );
    }
  }

  try {
    // Hash password with PBKDF2
    const hashedPassword = await hashPassword(password);

    // Generate 6-digit verification code
    const verificationCode = Math.random().toString().substring(2, 8);
    const expiryTime = new Date(Date.now() + 24 * 60 * 60 * 1000); // 24 hours

    const [newUser] = await db
      .insert(users)
      .values({
        name,
        email,
        password: hashedPassword,
        role: role as "superadmin" | "admin" | "driver",
        rfidTag: rfidTag || null,
        isActive: true,
        isEmailVerified: false, // Not verified yet
        verificationCode,
        verificationCodeExpiry: expiryTime,
      })
      .returning();

    // Send verification email
    const emailResult = await sendVerificationEmail(
      c.env.RESEND_API_KEY,
      email,
      verificationCode,
      "https://tagsakay.com"
    );

    if (!emailResult.success) {
      console.error("Failed to send verification email:", emailResult.error);
      // Don't fail registration, just log the error
    }

    // Log successful registration
    securityLogger.log({
      eventType: SecurityEventType.LOGIN_SUCCESS,
      severity: SeverityLevel.LOW,
      username: email,
      ipAddress,
      message: `New user registered (awaiting email verification): ${email}`,
    });

    // Return response - user is registered but not verified yet
    return c.json(
      {
        success: true,
        message:
          "Registration successful! Check your email to verify your account.",
        data: {
          email: newUser.email,
          verified: false,
          // Don't send token yet - user must verify first
        },
      },
      201
    );
  } catch (error: any) {
    securityLogger.log({
      eventType: SecurityEventType.ERROR,
      severity: SeverityLevel.HIGH,
      username: email,
      ipAddress,
      message: "Registration failed",
      metadata: { error: error.message },
    });

    return c.json(
      {
        success: false,
        message: "Registration failed. Please try again.",
      },
      500
    );
  }
});

// POST /api/auth/verify-email - Verify email with code
app.post("/verify-email", async (c) => {
  const db = c.get("db");
  const ipAddress = getIpAddress(c);

  const body = await c.req.json();
  const { email, code } = body;

  // Validate inputs
  if (!email || !code) {
    return c.json(
      {
        success: false,
        message: "Email and verification code are required",
      },
      400
    );
  }

  try {
    const [user] = await db
      .select()
      .from(users)
      .where(eq(users.email, email))
      .limit(1);

    if (!user) {
      logValidationFailure(
        "/api/auth/verify-email",
        ["User not found"],
        ipAddress,
        email
      );
      return c.json(
        {
          success: false,
          message: "User not found",
        },
        404
      );
    }

    if (user.isEmailVerified) {
      return c.json(
        {
          success: false,
          message: "Email already verified",
        },
        400
      );
    }

    // Check if code matches and not expired
    if (user.verificationCode !== code) {
      logValidationFailure(
        "/api/auth/verify-email",
        ["Invalid verification code"],
        ipAddress,
        email
      );
      return c.json(
        {
          success: false,
          message: "Invalid verification code",
        },
        400
      );
    }

    if (
      !user.verificationCodeExpiry ||
      new Date() > user.verificationCodeExpiry
    ) {
      return c.json(
        {
          success: false,
          message: "Verification code has expired. Please register again.",
        },
        400
      );
    }

    // Mark as verified
    await db
      .update(users)
      .set({
        isEmailVerified: true,
        verificationCode: null,
        verificationCodeExpiry: null,
      })
      .where(eq(users.id, user.id));

    // Generate token for login
    const token = await generateJWT(
      {
        id: user.id,
        email: user.email,
        role: user.role,
        name: user.name,
      },
      c.env.JWT_SECRET,
      "4h"
    );

    // Log successful verification
    securityLogger.log({
      eventType: SecurityEventType.LOGIN_SUCCESS,
      severity: SeverityLevel.LOW,
      username: email,
      ipAddress,
      message: `Email verified: ${email}`,
    });

    return c.json({
      success: true,
      message: "Email verified successfully",
      data: {
        token,
        expiresIn: "4h",
        user: {
          id: user.id,
          name: user.name,
          email: user.email,
          role: user.role,
          rfidTag: user.rfidTag,
        },
      },
    });
  } catch (error: any) {
    securityLogger.log({
      eventType: SecurityEventType.ERROR,
      severity: SeverityLevel.HIGH,
      username: email,
      ipAddress,
      message: "Email verification failed",
      metadata: { error: error.message },
    });

    return c.json(
      {
        success: false,
        message: "Verification failed. Please try again.",
      },
      500
    );
  }
});

// POST /api/auth/refresh
app.post("/refresh", async (c) => {
  const authHeader = c.req.header("Authorization");

  if (!authHeader || !authHeader.startsWith("Bearer ")) {
    return c.json(
      {
        success: false,
        message: "No token provided",
      },
      401
    );
  }

  const token = authHeader.substring(7);

  try {
    // Verify the old token (even if expired, we can extract the payload)
    const payload = await verifyJWT(token, c.env.JWT_SECRET);

    const db = c.get("db");

    // Verify user still exists and is active
    const [user] = await db
      .select()
      .from(users)
      .where(eq(users.id, payload.id))
      .limit(1);

    if (!user) {
      return c.json(
        {
          success: false,
          message: "User not found",
        },
        404
      );
    }

    if (!user.isActive) {
      return c.json(
        {
          success: false,
          message: "Account is inactive",
        },
        403
      );
    }

    // Generate new token with updated data
    const newToken = await generateJWT(
      {
        id: user.id,
        email: user.email,
        role: user.role,
        name: user.name,
      },
      c.env.JWT_SECRET
    );

    return c.json({
      success: true,
      message: "Token refreshed successfully",
      data: {
        token: newToken,
        user: {
          id: user.id,
          name: user.name,
          email: user.email,
          role: user.role,
          rfidTag: user.rfidTag,
        },
      },
    });
  } catch (error: any) {
    console.error("Token refresh error:", error);
    return c.json(
      {
        success: false,
        message: "Token refresh failed",
        error: error.message,
      },
      401
    );
  }
});

// POST /api/auth/logout
app.post("/logout", authMiddleware, async (c) => {
  // In a stateless JWT system, logout is handled client-side by removing the token
  // However, we can add server-side logic here if needed (e.g., token blacklisting)

  const user = c.get("user");

  // Optional: Log the logout event
  console.log(`User ${user.email} (ID: ${user.id}) logged out`);

  // Optional: You could add token to a blacklist table here
  // For now, we'll just return success
  return c.json({
    success: true,
    message: "Logout successful",
    data: {
      loggedOut: true,
    },
  });
});

// Add more auth endpoints: register, refresh, logout, etc.

export default app;
