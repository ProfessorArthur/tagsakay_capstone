import api from "./api";
import { ref } from "vue";

export interface ScanStats {
  label: string;
  count: number;
}

export interface DeviceStatus {
  id: string;
  name: string;
  lastActive: string;
  status: "online" | "offline";
  location?: string;
}

export interface RfidScanResult {
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
  user?: {
    id: number;
    name: string;
    role: string;
  };
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
      const response = await api.get("/devices/active");
      return response.data;
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
      recentScans.value = response.data.data;
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
      const [weeklyStats, recentScans, devices, rfidCards, users] =
        await Promise.all([
          this.getWeeklyStats(),
          this.getRecentScans(100),
          api.get("/devices"),
          api.get("/rfid"),
          api.get("/users"),
        ]);

      // Calculate today's scans
      const todayStart = new Date();
      todayStart.setHours(0, 0, 0, 0);
      const todayScans = recentScans.filter(
        (scan) =>
          new Date(scan.scanTime) >= todayStart && scan.status === "success"
      ).length;

      // Calculate device statistics
      const allDevices = devices.data || [];
      const activeDevicesData = await this.getConnectedDevices();
      const onlineDevices = activeDevicesData.filter(
        (d) => d.status === "online"
      ).length;

      // Calculate user statistics
      const allUsers = users.data || [];
      const userStats = {
        drivers: allUsers.filter((u: any) => u.role === "driver").length,
        admins: allUsers.filter((u: any) => u.role === "admin").length,
        superadmins: allUsers.filter((u: any) => u.role === "superadmin")
          .length,
        total: allUsers.length,
      };

      return {
        todayScans,
        totalRegisteredCards: rfidCards.data?.length || 0,
        totalDevices: allDevices.length,
        onlineDevices,
        offlineDevices: allDevices.length - onlineDevices,
        totalUsers: allUsers.length,
        userStats,
        weeklyStats,
        recentScansCount: recentScans.length,
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
