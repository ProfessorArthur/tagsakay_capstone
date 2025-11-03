import apiClient from "./api";
import type { ApiResponse } from "./api";

interface LoginCredentials {
  email: string;
  password: string;
}

interface RegisterData {
  name: string;
  email: string;
  password: string;
  role?: string;
  rfidTag?: string;
}

interface User {
  id: number;
  name: string;
  email: string;
  role: string;
  isActive: boolean;
  rfidTag?: string;
  createdAt?: string;
  updatedAt?: string;
}

interface AuthResponse {
  token: string;
  user: User;
  expiresIn?: string;
}

interface PasswordStrengthResult {
  valid: boolean;
  score: number;
  feedback: string[];
}

const authService = {
  /**
   * Login with email and password
   * Handles rate limiting (5 attempts/min) and account lockout (5 failed attempts)
   */
  async login(credentials: LoginCredentials): Promise<AuthResponse> {
    try {
      const response = await apiClient.post("/auth/login", credentials);
      return response.data;
    } catch (error: any) {
      // Enhanced error handling for rate limiting and account lockout
      if (error.response?.status === 429) {
        const data = error.response.data as ApiResponse;
        throw new Error(
          data.message || "Too many login attempts. Please try again later."
        );
      }

      if (error.response?.status === 403) {
        const data = error.response.data as ApiResponse;
        throw new Error(
          data.message || "Account is locked. Please try again later."
        );
      }

      // Generic error message for failed login (security best practice)
      throw new Error(error.message || "Invalid email or password");
    }
  },

  /**
   * Register new user with password strength validation
   */
  async register(userData: RegisterData): Promise<AuthResponse> {
    try {
      const response = await apiClient.post("/auth/register", userData);
      return response.data;
    } catch (error: any) {
      // Handle validation errors
      if (error.response?.status === 400) {
        const data = error.response.data as ApiResponse;
        if (data.errors && data.errors.length > 0) {
          throw new Error(data.errors.join(", "));
        }
      }

      throw new Error(error.message || "Registration failed");
    }
  },

  /**
   * Validate password strength (client-side check before submission)
   * Matches backend OWASP requirements
   */
  validatePasswordStrength(password: string): PasswordStrengthResult {
    const feedback: string[] = [];
    let score = 0;

    // Length check
    if (password.length < 8) {
      feedback.push("Password must be at least 8 characters");
      return { valid: false, score: 0, feedback };
    }

    if (password.length >= 8) score++;
    if (password.length >= 12) score++;

    // Complexity checks
    if (/[a-z]/.test(password) && /[A-Z]/.test(password)) score++;
    if (/\d/.test(password)) score++;
    if (/[^a-zA-Z0-9]/.test(password)) score++;

    // Common patterns (weak passwords)
    const weakPatterns = [
      /^[0-9]+$/,
      /^[a-zA-Z]+$/,
      /password/i,
      /admin/i,
      /123456/,
      /qwerty/i,
    ];

    if (weakPatterns.some((pattern) => pattern.test(password))) {
      score = Math.max(0, score - 2);
      feedback.push("Avoid common patterns and words");
    }

    // Feedback based on score
    if (score === 0) {
      feedback.push("Very weak password");
    } else if (score === 1) {
      feedback.push("Weak password - add more variety");
    } else if (score === 2) {
      feedback.push("Fair password - consider adding symbols");
    } else if (score === 3) {
      feedback.push("Good password");
    } else if (score >= 4) {
      feedback.push("Strong password");
    }

    return {
      valid: password.length >= 8,
      score,
      feedback,
    };
  },

  /**
   * Refresh JWT token (token expires in 4 hours)
   */
  async refreshToken(): Promise<AuthResponse> {
    const response = await apiClient.post("/auth/refresh");
    return response.data;
  },

  /**
   * Logout and clear all session data
   */
  async logout(): Promise<void> {
    try {
      // Call logout endpoint (will be used for token blacklisting)
      await apiClient.post("/auth/logout");
    } catch (error) {
      // Continue with local cleanup even if API call fails
      console.error("Logout API error:", error);
    } finally {
      this.clearSession();
    }
  },

  /**
   * Clear session data (local)
   */
  clearSession(): void {
    localStorage.removeItem("token");
    localStorage.removeItem("user");

    // Dispatch storage event to notify all components
    window.dispatchEvent(new Event("storage"));

    // Clear any session cookies
    document.cookie.split(";").forEach(function (c) {
      document.cookie = c
        .replace(/^ +/, "")
        .replace(/=.*/, "=;expires=" + new Date().toUTCString() + ";path=/");
    });
  },

  getUser(): User | null {
    const userStr = localStorage.getItem("user");
    return userStr ? JSON.parse(userStr) : null;
  },

  isLoggedIn(): boolean {
    return !!localStorage.getItem("token");
  },

  isAdmin(): boolean {
    const user = this.getUser();
    return user?.role === "admin" || user?.role === "superadmin";
  },

  isSuperAdmin(): boolean {
    const user = this.getUser();
    return user?.role === "superadmin";
  },

  saveUserData(data: AuthResponse): void {
    localStorage.setItem("token", data.token);
    localStorage.setItem("user", JSON.stringify(data.user));

    // Store token expiration time (4 hours from now)
    const expiresAt = Date.now() + 4 * 60 * 60 * 1000;
    localStorage.setItem("tokenExpiresAt", expiresAt.toString());
  },

  /**
   * Check if token is expired or will expire soon (within 5 minutes)
   */
  shouldRefreshToken(): boolean {
    const expiresAtStr = localStorage.getItem("tokenExpiresAt");
    if (!expiresAtStr) return true;

    const expiresAt = parseInt(expiresAtStr);
    const fiveMinutes = 5 * 60 * 1000;

    return Date.now() >= expiresAt - fiveMinutes;
  },
};

export default authService;
export type {
  User,
  LoginCredentials,
  RegisterData,
  AuthResponse,
  PasswordStrengthResult,
};
