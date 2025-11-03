// ============================================================================
// INPUT VALIDATION LIBRARY - OWASP Compliant
// ============================================================================
// Implements allowlist-based validation for all user inputs
// Prevents injection attacks, XSS, and data corruption

// ============================================================================
// EMAIL VALIDATION (RFC 5321 Compliant)
// ============================================================================

export function validateEmail(email: string): {
  valid: boolean;
  error?: string;
} {
  if (!email || typeof email !== "string") {
    return { valid: false, error: "Email is required" };
  }

  // Basic length checks
  if (email.length > 254) {
    return { valid: false, error: "Email exceeds maximum length" };
  }

  // Must contain exactly one @ symbol
  const atCount = (email.match(/@/g) || []).length;
  if (atCount !== 1) {
    return { valid: false, error: "Invalid email format" };
  }

  const [local, domain] = email.split("@");

  // Local part validation
  if (!local || local.length > 64) {
    return { valid: false, error: "Invalid email format" };
  }

  // Domain part validation (letters, numbers, hyphens, periods)
  const domainRegex = /^[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/;
  if (!domain || !domainRegex.test(domain)) {
    return { valid: false, error: "Invalid email domain" };
  }

  // Check for dangerous characters
  const dangerousChars = /<|>|"|'|`|\0/;
  if (dangerousChars.test(email)) {
    return { valid: false, error: "Email contains invalid characters" };
  }

  // Basic email regex (not perfect but practical)
  const emailRegex =
    /^[a-zA-Z0-9.!#$%&'*+/=?^_`{|}~-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;

  if (!emailRegex.test(email)) {
    return { valid: false, error: "Invalid email format" };
  }

  return { valid: true };
}

// ============================================================================
// RFID TAG VALIDATION
// ============================================================================

export function validateRfidTag(tagId: string): {
  valid: boolean;
  error?: string;
} {
  if (!tagId || typeof tagId !== "string") {
    return { valid: false, error: "RFID tag is required" };
  }

  // Length check (typical RFID tags are 8-24 characters)
  if (tagId.length < 4 || tagId.length > 32) {
    return { valid: false, error: "RFID tag must be 4-32 characters" };
  }

  // Alphanumeric only (allowlist approach)
  const rfidRegex = /^[A-Za-z0-9]+$/;
  if (!rfidRegex.test(tagId)) {
    return {
      valid: false,
      error: "RFID tag must contain only letters and numbers",
    };
  }

  return { valid: true };
}

// ============================================================================
// MAC ADDRESS VALIDATION
// ============================================================================

export function validateMacAddress(mac: string): {
  valid: boolean;
  error?: string;
} {
  if (!mac || typeof mac !== "string") {
    return { valid: false, error: "MAC address is required" };
  }

  // Support both formats: XX:XX:XX:XX:XX:XX and XXXXXXXXXXXX
  const macWithColons = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/;
  const macWithoutColons = /^[0-9A-Fa-f]{12}$/;

  if (!macWithColons.test(mac) && !macWithoutColons.test(mac)) {
    return { valid: false, error: "Invalid MAC address format" };
  }

  return { valid: true };
}

// ============================================================================
// STRING VALIDATION (General Purpose)
// ============================================================================

export interface StringValidationOptions {
  minLength?: number;
  maxLength?: number;
  pattern?: RegExp;
  allowUnicode?: boolean;
  fieldName?: string;
}

export function validateString(
  value: string,
  options: StringValidationOptions
): { valid: boolean; error?: string } {
  if (value === undefined || value === null) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} is required`,
    };
  }

  if (typeof value !== "string") {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must be a string`,
    };
  }

  // Length validation
  if (options.minLength !== undefined && value.length < options.minLength) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must be at least ${
        options.minLength
      } characters`,
    };
  }

  if (options.maxLength !== undefined && value.length > options.maxLength) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must not exceed ${
        options.maxLength
      } characters`,
    };
  }

  // Pattern validation
  if (options.pattern && !options.pattern.test(value)) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} contains invalid characters`,
    };
  }

  // Unicode validation (if not allowed)
  if (!options.allowUnicode) {
    const asciiOnly = /^[\x20-\x7E]*$/;
    if (!asciiOnly.test(value)) {
      return {
        valid: false,
        error: `${options.fieldName || "Field"} contains non-ASCII characters`,
      };
    }
  }

  return { valid: true };
}

// ============================================================================
// NUMBER VALIDATION
// ============================================================================

export interface NumberValidationOptions {
  min?: number;
  max?: number;
  integer?: boolean;
  fieldName?: string;
}

export function validateNumber(
  value: any,
  options: NumberValidationOptions
): { valid: boolean; error?: string; value?: number } {
  const num = typeof value === "string" ? parseFloat(value) : value;

  if (typeof num !== "number" || isNaN(num)) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must be a number`,
    };
  }

  // Integer check
  if (options.integer && !Number.isInteger(num)) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must be an integer`,
    };
  }

  // Range validation
  if (options.min !== undefined && num < options.min) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must be at least ${options.min}`,
    };
  }

  if (options.max !== undefined && num > options.max) {
    return {
      valid: false,
      error: `${options.fieldName || "Field"} must not exceed ${options.max}`,
    };
  }

  return { valid: true, value: num };
}

