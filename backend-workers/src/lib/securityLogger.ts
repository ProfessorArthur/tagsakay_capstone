// ============================================================================
// SECURITY LOGGING LIBRARY - OWASP Compliant
// ============================================================================
// Logs security-relevant events for audit trail and threat detection

export enum SecurityEventType {
  // Authentication Events
  LOGIN_SUCCESS = "LOGIN_SUCCESS",
  LOGIN_FAILURE = "LOGIN_FAILURE",
  LOGOUT = "LOGOUT",
  TOKEN_REFRESH = "TOKEN_REFRESH",
  TOKEN_INVALID = "TOKEN_INVALID",
  TOKEN_EXPIRED = "TOKEN_EXPIRED",

  // Authorization Events
  ACCESS_DENIED = "ACCESS_DENIED",
  ROLE_CHECK_FAILED = "ROLE_CHECK_FAILED",

  // Account Security Events
  ACCOUNT_LOCKED = "ACCOUNT_LOCKED",
  ACCOUNT_UNLOCKED = "ACCOUNT_UNLOCKED",
  PASSWORD_CHANGE = "PASSWORD_CHANGE",
  PASSWORD_RESET = "PASSWORD_RESET",

  // API Security Events
  RATE_LIMIT_EXCEEDED = "RATE_LIMIT_EXCEEDED",
  INVALID_API_KEY = "INVALID_API_KEY",
  API_KEY_CREATED = "API_KEY_CREATED",
  API_KEY_REVOKED = "API_KEY_REVOKED",

  // Input Validation Events
  VALIDATION_FAILED = "VALIDATION_FAILED",
  SUSPICIOUS_INPUT = "SUSPICIOUS_INPUT",
  INJECTION_ATTEMPT = "INJECTION_ATTEMPT",

  // Device Security Events
  DEVICE_REGISTERED = "DEVICE_REGISTERED",
  DEVICE_AUTH_FAILED = "DEVICE_AUTH_FAILED",
  UNREGISTERED_SCAN = "UNREGISTERED_SCAN",

  // System Events
  CONFIG_CHANGE = "CONFIG_CHANGE",
  ERROR = "ERROR",
}

export enum SeverityLevel {
  LOW = "LOW",
  MEDIUM = "MEDIUM",
  HIGH = "HIGH",
  CRITICAL = "CRITICAL",
}

export interface SecurityLogEntry {
  timestamp: string;
  eventType: SecurityEventType;
  severity: SeverityLevel;
  userId?: string;
  username?: string;
  deviceId?: string;
  ipAddress?: string;
  userAgent?: string;
  endpoint?: string;
  method?: string;
  message: string;
  metadata?: Record<string, any>;
}

// ============================================================================
// SECURITY LOGGER CLASS
// ============================================================================

class SecurityLogger {
  private logs: SecurityLogEntry[] = [];
  private maxLogs: number = 10000;
  private lastCleanup: number = Date.now();

  constructor() {
    // No automatic cleanup - Cloudflare Workers don't support timers at global scope
    // Cleanup runs manually on log() calls
  }

  private cleanup() {
    const now = Date.now();

    // Only cleanup every hour
    if (now - this.lastCleanup < 60 * 60 * 1000) {
      return;
    }

    this.lastCleanup = now;
    const maxAge = 24 * 60 * 60 * 1000; // 24 hours
    const cutoff = new Date(now - maxAge).toISOString();

    this.logs = this.logs.filter((log) => log.timestamp > cutoff);

    // Keep only most recent logs if still too many
    if (this.logs.length > this.maxLogs) {
      this.logs = this.logs.slice(-this.maxLogs);
    }
  }

  log(entry: Omit<SecurityLogEntry, "timestamp">) {
    // Run cleanup periodically
    this.cleanup();

    const logEntry: SecurityLogEntry = {
      ...entry,
      timestamp: new Date().toISOString(),
    };

    this.logs.push(logEntry);

    // Also log to console for debugging
    const level = this.getSeverityLogLevel(entry.severity);
    console[level](this.formatLogMessage(logEntry));
  }

  private getSeverityLogLevel(
    severity: SeverityLevel
  ): "log" | "warn" | "error" {
    switch (severity) {
      case SeverityLevel.CRITICAL:
      case SeverityLevel.HIGH:
        return "error";
      case SeverityLevel.MEDIUM:
        return "warn";
      default:
        return "log";
    }
  }

  private formatLogMessage(entry: SecurityLogEntry): string {
    const parts = [
      `[SECURITY][${entry.severity}]`,
      entry.eventType,
      entry.username ? `user=${entry.username}` : null,
      entry.ipAddress ? `ip=${entry.ipAddress}` : null,
      entry.endpoint ? `endpoint=${entry.endpoint}` : null,
      entry.message,
    ].filter(Boolean);

    return parts.join(" | ");
  }

  getRecentLogs(limit: number = 100): SecurityLogEntry[] {
    return this.logs.slice(-limit);
  }

  getLogsByUser(username: string, limit: number = 100): SecurityLogEntry[] {
    return this.logs.filter((log) => log.username === username).slice(-limit);
  }

  getLogsByType(
    eventType: SecurityEventType,
    limit: number = 100
  ): SecurityLogEntry[] {
    return this.logs.filter((log) => log.eventType === eventType).slice(-limit);
  }

