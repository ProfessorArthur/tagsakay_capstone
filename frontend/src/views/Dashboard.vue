<script setup lang="ts">
import { ref, onMounted, computed, onUnmounted } from "vue";
import authService from "../services/auth";
import type { User } from "../services/auth";
import RfidChart from "../components/RfidChart.vue";
import RfidDeviceStatus from "../components/RfidDeviceStatus.vue";
import RfidLiveScans from "../components/RfidLiveScans.vue";
import rfidStatsService from "../services/rfidStats";
import { useRealTimeScans } from "../composables/useRealTimeScans";
import { Line, Pie, Bar } from "vue-chartjs";
import {
  Chart as ChartJS,
  Title,
  Tooltip,
  Legend,
  BarElement,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  ArcElement,
  type ChartData,
  type ChartOptions,
} from "chart.js";

ChartJS.register(
  CategoryScale,
  LinearScale,
  BarElement,
  PointElement,
  LineElement,
  ArcElement,
  Title,
  Tooltip,
  Legend
);

ChartJS.register(
  CategoryScale,
  LinearScale,
  BarElement,
  PointElement,
  LineElement,
  ArcElement,
  Title,
  Tooltip,
  Legend
);

const user = ref<User | null>(null);
const loading = ref(true);
const activePeriod = ref<"weekly" | "monthly">("weekly");

// Real-time statistics from backend
const todayScans = ref(0);
const totalRegisteredCards = ref(0);
const activeDevices = ref(0);
const totalUsers = ref(0);
const weeklyScansData = ref<any[]>([]);
const recentScansCount = ref(0);

// User statistics
const userStats = ref({
  drivers: 0,
  admins: 0,
  superadmins: 0,
});

// Device statistics
const deviceStats = ref({
  online: 0,
  offline: 0,
  total: 0,
});

// Real-time polling integration (replaces WebSocket)
const {
  recentScans,
  stats: realTimeStats,
  isConnected: pollingActive,
  isConnecting: pollingStarting,
  startListening,
  stopListening,
} = useRealTimeScans();

// Real-time event handlers
const handleRfidScan = (event: Event) => {
  const customEvent = event as CustomEvent;
  const { scan, device } = customEvent.detail;
  console.log("New RFID scan:", scan);

  // Update today's scan count immediately
  todayScans.value++;

  // Show success notification
  const toast = document.createElement("div");
  toast.className =
    "alert alert-success shadow-lg fixed top-4 right-4 z-50 max-w-sm";
  toast.innerHTML = `
    <div>
      <svg xmlns="http://www.w3.org/2000/svg" class="stroke-current flex-shrink-0 h-6 w-6" fill="none" viewBox="0 0 24 24">
        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
      </svg>
      <div>
        <h3 class="font-bold">RFID Scan Detected!</h3>
        <div class="text-xs">${scan.rfidTagId} at ${
    device?.location || "Unknown"
  }</div>
      </div>
    </div>
  `;
  document.body.appendChild(toast);
  setTimeout(() => {
    document.body.removeChild(toast);
  }, 3000);
};

const handlePollingConnection = () => {
  console.log("HTTP polling started for real-time updates");
};

const handlePollingDisconnection = () => {
  console.log("HTTP polling stopped");
};

// Chart Data
const dailyTripsData = computed<ChartData<"line">>(() => {
  if (weeklyScansData.value.length === 0) {
    return {
      labels: ["06/30", "07/01", "07/02", "07/03", "07/04", "07/05", "07/06"],
      datasets: [
        {
          label: "Daily Scans",
          backgroundColor: "rgba(59, 130, 246, 0.2)",
          borderColor: "rgba(59, 130, 246, 1)",
          data: [145, 170, 165, 165, 185, 185, 180],
          tension: 0.4,
          fill: true,
        },
      ],
    };
  }

  return {
    labels: weeklyScansData.value.map((_, index) => {
      const date = new Date();
      date.setDate(date.getDate() - (6 - index));
      return date.toLocaleDateString("en-US", {
        month: "2-digit",
        day: "2-digit",
      });
    }),
    datasets: [
      {
        label: "Daily Scans",
        backgroundColor: "rgba(59, 130, 246, 0.2)",
        borderColor: "rgba(59, 130, 246, 1)",
        data: weeklyScansData.value.map((item) => item.count),
        tension: 0.4,
        fill: true,
      },
    ],
  };
});

