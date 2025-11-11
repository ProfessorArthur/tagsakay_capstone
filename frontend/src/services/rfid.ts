import apiClient from "./api";
import type { ApiResponse } from "./api";

interface RfidUserSummary {
  id: number;
  name: string;
  email?: string;
  role: string;
}

interface Rfid {
  id: string;
  tagId: string;
  userId: number | null;
  user?: RfidUserSummary | null;
  isActive: boolean;
  lastScanned: string | null;
  lastSeen?: string | null;
  deviceId: string | null;
  registeredBy?: number | null;
  metadata: Record<string, any>;
  createdAt: string;
  updatedAt: string;
  isRegistered?: boolean;
}

interface RfidScan {
  id: string;
  rfidTagId: string;
  deviceId: string | null;
  userId: number | null;
  eventType: "entry" | "exit" | "unknown";
  location: string | null;
  vehicleId: string | null;
  scanTime: string;
  status: "success" | "failed" | "unauthorized";
  metadata: Record<string, any>;
}

interface UnregisteredRfidScan {
  id: string;
  tagId: string;
  lastSeen: string;
  deviceId: string | null;
  location: string | null;
  scanCount: number;
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
  async getUnregisteredCards(): Promise<Rfid[]> {
    try {
      const response = await apiClient.get("/rfid/unregistered");
      const payload = response.data;

      if (Array.isArray(payload?.rfids)) {
        return payload.rfids;
      }

      if (Array.isArray(payload?.unregisteredTags)) {
        return payload.unregisteredTags;
      }

      if (Array.isArray(payload?.cards)) {
        return payload.cards;
      }

      if (Array.isArray(payload)) {
        return payload;
      }

      return [];
    } catch (error: any) {
      console.error("Failed to fetch unregistered cards:", error);
      return [];
    }
  },

  /**
   * Get all registered RFID cards
   */
  async getAllRfidCards(): Promise<Rfid[]> {
    try {
      const response = await apiClient.get("/rfid");
      const payload = response.data;
      const rfids = Array.isArray(payload?.rfids)
        ? payload.rfids
        : Array.isArray(payload)
        ? payload
        : [];

      return rfids.map((raw: any) => ({
        ...raw,
        tagId: typeof raw?.tagId === "string" ? raw.tagId.toUpperCase() : "",
        isRegistered: Boolean(raw?.user?.id),
        lastSeen: raw?.lastScanned ?? raw?.updatedAt ?? null,
        metadata: raw?.metadata ?? {},
      }));
    } catch (error: any) {
      console.error("Failed to fetch all RFID cards:", error);
      return [];
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
  async getRecentUnregisteredScans(): Promise<
    ApiResponse<UnregisteredRfidScan[]>
  > {
    try {
      const response = await apiClient.get("/rfid/scans/unregistered");
      const payload = response.data;
      const tags = Array.isArray(payload?.unregisteredTags)
        ? payload.unregisteredTags
        : Array.isArray(payload)
        ? payload
        : [];
      return {
        success: true,
        data: tags,
      };
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
export type { Rfid, RfidScan, UnregisteredRfidScan, RegisterRfidData };