  getLogsBySeverity(
    severity: SeverityLevel,
    limit: number = 100
  ): SecurityLogEntry[] {
    return this.logs.filter((log) => log.severity === severity).slice(-limit);
  }
}

// Export singleton instance
export const securityLogger = new SecurityLogger();

// ============================================================================
// HELPER FUNCTIONS FOR COMMON SECURITY EVENTS
// ============================================================================

export function logLoginSuccess(
  username: string,
  ipAddress?: string,
  userAgent?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.LOGIN_SUCCESS,
    severity: SeverityLevel.LOW,
    username,
    ipAddress,
    userAgent,
    message: `User ${username} logged in successfully`,
  });
}

export function logLoginFailure(
  username: string,
  reason: string,
  ipAddress?: string,
  userAgent?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.LOGIN_FAILURE,
    severity: SeverityLevel.MEDIUM,
    username,
    ipAddress,
    userAgent,
    message: `Login failed for user ${username}: ${reason}`,
    metadata: { reason },
  });
}

export function logAccountLocked(
  username: string,
  duration: number,
  ipAddress?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.ACCOUNT_LOCKED,
    severity: SeverityLevel.HIGH,
    username,
    ipAddress,
    message: `Account ${username} locked for ${duration}ms due to excessive failed login attempts`,
    metadata: { lockDuration: duration },
  });
}

export function logAccessDenied(
  username: string,
  endpoint: string,
  method: string,
  reason: string,
  ipAddress?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.ACCESS_DENIED,
    severity: SeverityLevel.MEDIUM,
    username,
    endpoint,
    method,
    ipAddress,
    message: `Access denied to ${method} ${endpoint} for user ${username}: ${reason}`,
    metadata: { reason },
  });
}

export function logInvalidToken(ipAddress?: string, userAgent?: string) {
  securityLogger.log({
    eventType: SecurityEventType.TOKEN_INVALID,
    severity: SeverityLevel.MEDIUM,
    ipAddress,
    userAgent,
    message: "Invalid JWT token detected",
  });
}

export function logRateLimitExceeded(
  endpoint: string,
  ipAddress?: string,
  username?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.RATE_LIMIT_EXCEEDED,
    severity: SeverityLevel.HIGH,
    endpoint,
    ipAddress,
    username,
    message: `Rate limit exceeded for ${endpoint}`,
  });
}

export function logValidationFailure(
  endpoint: string,
  errors: string[],
  ipAddress?: string,
  username?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.VALIDATION_FAILED,
    severity: SeverityLevel.LOW,
    endpoint,
    ipAddress,
    username,
    message: `Input validation failed for ${endpoint}`,
    metadata: { errors },
  });
}

export function logSuspiciousInput(
  endpoint: string,
  field: string,
  value: string,
  ipAddress?: string,
  username?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.SUSPICIOUS_INPUT,
    severity: SeverityLevel.HIGH,
    endpoint,
    ipAddress,
    username,
    message: `Suspicious input detected in field '${field}' at ${endpoint}`,
    metadata: { field, valueSample: value.substring(0, 50) },
  });
}

export function logDeviceAuthFailed(deviceId: string, ipAddress?: string) {
  securityLogger.log({
    eventType: SecurityEventType.DEVICE_AUTH_FAILED,
    severity: SeverityLevel.MEDIUM,
    deviceId,
    ipAddress,
    message: `Device authentication failed for device ${deviceId}`,
  });
}

export function logPasswordChange(username: string, ipAddress?: string) {
  securityLogger.log({
    eventType: SecurityEventType.PASSWORD_CHANGE,
    severity: SeverityLevel.MEDIUM,
    username,
    ipAddress,
    message: `Password changed for user ${username}`,
  });
}

export function logApiKeyCreated(
  keyId: string,
  createdBy: string,
  ipAddress?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.API_KEY_CREATED,
    severity: SeverityLevel.MEDIUM,
    username: createdBy,
    ipAddress,
    message: `New API key created by ${createdBy}`,
    metadata: { keyId },
  });
}

export function logApiKeyRevoked(
  keyId: string,
  revokedBy: string,
  ipAddress?: string
) {
  securityLogger.log({
    eventType: SecurityEventType.API_KEY_REVOKED,
    severity: SeverityLevel.MEDIUM,
    username: revokedBy,
    ipAddress,
    message: `API key revoked by ${revokedBy}`,
    metadata: { keyId },
  });
}

// ============================================================================
// SANITIZATION FOR LOGGING
// ============================================================================

export function sanitizeForLog(value: any): string {
  if (value === null || value === undefined) {
    return "null";
  }

  const str = String(value);

  // Remove control characters and limit length
  const sanitized = str.replace(/[\x00-\x1F\x7F]/g, "").substring(0, 200);

  return sanitized;
}

export function maskSensitiveData(data: any): any {
  if (typeof data !== "object" || data === null) {
    return data;
  }

  const masked = { ...data };
  const sensitiveFields = [
    "password",
    "token",
    "apiKey",
    "secret",
    "key",
    "authorization",
  ];

  for (const key of Object.keys(masked)) {
    const lowerKey = key.toLowerCase();
    if (sensitiveFields.some((field) => lowerKey.includes(field))) {
      masked[key] = "***REDACTED***";
    }
  }

  return masked;
}