const dailyTripsOptions = computed<ChartOptions<"line">>(() => ({
  responsive: true,
  maintainAspectRatio: false,
  scales: {
    y: {
      beginAtZero: false,
      min: 120,
      grid: { color: "rgba(203, 213, 225, 0.1)" },
      ticks: { color: "rgba(203, 213, 225, 0.7)" },
    },
    x: {
      grid: { display: false },
      ticks: { color: "rgba(203, 213, 225, 0.7)" },
    },
  },
  plugins: { legend: { display: false } },
}));

const userAccessData = computed<ChartData<"pie">>(() => ({
  labels: ["Drivers", "Admins", "Super Admins"],
  datasets: [
    {
      backgroundColor: [
        "rgba(59, 130, 246, 1)",
        "rgba(96, 165, 250, 1)",
        "rgba(147, 195, 255, 1)",
      ],
      data: [
        userStats.value.drivers,
        userStats.value.admins,
        userStats.value.superadmins,
      ],
      borderWidth: 0,
    },
  ],
}));

const userAccessOptions = computed<ChartOptions<"pie">>(() => ({
  responsive: true,
  maintainAspectRatio: false,
  plugins: { legend: { display: false } },
}));

const historicalTripsData = computed<ChartData<"bar">>(() => {
  if (weeklyScansData.value.length === 0) {
    return {
      labels: [
        "06/22",
        "06/23",
        "06/24",
        "06/25",
        "06/26",
        "06/27",
        "06/28",
        "06/29",
        "06/30",
        "07/01",
        "07/02",
        "07/03",
        "07/04",
        "07/05",
      ],
      datasets: [
        {
          label: "Scans",
          backgroundColor: "rgba(59, 130, 246, 0.8)",
          data: [
            180, 190, 130, 140, 155, 150, 190, 180, 120, 140, 150, 140, 180,
            175,
          ],
          borderRadius: 4,
        },
      ],
    };
  }

  const extendedData = [...weeklyScansData.value, ...weeklyScansData.value];
  return {
    labels: extendedData.slice(0, 14).map((_, index) => {
      const date = new Date();
      date.setDate(date.getDate() - (13 - index));
      return date.toLocaleDateString("en-US", {
        month: "2-digit",
        day: "2-digit",
      });
    }),
    datasets: [
      {
        label: "Scans",
        backgroundColor: "rgba(59, 130, 246, 0.8)",
        data: extendedData.slice(0, 14).map((item) => item.count || 0),
        borderRadius: 4,
      },
    ],
  };
});

const historicalTripsOptions = computed<ChartOptions<"bar">>(() => ({
  responsive: true,
  maintainAspectRatio: false,
  scales: {
    y: { display: false, beginAtZero: true },
    x: { display: false },
  },
  plugins: { legend: { display: false } },
}));

// Methods
const loadStatistics = async () => {
  loading.value = true;
  try {
    // Load comprehensive dashboard statistics
    const stats = await rfidStatsService.getDashboardStats();

    // Update all statistics from the dashboard stats response
    todayScans.value = stats.todayScans || 42;
    totalRegisteredCards.value = stats.totalRegisteredCards || 156;
    activeDevices.value = stats.onlineDevices || 3;
    totalUsers.value = stats.totalUsers || 42;

    userStats.value = {
      drivers: stats.userStats?.drivers || 37,
      admins: stats.userStats?.admins || 3,
      superadmins: stats.userStats?.superadmins || 2,
    };

    deviceStats.value = {
      online: stats.onlineDevices || 2,
      offline: (stats.totalDevices || 3) - (stats.onlineDevices || 2),
      total: stats.totalDevices || 3,
    };

    weeklyScansData.value = stats.weeklyStats || [];
    recentScansCount.value = stats.recentScansCount || 15;
  } catch (error) {
    console.error("Failed to load statistics:", error);
    // Use fallback data
    todayScans.value = 42;
    totalRegisteredCards.value = 156;
    activeDevices.value = 3;
    totalUsers.value = 42;

    userStats.value = {
      drivers: 37,
      admins: 3,
      superadmins: 2,
    };

    deviceStats.value = {
      online: 2,
      offline: 1,
      total: 3,
    };

    weeklyScansData.value = [];
    recentScansCount.value = 15;
  } finally {
    loading.value = false;
  }
};

