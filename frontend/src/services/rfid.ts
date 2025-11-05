import apiClient from "./api";
import type { ApiResponse } from "./api";

interface Rfid {
  id: string;
  tagId: string;
  userId: number | null;
  isActive: boolean;
  lastScanned: string | null;
  deviceId: string | null;
  registeredBy: number;
  metadata: Record<string, any>;
  createdAt: string;
  updatedAt: string;
}

interface RfidScan {
  id: string;
  rfidTagId: string;
  deviceId: string;
  userId: number | null;
  eventType: "entry" | "exit" | "unknown";
  location: string | null;
  vehicleId: string | null;
  scanTime: string;
  status: "success" | "failed" | "unauthorized";
  metadata: Record<string, any>;
}

interface RegisterRfidData {
  tagId: string;
  userId?: number;
  metadata?: Record<string, any>;
}

/**
 * Client-side RFID tag validation (matches backend: 4-32 alphanumeric)
 */
const validateRfidTag = (tagId: string): { valid: boolean; error?: string } => {
  if (!tagId || tagId.length === 0) {
    return { valid: false, error: "RFID tag is required" };
  }

  if (tagId.length < 4 || tagId.length > 32) {
    return {
      valid: false,
      error: "RFID tag must be between 4 and 32 characters",
    };
  }

  // Alphanumeric only
  const alphanumericRegex = /^[a-zA-Z0-9]+$/;
  if (!alphanumericRegex.test(tagId)) {
    return {
      valid: false,
      error: "RFID tag must contain only letters and numbers",
    };
  }

  return { valid: true };
};

const rfidService = {
  /**
   * Register a new RFID tag
   */
  async registerRfid(data: RegisterRfidData): Promise<Rfid> {
    // Client-side validation
    const validation = validateRfidTag(data.tagId);
    if (!validation.valid) {
      throw new Error(validation.error);
    }

    try {
      const response = await apiClient.post("/rfid/register", data);
      return response.data;
    } catch (error: any) {
      if (error.response?.status === 400) {
        const apiResponse = error.response.data as ApiResponse;
        if (apiResponse.errors && apiResponse.errors.length > 0) {
          throw new Error(apiResponse.errors.join(", "));
        }
      }
      throw new Error(error.message || "Failed to register RFID tag");
    }
  },

  /**
   * Get RFID tag information
   */
  async getRfidInfo(id: string): Promise<Rfid> {
    try {
      const response = await apiClient.get(`/rfid/${id}`);
      return response.data;
    } catch (error: any) {
      throw new Error(error.message || "Failed to fetch RFID info");
    }
  },

  /**
   * Update RFID tag status (activate/deactivate)
   */
  async updateRfidStatus(id: string, isActive: boolean): Promise<Rfid> {
    try {
      const response = await apiClient.put(`/rfid/${id}/status`, { isActive });
      return response.data;
    } catch (error: any) {
      throw new Error(error.message || "Failed to update RFID status");
    }
  },

  /**
   * Get unregistered RFID cards (scanned but not registered)
   */
  async getUnregisteredCards(): Promise<ApiResponse<Rfid[]>> {
    try {
      const response = await apiClient.get("/rfid/unregistered");
      return response.data || { success: false, data: [] };
    } catch (error: any) {
      console.error("Failed to fetch unregistered cards:", error);
      return { success: false, data: [] }; // Return empty array on error
    }
  },

  /**
   * Get all registered RFID cards
   */
  async getAllRfidCards(): Promise<ApiResponse<Rfid[]>> {
    try {
      const response = await apiClient.get("/rfid");
      return response.data || { success: false, data: [] };
    } catch (error: any) {
      console.error("Failed to fetch all RFID cards:", error);
      return { success: false, data: [] }; // Return empty array on error
    }
  },

  /**
   * Check if a tag was recently scanned
   */
  async checkRecentScan(tagId: string): Promise<any> {
    const validation = validateRfidTag(tagId);
    if (!validation.valid) {
      throw new Error(validation.error);
    }

    try {
      const response = await apiClient.get(`/rfid/check-recent-scan/${tagId}`);
      return response.data;
    } catch (error: any) {
      throw new Error(error.message || "Failed to check recent scan");
    }
  },

  /**
   * Get recent unregistered scans (for registration workflow)
   */
  async getRecentUnregisteredScans(): Promise<ApiResponse<RfidScan[]>> {
    try {
      const response = await apiClient.get("/rfid/scans/unregistered");
      return response.data || { success: false, data: [] };
    } catch (error: any) {
      console.error("Failed to fetch recent unregistered scans:", error);
      return { success: false, data: [] }; // Return empty array on error to prevent component crashes
    }
  },

  /**
   * Validate RFID tag format (client-side helper)
   */
  validateRfidTag,
};

export default rfidService;
export type { Rfid, RfidScan, RegisterRfidData };