// ============================================================================
// ENUM VALIDATION (Allowlist)
// ============================================================================

export function validateEnum<T extends string>(
  value: string,
  allowedValues: readonly T[],
  fieldName?: string
): { valid: boolean; error?: string; value?: T } {
  if (!value || typeof value !== "string") {
    return { valid: false, error: `${fieldName || "Field"} is required` };
  }

  if (!allowedValues.includes(value as T)) {
    return {
      valid: false,
      error: `${fieldName || "Field"} must be one of: ${allowedValues.join(
        ", "
      )}`,
    };
  }

  return { valid: true, value: value as T };
}

// ============================================================================
// SANITIZATION HELPERS
// ============================================================================

export function sanitizeString(value: string): string {
  if (!value) return "";

  // Remove null bytes
  let sanitized = value.replace(/\0/g, "");

  // Trim whitespace
  sanitized = sanitized.trim();

  // Normalize unicode (NFC normalization)
  sanitized = sanitized.normalize("NFC");

  return sanitized;
}

export function sanitizeForLog(value: any): string {
  if (value === null || value === undefined) return "null";

  const str = String(value);

  // Remove control characters for safe logging
  return str.replace(/[\x00-\x1F\x7F]/g, "");
}

// ============================================================================
// REQUEST BODY VALIDATION
// ============================================================================

export interface ValidationSchema {
  [key: string]: {
    type: "string" | "number" | "boolean" | "email" | "rfid" | "mac" | "enum";
    required?: boolean;
    minLength?: number;
    maxLength?: number;
    min?: number;
    max?: number;
    pattern?: RegExp;
    allowedValues?: readonly string[];
  };
}

export function validateRequestBody(
  body: any,
  schema: ValidationSchema
): { valid: boolean; errors: string[]; sanitized?: any } {
  const errors: string[] = [];
  const sanitized: any = {};

  // Check for required fields
  for (const [field, rules] of Object.entries(schema)) {
    const value = body[field];

    if (
      rules.required &&
      (value === undefined || value === null || value === "")
    ) {
      errors.push(`${field} is required`);
      continue;
    }

    if (value === undefined || value === null) {
      continue; // Skip validation for optional missing fields
    }

    // Type-specific validation
    let result;
    switch (rules.type) {
      case "email":
        result = validateEmail(value);
        if (!result.valid) errors.push(`${field}: ${result.error}`);
        else sanitized[field] = sanitizeString(value);
        break;

      case "rfid":
        result = validateRfidTag(value);
        if (!result.valid) errors.push(`${field}: ${result.error}`);
        else sanitized[field] = value.toUpperCase();
        break;

      case "mac":
        result = validateMacAddress(value);
        if (!result.valid) errors.push(`${field}: ${result.error}`);
        else sanitized[field] = value;
        break;

      case "enum":
        result = validateEnum(value, rules.allowedValues || [], field);
        if (!result.valid) errors.push(`${field}: ${result.error}`);
        else sanitized[field] = result.value;
        break;

      case "string":
        result = validateString(value, {
          minLength: rules.minLength,
          maxLength: rules.maxLength,
          pattern: rules.pattern,
          fieldName: field,
        });
        if (!result.valid) errors.push(`${field}: ${result.error}`);
        else sanitized[field] = sanitizeString(value);
        break;

      case "number":
        result = validateNumber(value, {
          min: rules.min,
          max: rules.max,
          fieldName: field,
        });
        if (!result.valid) errors.push(`${field}: ${result.error}`);
        else sanitized[field] = result.value;
        break;

      case "boolean":
        if (typeof value !== "boolean") {
          errors.push(`${field} must be a boolean`);
        } else {
          sanitized[field] = value;
        }
        break;
    }
  }

  return {
    valid: errors.length === 0,
    errors,
    sanitized: errors.length === 0 ? sanitized : undefined,
  };
}
