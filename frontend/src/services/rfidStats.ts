import api from "./api";
import { ref } from "vue";
import deviceService, { type Device } from "./device";
import rfidService from "./rfid";
import userService from "./user";

export interface ScanStats {
  label: string;
  count: number;
}

export interface DeviceStatus {
  id: string;
  name: string;
  lastActive: string | null;
  status: "online" | "offline";
  location?: string;
  lastSeenAgoSeconds?: number | null;
}

export interface RfidScanResult {
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
  user?: {
    id: number;
    name: string;
    role: string;
    isActive?: boolean;
  } | null;
}

const recentScans = ref<RfidScanResult[]>([]);
const loading = ref(false);
const error = ref<string | null>(null);

// Polling interval in milliseconds
const DEFAULT_POLLING_INTERVAL = 5000;
let pollingInterval: number | null = null;

const rfidStatsService = {
  /**
   * Get RFID scan statistics by day for the last 7 days
   */
  async getWeeklyStats(): Promise<ScanStats[]> {
    try {
      const response = await api.get("/rfid/stats/weekly");
      return response.data;
    } catch (error) {
      console.error("Error fetching weekly RFID stats:", error);
      return [];
    }
  },

  /**
   * Get RFID scan statistics by month for the last 6 months
   */
  async getMonthlyStats(): Promise<ScanStats[]> {
    try {
      const response = await api.get("/rfid/stats/monthly");
      return response.data;
    } catch (error) {
      console.error("Error fetching monthly RFID stats:", error);
      return [];
    }
  },

  /**
   * Get connected RFID devices status
   * Shows which ESP32 devices are currently connected
   */
  async getConnectedDevices(): Promise<DeviceStatus[]> {
    try {
      const devices = await deviceService.getAllDevices();

      const mappedDevices = devices.map(
        (device: Device): DeviceStatus => ({
          id: device.deviceId,
          name: device.name,
          status: device.status === "online" ? "online" : "offline",
          lastActive: device.lastSeen ?? device.updatedAt ?? null,
          location: device.location,
          lastSeenAgoSeconds: device.lastSeenAgoSeconds ?? null,
        })
      );

      return mappedDevices.sort((a, b) => {
        if (a.status !== b.status) {
          return a.status === "online" ? -1 : 1;
        }

        const aTime = a.lastActive ? new Date(a.lastActive).getTime() : 0;
        const bTime = b.lastActive ? new Date(b.lastActive).getTime() : 0;
        return bTime - aTime;
      });
    } catch (error) {
      console.error("Error fetching connected RFID devices:", error);
      return []; // Return empty array if error occurs
    }
  },

  /**
   * Get recent RFID scans
   * Shows the most recent scans from all devices
   */
  async getRecentScans(limit: number = 10): Promise<RfidScanResult[]> {
    try {
      loading.value = true;
      error.value = null;
      const response = await api.get(`/rfid/scans/recent?limit=${limit}`);
      const payload = response.data;
      const scans = Array.isArray(payload?.scans)
        ? payload.scans
        : Array.isArray(payload)
        ? payload
        : [];

      recentScans.value = scans;
      return recentScans.value;
    } catch (err: any) {
      error.value =
        err.response?.data?.message || "Failed to fetch recent scans";
      console.error("Error fetching recent RFID scans:", err);
      return [];
    } finally {
      loading.value = false;
    }
  },

  /**
   * Start polling for recent RFID scans
   * Automatically updates recentScans ref at the given interval
   */
  startPolling(interval: number = DEFAULT_POLLING_INTERVAL) {
    // Clear any existing polling
    if (pollingInterval) {
      clearInterval(pollingInterval);
    }

    // Get initial data
    this.getRecentScans();

    // Set up polling interval
    pollingInterval = setInterval(() => {
      this.getRecentScans();
    }, interval) as unknown as number;
  },

  /**
   * Stop polling for RFID scans
   */
  stopPolling() {
    if (pollingInterval) {
      clearInterval(pollingInterval);
      pollingInterval = null;
    }
  },

  /**
   * Get comprehensive dashboard statistics
   */
  async getDashboardStats() {
    try {
      const [
        weeklyStats,
        recentScanList,
        deviceList,
        rfidCardList,
        usersResponse,
      ] = await Promise.all([
        this.getWeeklyStats(),
        this.getRecentScans(100),
        deviceService.getAllDevices(),
        rfidService.getAllRfidCards(),
        userService.getUsers(),
      ]);

      const todayStart = new Date();
      todayStart.setHours(0, 0, 0, 0);
      const successfulScans = Array.isArray(recentScanList)
        ? recentScanList.filter(
            (scan) =>
              new Date(scan.scanTime) >= todayStart && scan.status === "success"
          )
        : [];

      const onlineDevices = deviceList.filter(
        (device) => device.status === "online"
      ).length;

      const allUsers = Array.isArray(usersResponse) ? usersResponse : [];

      const userStats = {
        drivers: allUsers.filter((u: any) => u.role === "driver").length,
        admins: allUsers.filter((u: any) => u.role === "admin").length,
        superadmins: allUsers.filter((u: any) => u.role === "superadmin")
          .length,
        total: allUsers.length,
      };

      return {
        todayScans: successfulScans.length,
        totalRegisteredCards: rfidCardList.length,
        totalDevices: deviceList.length,
        onlineDevices,
        offlineDevices: deviceList.length - onlineDevices,
        totalUsers: allUsers.length,
        userStats,
        weeklyStats,
        recentScansCount: Array.isArray(recentScanList)
          ? recentScanList.length
          : 0,
      };
    } catch (error) {
      console.error("Error fetching dashboard statistics:", error);
      return {
        todayScans: 0,
        totalRegisteredCards: 0,
        totalDevices: 0,
        onlineDevices: 0,
        offlineDevices: 0,
        totalUsers: 0,
        userStats: { drivers: 0, admins: 0, superadmins: 0, total: 0 },
        weeklyStats: [],
        recentScansCount: 0,
      };
    }
  },

  // Expose reactive references
  recentScans,
  loading,
  error,
};

export default rfidStatsService;
