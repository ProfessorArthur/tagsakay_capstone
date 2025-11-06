import api from "./api";
import type { ApiResponse } from "./api";

export interface Device {
  id: number;
  deviceId: string;
  macAddress: string;
  name: string;
  location: string;
  isActive: boolean;
  registrationMode: boolean;
  scanMode: boolean;
  pendingRegistrationTagId?: string;
  lastSeen?: string;
  createdAt?: string;
  updatedAt?: string;
}

export interface RegisterDeviceRequest {
  macAddress: string;
  name: string;
  location: string;
}

export interface RegisterDeviceResponse {
  device: Device;
  apiKey: string;
}

export interface UpdateDeviceStatusRequest {
  isActive?: boolean;
  registrationMode?: boolean;
  pendingRegistrationTagId?: string;
  scanMode?: boolean;
}

/**
 * Client-side MAC address validation (matches backend: XX:XX:XX:XX:XX:XX or XXXXXXXXXXXX)
 */
const validateMacAddress = (
  mac: string
): { valid: boolean; error?: string } => {
  if (!mac || mac.length === 0) {
    return { valid: false, error: "MAC address is required" };
  }

  // Format 1: XX:XX:XX:XX:XX:XX (with colons)
  const macWithColonsRegex = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/;

  // Format 2: XXXXXXXXXXXX (without colons)
  const macWithoutColonsRegex = /^[0-9A-Fa-f]{12}$/;

  if (!macWithColonsRegex.test(mac) && !macWithoutColonsRegex.test(mac)) {
    return {
      valid: false,
      error:
        "Invalid MAC address format. Use XX:XX:XX:XX:XX:XX or XXXXXXXXXXXX",
    };
  }

  return { valid: true };
};

/**
 * Validate device name
 */
const validateDeviceName = (
  name: string
): { valid: boolean; error?: string } => {
  if (!name || name.trim().length === 0) {
    return { valid: false, error: "Device name is required" };
  }

  if (name.length < 2 || name.length > 100) {
    return {
      valid: false,
      error: "Device name must be between 2 and 100 characters",
    };
  }

  return { valid: true };
};

/**
 * Validate device location
 */
const validateLocation = (
  location: string
): { valid: boolean; error?: string } => {
  if (!location || location.trim().length === 0) {
    return { valid: false, error: "Device location is required" };
  }

  if (location.length > 200) {
    return { valid: false, error: "Location must be less than 200 characters" };
  }

  return { valid: true };
};

const deviceService = {
  /**
   * Register a new device with its MAC address
   */
  registerDevice: async (
    deviceData: RegisterDeviceRequest
  ): Promise<RegisterDeviceResponse> => {
    // Client-side validation
    const macValidation = validateMacAddress(deviceData.macAddress);
    if (!macValidation.valid) {
      throw new Error(macValidation.error);
    }

    const nameValidation = validateDeviceName(deviceData.name);
    if (!nameValidation.valid) {
      throw new Error(nameValidation.error);
    }

    const locationValidation = validateLocation(deviceData.location);
    if (!locationValidation.valid) {
      throw new Error(locationValidation.error);
    }

    try {
      const response = await api.post("/devices/register", deviceData);
      return response.data as RegisterDeviceResponse;
    } catch (error: any) {
      if (error.response?.status === 400) {
        const apiResponse = error.response.data as ApiResponse;
        if (apiResponse.errors && apiResponse.errors.length > 0) {
          throw new Error(apiResponse.errors.join(", "));
        }
      }
      if (error.response?.status === 429) {
        throw new Error(
          "Too many registration attempts. Please try again later."
        );
      }
      throw new Error(error.message || "Failed to register device");
    }
  },

  /**
   * Get active devices that are online
   */
  getActiveDevices: async (): Promise<Device[]> => {
    try {
      const response = await api.get("/devices/active");
      return response.data?.devices || [];
    } catch (error: any) {
      console.error("Failed to fetch active devices:", error);
      return [];
    }
  },

  /**
   * Get all registered devices
   */
  getAllDevices: async (): Promise<Device[]> => {
    try {
      const response = await api.get("/devices");
      return response.data?.devices || [];
    } catch (error: any) {
      console.error("Failed to fetch all devices:", error);
      return [];
    }
  },

  /**
   * Update device status (enable/disable)
   */
  updateDeviceStatus: async (
    deviceId: string,
    statusData: UpdateDeviceStatusRequest
  ): Promise<Partial<Device>> => {
    try {
      const response = await api.put(`/devices/${deviceId}`, statusData);
      const device = response.data?.device as Partial<Device> | undefined;
      if (!device) {
        throw new Error("Device payload missing from update response");
      }
      return device;
    } catch (error: any) {
      throw new Error(error.message || "Failed to update device status");
    }
  },

  /**
   * Delete a device
   */
  deleteDevice: async (deviceId: string): Promise<void> => {
    try {
      await api.delete(`/devices/${deviceId}`);
    } catch (error: any) {
      throw new Error(error.message || "Failed to delete device");
    }
  },

  /**
   * Enable registration mode for a device
   */
  enableRegistrationMode: async (
    deviceId: string,
    tagId?: string
  ): Promise<Partial<Device>> => {
    try {
      const response = await api.post(`/devices/${deviceId}/mode`, {
        registrationMode: true,
        pendingRegistrationTagId: tagId || "",
        scanMode: !tagId,
      });
      const device = response.data?.device as Partial<Device> | undefined;
      if (!device) {
        throw new Error("Device payload missing from enable response");
      }
      return device;
    } catch (error: any) {
      throw new Error(error.message || "Failed to enable registration mode");
    }
  },

  /**
   * Disable registration mode for a device
   */
  disableRegistrationMode: async (
    deviceId: string
  ): Promise<Partial<Device>> => {
    try {
      const response = await api.post(`/devices/${deviceId}/mode`, {
        registrationMode: false,
        pendingRegistrationTagId: "",
      });
      const device = response.data?.device as Partial<Device> | undefined;
      if (!device) {
        throw new Error("Device payload missing from disable response");
      }
      return device;
    } catch (error: any) {
      throw new Error(error.message || "Failed to disable registration mode");
    }
  },

  /**
   * Set registration mode for a device with more control
   */
  setRegistrationMode: async ({
    deviceId,
    enabled,
    tagId,
  }: {
    deviceId: string;
    enabled: boolean;
    tagId?: string;
  }): Promise<Partial<Device>> => {
    try {
      const response = await api.post(`/devices/${deviceId}/mode`, {
        registrationMode: enabled,
        pendingRegistrationTagId: tagId || "",
        scanMode: !tagId && enabled,
      });
      const device = response.data?.device as Partial<Device> | undefined;
      if (!device) {
        throw new Error("Device payload missing from mode response");
      }
      return device;
    } catch (error: any) {
      throw new Error(error.message || "Failed to set registration mode");
    }
  },

  // Export validation helpers
  validateMacAddress,
  validateDeviceName,
  validateLocation,
};

export default deviceService;