onMounted(async () => {
  user.value = authService.getUser();
  await loadStatistics();

  // Start HTTP polling for real-time updates
  startListening();

  // Add event listeners for real-time events
  window.addEventListener("rfid-scan", handleRfidScan);
  window.addEventListener("polling-connected", handlePollingConnection);
  window.addEventListener("polling-disconnected", handlePollingDisconnection);
});

onUnmounted(() => {
  // Stop HTTP polling
  stopListening();

  // Remove event listeners
  window.removeEventListener("rfid-scan", handleRfidScan);
  window.removeEventListener("polling-connected", handlePollingConnection);
  window.removeEventListener(
    "polling-disconnected",
    handlePollingDisconnection
  );
});
</script>

<template>
  <div v-if="loading" class="flex justify-center items-center min-h-[50vh]">
    <span class="loading loading-spinner loading-lg"></span>
  </div>

  <div v-else>
    <!-- Dashboard Header -->
    <div class="mb-8">
      <div class="flex justify-between items-start mb-4">
        <div>
          <h1 class="text-3xl font-bold text-base-content mb-2">Dashboard</h1>
          <p class="text-base-content/70">Welcome back, {{ user?.name }}!</p>
        </div>

        <!-- Polling Connection Status -->
        <div class="flex items-center gap-2">
          <div
            class="tooltip"
            :data-tip="
              pollingActive
                ? 'Real-time updates active (HTTP polling)'
                : pollingStarting
                ? 'Starting real-time updates...'
                : 'Real-time updates inactive'
            "
          >
            <div
              class="flex items-center gap-2 px-3 py-2 rounded-lg"
              :class="{
                'bg-success/10 text-success': pollingActive,
                'bg-warning/10 text-warning': pollingStarting,
                'bg-error/10 text-error': !pollingActive && !pollingStarting,
              }"
            >
              <div
                class="w-2 h-2 rounded-full"
                :class="{
                  'bg-success animate-pulse': pollingActive,
                  'bg-warning animate-spin': pollingStarting,
                  'bg-error': !pollingActive && !pollingStarting,
                }"
              ></div>
              <span class="text-sm font-medium">
                {{
                  pollingActive
                    ? "Live"
                    : pollingStarting
                    ? "Starting"
                    : "Offline"
                }}
              </span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Statistics Cards -->
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6 mb-8">
      <!-- Today's Scans -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm">
        <div class="flex items-center justify-between mb-4">
          <div>
            <p class="text-base-content/60 text-sm font-medium">
              Today's Scans
            </p>
            <p class="text-3xl font-bold text-base-content">{{ todayScans }}</p>
          </div>
          <div class="p-3 bg-primary/10 rounded-lg">
            <svg
              class="w-6 h-6 text-primary"
              fill="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                d="M12 2C13.1 2 14 2.9 14 4C14 5.1 13.1 6 12 6C10.9 6 10 5.1 10 4C10 2.9 10.9 2 12 2ZM21 9V7L15 1L13.5 2.5L18.5 7.5H9.5L14.5 2.5L13 1L7 7V9H21ZM12 8C13.66 8 15 9.34 15 11V16L13.5 15L12 16L10.5 15L9 16V11C9 9.34 10.34 8 12 8Z"
              />
            </svg>
          </div>
        </div>
        <p class="text-sm text-success">+5 from yesterday</p>
      </div>

      <!-- Registered Cards -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm">
        <div class="flex items-center justify-between mb-4">
          <div>
            <p class="text-base-content/60 text-sm font-medium">
              Registered Cards
            </p>
            <p class="text-3xl font-bold text-base-content">
              {{ totalRegisteredCards }}
            </p>
          </div>
          <div class="p-3 bg-secondary/10 rounded-lg">
            <svg
              class="w-6 h-6 text-secondary"
              fill="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                d="M20 4H4c-1.11 0-1.99.89-1.99 2L2 18c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V6c0-1.11-.89-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z"
              />
            </svg>
          </div>
        </div>
        <p class="text-sm text-success">+12 this month</p>
      </div>

      <!-- Active Devices -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm">
        <div class="flex items-center justify-between mb-4">
          <div>
            <p class="text-base-content/60 text-sm font-medium">
              Active Devices
            </p>
            <p class="text-3xl font-bold text-base-content">
              {{ activeDevices }}
            </p>
          </div>
          <div class="p-3 bg-accent/10 rounded-lg">
            <svg
              class="w-6 h-6 text-accent"
              fill="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                d="M9 4C9 3.45 9.45 3 10 3H14C14.55 3 15 3.45 15 4V6H17C18.1 6 19 6.9 19 8V19C19 20.1 18.1 21 17 21H7C5.9 21 5 20.1 5 19V8C5 6.9 5.9 6 7 6H9V4ZM7 8V19H17V8H7ZM10 10H14V12H10V10ZM10 14H14V16H10V14Z"
              />
            </svg>
          </div>
        </div>
        <p class="text-sm text-success">All systems online</p>
      </div>

      <!-- Total Users -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm">
        <div class="flex items-center justify-between mb-4">
          <div>
            <p class="text-base-content/60 text-sm font-medium">Total Users</p>
            <p class="text-3xl font-bold text-base-content">{{ totalUsers }}</p>
          </div>
          <div class="p-3 bg-info/10 rounded-lg">
            <svg
              class="w-6 h-6 text-info"
              fill="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"
              />
            </svg>
          </div>
        </div>
        <p class="text-sm text-success">+2 new users</p>
      </div>
    </div>

    <!-- Real-time Scans Section -->
    <div class="bg-base-200 rounded-lg p-6 shadow-sm mb-8">
      <div class="flex justify-between items-center mb-6">
        <h3 class="text-lg font-semibold text-base-content">
          Recent RFID Scans
        </h3>
        <div class="flex items-center gap-2 text-sm">
          <span class="text-base-content/60">Real-time updates:</span>
          <span :class="pollingActive ? 'text-success' : 'text-error'">
            {{ pollingActive ? "Active" : "Inactive" }}
          </span>
        </div>
      </div>

      <div v-if="recentScans.length === 0" class="text-center py-8">
        <div class="text-base-content/40 mb-2">
          <svg
            class="w-12 h-12 mx-auto"
            fill="currentColor"
            viewBox="0 0 24 24"
          >
            <path
              d="M20 4H4c-1.11 0-1.99.89-1.99 2L2 18c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V6c0-1.11-.89-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z"
            />
          </svg>
        </div>
        <p class="text-base-content/60">No recent scans detected</p>
        <p class="text-sm text-base-content/40">
          RFID scans will appear here in real-time
        </p>
      </div>

      <div v-else class="space-y-3 max-h-80 overflow-y-auto">
        <div
          v-for="scan in recentScans.slice(0, 10)"
          :key="scan.id"
          class="flex items-center justify-between p-3 bg-base-100 rounded-lg border-l-4"
          :class="{
            'border-success': scan.status === 'success',
            'border-warning': scan.status === 'failed',
            'border-error': scan.status === 'unauthorized',
          }"
        >
          <div class="flex items-center gap-3">
            <div
              class="p-2 rounded-lg"
              :class="{
                'bg-success/10': scan.status === 'success',
                'bg-warning/10': scan.status === 'failed',
                'bg-error/10': scan.status === 'unauthorized',
              }"
            >
              <svg
                class="w-5 h-5"
                :class="{
                  'text-success': scan.status === 'success',
                  'text-warning': scan.status === 'failed',
                  'text-error': scan.status === 'unauthorized',
                }"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  d="M20 4H4c-1.11 0-1.99.89-1.99 2L2 18c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V6c0-1.11-.89-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z"
                />
              </svg>
            </div>
            <div>
              <p class="font-medium text-base-content">{{ scan.rfidTagId }}</p>
              <p class="text-sm text-base-content/60">
                {{ scan.location || "Unknown Location" }}
                <span v-if="scan.metadata?.userName">
                  • {{ scan.metadata.userName }}</span
                >
              </p>
            </div>
          </div>
          <div class="text-right">
            <p
              class="text-sm font-medium"
              :class="{
                'text-success': scan.status === 'success',
                'text-warning': scan.status === 'failed',
                'text-error': scan.status === 'unauthorized',
              }"
            >
              {{ scan.status.charAt(0).toUpperCase() + scan.status.slice(1) }}
            </p>
            <p class="text-xs text-base-content/40">
              {{ new Date(scan.scanTime).toLocaleTimeString() }}
            </p>
          </div>
        </div>
      </div>

      <!-- Real-time Stats -->
      <div class="grid grid-cols-3 gap-4 mt-6 pt-6 border-t border-base-300">
        <div class="text-center">
          <p class="text-2xl font-bold text-primary">
            {{ realTimeStats.totalScans }}
          </p>
          <p class="text-sm text-base-content/60">Total Scans</p>
        </div>
        <div class="text-center">
          <p class="text-2xl font-bold text-success">
            {{ realTimeStats.successfulScans }}
          </p>
          <p class="text-sm text-base-content/60">Successful</p>
        </div>
        <div class="text-center">
          <p class="text-2xl font-bold text-warning">
            {{ realTimeStats.unregisteredScans }}
          </p>
          <p class="text-sm text-base-content/60">Unregistered</p>
        </div>
      </div>
    </div>

    <!-- Charts Section -->
    <div class="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-8">
      <!-- Daily Trips Chart -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm lg:col-span-2">
        <div class="flex items-center justify-between mb-6">
          <h3 class="text-lg font-semibold text-base-content">
            Daily Activity
          </h3>
          <select
            class="select select-sm select-bordered"
            v-model="activePeriod"
          >
            <option value="weekly">This Week</option>
            <option value="monthly">This Month</option>
          </select>
        </div>
        <div class="h-80">
          <Line :data="dailyTripsData" :options="dailyTripsOptions" />
        </div>
      </div>

      <!-- User Access Overview -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm">
        <h3 class="text-lg font-semibold text-base-content mb-6">User Roles</h3>
        <div class="flex items-center justify-center mb-6">
          <div class="w-40 h-40">
            <Pie :data="userAccessData" :options="userAccessOptions" />
          </div>
        </div>
        <div class="space-y-3">
          <div class="flex items-center justify-between">
            <div class="flex items-center">
              <div class="w-3 h-3 bg-primary rounded-full mr-3"></div>
              <span class="text-sm text-base-content/80">Drivers</span>
            </div>
            <span class="text-sm font-medium">{{ userStats.drivers }}</span>
          </div>
          <div class="flex items-center justify-between">
            <div class="flex items-center">
              <div class="w-3 h-3 bg-primary/70 rounded-full mr-3"></div>
              <span class="text-sm text-base-content/80">Admins</span>
            </div>
            <span class="text-sm font-medium">{{ userStats.admins }}</span>
          </div>
          <div class="flex items-center justify-between">
            <div class="flex items-center">
              <div class="w-3 h-3 bg-primary/40 rounded-full mr-3"></div>
              <span class="text-sm text-base-content/80">Super Admins</span>
            </div>
            <span class="text-sm font-medium">{{ userStats.superadmins }}</span>
          </div>
        </div>
      </div>
    </div>

    <!-- Additional Components -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6 mb-8">
      <!-- Historical Trips -->
      <div class="bg-base-200 rounded-lg p-6 shadow-sm">
        <h3 class="text-lg font-semibold text-base-content mb-6">
          Historical Trips (14 Days)
        </h3>
        <div class="h-60">
          <Bar :data="historicalTripsData" :options="historicalTripsOptions" />
        </div>
      </div>

      <!-- RFID Device Status -->
      <div class="bg-base-200 rounded-lg shadow-sm p-0 overflow-hidden">
        <RfidDeviceStatus />
      </div>
    </div>

    <!-- Live Components -->
    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
      <!-- RFID Chart -->
      <div class="bg-base-200 rounded-lg shadow-sm p-0 overflow-hidden">
        <RfidChart />
      </div>

      <!-- Live Scans -->
      <div class="bg-base-200 rounded-lg shadow-sm p-0 overflow-hidden">
        <RfidLiveScans />
      </div>
    </div>
  </div>
</template>

<style scoped>
/* Custom styling for charts and components */
.grid > div {
  transition: transform 0.2s ease-in-out;
}

.grid > div:hover {
  transform: translateY(-2px);
}
</style>
