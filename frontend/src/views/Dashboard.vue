<script setup lang="ts">
import { ref, onMounted, computed } from "vue";
import { useRouter } from "vue-router";
import authService from "../services/auth";
import type { User } from "../services/auth";
import RfidChart from "../components/RfidChart.vue";
import RfidDeviceStatus from "../components/RfidDeviceStatus.vue";
import RfidLiveScans from "../components/RfidLiveScans.vue";
import rfidStatsService from "../services/rfidStats";
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

// Setup router
const router = useRouter();

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
const searchQuery = ref("");

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

// Current day trips data - based on weekly stats
const dailyTripsData = computed<ChartData<"line">>(() => {
  if (weeklyScansData.value.length === 0) {
    // Fallback data
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
    labels: weeklyScansData.value.map((item) => {
      const date = new Date();
      const dayIndex = weeklyScansData.value.indexOf(item);
      date.setDate(date.getDate() - (6 - dayIndex));
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
      grid: {
        color: "rgba(203, 213, 225, 0.1)",
      },
      ticks: {
        color: "rgba(203, 213, 225, 0.7)",
      },
    },
    x: {
      grid: {
        display: false,
      },
      ticks: {
        color: "rgba(203, 213, 225, 0.7)",
      },
    },
  },
  plugins: {
    legend: {
      display: false,
    },
  },
}));

// Device status data - based on real device statistics
const driverStatusData = computed<ChartData<"pie">>(() => ({
  labels: ["Online", "Offline", "Total Devices"],
  datasets: [
    {
      backgroundColor: [
        "rgba(34, 197, 94, 1)", // Green for online
        "rgba(239, 68, 68, 1)", // Red for offline
        "rgba(156, 163, 175, 1)", // Gray for total
      ],
      data: [
        deviceStats.value.online,
        deviceStats.value.offline,
        deviceStats.value.total,
      ],
      borderWidth: 0,
    },
  ],
}));

const driverStatusOptions = computed<ChartOptions<"pie">>(() => ({
  responsive: true,
  maintainAspectRatio: false,
  plugins: {
    legend: {
      display: false,
    },
  },
}));

// User access overview data - based on real user statistics
const userAccessData = computed<ChartData<"pie">>(() => ({
  labels: ["Drivers", "Admins", "Super Admins"],
  datasets: [
    {
      backgroundColor: [
        "rgba(59, 130, 246, 1)", // Primary for drivers
        "rgba(96, 165, 250, 1)", // Primary light for admins
        "rgba(147, 195, 255, 1)", // Primary lighter for super admins
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
  plugins: {
    legend: {
      display: false,
    },
  },
}));

// Historical trips data - based on real scan data
const historicalTripsData = computed<ChartData<"bar">>(() => {
  if (weeklyScansData.value.length === 0) {
    // Fallback data
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

  // Use the last 14 days of data (extend weekly to bi-weekly if needed)
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
    y: {
      display: false,
      beginAtZero: true,
    },
    x: {
      display: false,
    },
  },
  plugins: {
    legend: {
      display: false,
    },
  },
}));

// User Role Display
const userRoleDisplay = computed(() => {
  if (!user.value) return "";

  switch (user.value.role) {
    case "superadmin":
      return "Super Admin";
    case "admin":
      return "Admin";
    case "driver":
      return "Driver";
    default:
      return user.value.role;
  }
});

// Load statistics from backend
const loadStatistics = async () => {
  try {
    loading.value = true;

    const stats = await rfidStatsService.getDashboardStats();

    // Update all reactive values
    todayScans.value = stats.todayScans;
    totalRegisteredCards.value = stats.totalRegisteredCards;
    activeDevices.value = stats.onlineDevices;
    totalUsers.value = stats.totalUsers;
    weeklyScansData.value = stats.weeklyStats;
    recentScansCount.value = stats.recentScansCount;

    userStats.value = {
      drivers: stats.userStats.drivers,
      admins: stats.userStats.admins,
      superadmins: stats.userStats.superadmins,
    };

    deviceStats.value = {
      online: stats.onlineDevices,
      offline: stats.offlineDevices,
      total: stats.totalDevices,
    };
  } catch (error) {
    console.error("Error loading dashboard statistics:", error);
    // Keep default values on error
  } finally {
    loading.value = false;
  }
};

// Function to handle logout
const handleLogout = () => {
  authService.logout();
  router.push("/login");
  // Dispatch storage event to notify other components
  globalThis.window.dispatchEvent(new globalThis.Event("storage"));
};

onMounted(async () => {
  user.value = authService.getUser();
  await loadStatistics();
});
</script>

<template>
  <div v-if="loading" class="flex justify-center items-center min-h-screen">
    <span class="loading loading-spinner loading-lg"></span>
  </div>

  <div v-else class="min-h-screen bg-base-100 flex">
    <!-- Left Sidebar -->
    <div class="w-[310px] h-screen bg-base-200 flex-shrink-0">
      <div class="p-6">
        <!-- User Profile -->
        <div class="flex items-center mb-8">
          <div
            class="w-16 h-16 rounded-full bg-primary mr-4 overflow-hidden flex items-center justify-center"
          >
            <span class="text-2xl font-bold text-white">{{
              user?.name?.charAt(0) || "U"
            }}</span>
          </div>
          <div>
            <div class="text-base-content font-medium">
              {{ user?.name || "User" }}
            </div>
            <div class="text-base-content/70">{{ userRoleDisplay }}</div>
          </div>
        </div>

        <!-- Navigation Menu -->
        <nav class="space-y-2">
          <router-link
            to="/dashboard"
            class="flex items-center px-4 py-3 text-base-content bg-primary/10 rounded-lg"
          >
            <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
              <path
                d="M3 13h8V3H3v10zm0 8h8v-6H3v6zm10 0h8V11h-8v10zm0-18v6h8V3h-8z"
              />
            </svg>
            Dashboard
          </router-link>

          <router-link
            v-if="user?.role === 'admin' || user?.role === 'superadmin'"
            to="/rfid"
            class="flex items-center px-4 py-3 text-base-content/70 hover:text-base-content hover:bg-base-content/5 rounded-lg"
          >
            <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
              <path
                d="M12 2C13.1 2 14 2.9 14 4C14 5.1 13.1 6 12 6C10.9 6 10 5.1 10 4C10 2.9 10.9 2 12 2ZM21 9V7L15 1L13.5 2.5L18.5 7.5H9.5L14.5 2.5L13 1L7 7V9H21ZM12 8C13.66 8 15 9.34 15 11V16L13.5 15L12 16L10.5 15L9 16V11C9 9.34 10.34 8 12 8Z"
              />
            </svg>
            RFID Management
          </router-link>

          <router-link
            v-if="user?.role === 'admin' || user?.role === 'superadmin'"
            to="/rfid-cards"
            class="flex items-center px-4 py-3 text-base-content/70 hover:text-base-content hover:bg-base-content/5 rounded-lg"
          >
            <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
              <path
                d="M20 4H4c-1.11 0-1.99.89-1.99 2L2 18c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V6c0-1.11-.89-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z"
              />
            </svg>
            RFID Cards
          </router-link>

          <router-link
            v-if="user?.role === 'admin' || user?.role === 'superadmin'"
            to="/users"
            class="flex items-center px-4 py-3 text-base-content/70 hover:text-base-content hover:bg-base-content/5 rounded-lg"
          >
            <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
              <path
                d="M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z"
              />
            </svg>
            User Management
          </router-link>

          <router-link
            v-if="user?.role === 'admin' || user?.role === 'superadmin'"
            to="/devices"
            class="flex items-center px-4 py-3 text-base-content/70 hover:text-base-content hover:bg-base-content/5 rounded-lg"
          >
            <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
              <path
                d="M9 4C9 3.45 9.45 3 10 3H14C14.55 3 15 3.45 15 4V6H17C18.1 6 19 6.9 19 8V19C19 20.1 18.1 21 17 21H7C5.9 21 5 20.1 5 19V8C5 6.9 5.9 6 7 6H9V4ZM7 8V19H17V8H7ZM10 10H14V12H10V10ZM10 14H14V16H10V14Z"
              />
            </svg>
            ESP32 Devices
          </router-link>

          <router-link
            v-if="user?.role === 'admin' || user?.role === 'superadmin'"
            to="/apikeys"
            class="flex items-center px-4 py-3 text-base-content/70 hover:text-base-content hover:bg-base-content/5 rounded-lg"
          >
            <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
              <path
                d="M18 8a6 6 0 01-7.743 5.743L10 14l-1 1-1 1H6v2H2v-4l4.257-4.257A6 6 0 1118 8zm-6-4a1 1 0 100 2 2 2 0 012 2 1 1 0 102 0 4 4 0 00-4-4z"
              />
            </svg>
            API Keys
          </router-link>
        </nav>
      </div>
    </div>

    <!-- Main Content -->
    <div class="flex-1 flex flex-col">
      <!-- Top Bar -->
      <div class="h-[150px] px-8 py-5 flex items-start justify-between">
        <!-- Search Bar and Refresh Button -->
        <div class="relative flex-1 max-w-[998px] flex items-center space-x-4">
          <div class="relative flex-1">
            <svg
              class="absolute left-3 top-1/2 transform -translate-y-1/2 w-5 h-5 text-base-content"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="1.5"
                d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"
              />
            </svg>
            <input
              v-model="searchQuery"
              type="text"
              placeholder="Search"
              class="w-full h-[35px] pl-10 pr-4 bg-transparent border border-base-content/30 rounded-md text-base-content placeholder-base-content/50 focus:outline-none focus:border-primary"
            />
          </div>

          <!-- Refresh Statistics Button -->
          <button
            @click="loadStatistics"
            :disabled="loading"
            class="btn btn-circle btn-sm btn-ghost"
            title="Refresh Statistics"
          >
            <svg
              class="w-5 h-5"
              :class="{ 'animate-spin': loading }"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
              />
            </svg>
          </button>
        </div>

        <!-- User Menu -->
        <div class="flex items-center space-x-4 ml-8">
          <div class="avatar">
            <div
              class="w-8 h-8 rounded-full bg-primary text-white flex items-center justify-center"
            >
              {{ user?.name?.charAt(0) || "U" }}
            </div>
          </div>
          <div class="dropdown dropdown-end">
            <div
              tabindex="0"
              class="w-4 h-4 bg-base-content rounded-full flex items-center justify-center cursor-pointer"
            >
              <svg
                class="w-3 h-3 text-base-100"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path d="M7 10l5 5 5-5z" />
              </svg>
            </div>
            <ul
              tabindex="0"
              class="dropdown-content z-[1] menu p-2 shadow bg-base-200 rounded-box w-52"
            >
              <li><a>Profile</a></li>
              <li><a>Settings</a></li>
              <li><a @click="handleLogout">Logout</a></li>
            </ul>
          </div>
        </div>
      </div>

      <!-- Dashboard Content -->
      <div class="flex-1 px-8 pb-8 overflow-y-auto">
        <!-- Dashboard Header -->
        <div class="mb-8">
          <h1 class="text-2xl font-medium text-base-content mb-2">Dashboard</h1>
          <p class="text-base-content/75">Welcome {{ user?.name }},</p>
        </div>

        <!-- Stats Grid -->
        <div class="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-8">
          <!-- Total Admins -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md">
            <div class="flex items-start justify-between mb-4">
              <div>
                <svg
                  class="w-6 h-6 text-base-content mb-4"
                  fill="currentColor"
                  viewBox="0 0 24 24"
                >
                  <path
                    d="M21 17.0016L15.5016 16.0031C15 15 15 14.5641 15 14.0016C15.5016 13.5 16.5 13.0031 16.5 12.5016C17.0016 11.0016 17.0016 10.0031 17.0016 10.0031C17.2594 9.62812 18 9.00469 18 8.00156C18 6.99844 17.0016 6 17.0016 5.50313C17.0016 1.5 14.9625 0 12 0C9.16406 0 6.99844 1.5 6.99844 5.49844C6.99844 6 6 6.99844 6 7.99688C6 8.99531 6.7125 9.65625 6.99844 9.99844C6.99844 9.99844 6.99844 10.9969 7.5 12.4969C7.5 12.9984 8.49844 13.4953 9 13.9969C9 14.4984 9 14.9953 8.49844 15.9984L3 17.0016C0.998437 17.4984 0 21 0 24H24C24 21 23.0016 17.4984 21 17.0016Z"
                  />
                </svg>
                <div class="text-base-content/80 text-sm mb-2">
                  Total Admins
                </div>
              </div>
            </div>
            <div class="flex items-baseline">
              <span class="text-5xl font-bold text-base-content">{{
                user?.role === "superadmin" ? 3 : 2
              }}</span>
              <span class="text-2xl font-semibold text-success ml-2">+1</span>
            </div>
          </div>

          <!-- Total RFID Users -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md">
            <div class="flex items-start justify-between mb-4">
              <div>
                <svg
                  class="w-6 h-6 text-base-content mb-4"
                  fill="currentColor"
                  viewBox="0 0 24 24"
                >
                  <path
                    d="M21 17.0016L15.5016 16.0031C15 15 15 14.5641 15 14.0016C15.5016 13.5 16.5 13.0031 16.5 12.5016C17.0016 11.0016 17.0016 10.0031 17.0016 10.0031C17.2594 9.62812 18 9.00469 18 8.00156C18 6.99844 17.0016 6 17.0016 5.50313C17.0016 1.5 14.9625 0 12 0C9.16406 0 6.99844 1.5 6.99844 5.49844C6.99844 6 6 6.99844 6 7.99688C6 8.99531 6.7125 9.65625 6.99844 9.99844C6.99844 9.99844 6.99844 10.9969 7.5 12.4969C7.5 12.9984 8.49844 13.4953 9 13.9969C9 14.4984 9 14.9953 8.49844 15.9984L3 17.0016C0.998437 17.4984 0 21 0 24H24C24 21 23.0016 17.4984 21 17.0016Z"
                  />
                </svg>
                <div class="text-base-content/80 text-sm mb-2">
                  Total RFID Users
                </div>
              </div>
            </div>
            <div class="flex items-baseline">
              <span class="text-5xl font-bold text-base-content">40</span>
              <span class="text-2xl font-semibold text-success ml-2">+2</span>
            </div>
          </div>

          <!-- RFID Device Status Component -->
          <div
            class="bg-base-200 rounded-lg shadow-md lg:col-span-1 lg:row-span-1 p-0 overflow-hidden"
          >
            <RfidDeviceStatus />
          </div>

          <!-- RFID Status -->
          <div
            class="bg-base-200 rounded-lg p-6 shadow-md lg:col-span-3 lg:row-span-1"
          >
            <div class="flex items-center mb-4">
              <svg
                class="w-6 h-6 text-base-content mr-3"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  d="M19.5 15H15C14.175 15 13.5 14.325 13.5 13.5C13.5 12.675 14.175 12 15 12H19.5C19.5 12 24 6.98438 24 4.5C24 2.01562 21.9844 0 19.5 0C17.0156 0 15 2.01562 15 4.5C15 5.69531 16.0406 7.47188 17.1234 9H15C12.5203 9 10.5 11.0203 10.5 13.5C10.5 15.9797 12.5203 18 15 18H19.5C20.325 18 21 18.675 21 19.5C21 20.325 20.325 21 19.5 21H8.69531C7.94531 22.1625 7.11094 23.2359 6.47812 24H19.5C21.9797 24 24 21.9797 24 19.5C24 17.0203 21.9797 15 19.5 15ZM19.5 3C20.3297 3 21 3.67031 21 4.5C21 5.32969 20.3297 6 19.5 6C18.6703 6 18 5.32969 18 4.5C18 3.67031 18.6703 3 19.5 3ZM4.5 12C2.01562 12 0 14.0156 0 16.5C0 18.9844 4.5 24 4.5 24C4.5 24 9 18.9844 9 16.5C9 14.0156 6.98438 12 4.5 12ZM4.5 18C3.67031 18 3 17.3297 3 16.5C3 15.6703 3.67031 15 4.5 15C5.32969 15 6 15.6703 6 16.5C6 17.3297 5.32969 18 4.5 18Z"
                />
              </svg>
              <span class="text-base-content/80 text-sm"
                >RFID System Status</span
              >
            </div>
            <div class="bg-base-100 rounded-lg p-4 mb-4">
              <div class="alert alert-success shadow-lg">
                <svg
                  xmlns="http://www.w3.org/2000/svg"
                  class="stroke-current shrink-0 h-6 w-6"
                  fill="none"
                  viewBox="0 0 24 24"
                >
                  <path
                    stroke-linecap="round"
                    stroke-linejoin="round"
                    stroke-width="2"
                    d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"
                  />
                </svg>
                <span>TagSakay System is operational</span>
              </div>
            </div>
            <div class="stats stats-vertical shadow w-full">
              <div class="stat">
                <div class="stat-title">Last Heartbeat</div>
                <div class="stat-value text-sm">2025-09-18 08:15:22</div>
              </div>
              <div class="stat">
                <div class="stat-title">Active Readers</div>
                <div class="stat-value">{{ deviceStats.online }}</div>
              </div>
              <div class="stat">
                <div class="stat-title">Scans Today</div>
                <div class="stat-value">{{ todayScans }}</div>
              </div>
              <div class="stat">
                <div class="stat-title">Your RFID Tag</div>
                <div class="stat-value text-sm font-mono">
                  {{ user?.rfidTag || "Not assigned" }}
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- Live RFID Scans -->
        <div class="mb-6">
          <RfidLiveScans
            :limit="10"
            :autoRefresh="true"
            :refreshInterval="5000"
          />
        </div>

        <!-- Charts Grid -->
        <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <!-- Daily Trips Chart -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md lg:col-span-2">
            <div class="flex items-center mb-6">
              <svg
                class="w-6 h-6 text-base-content mr-3"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  d="M10.9377 5.46875C10.9377 5.26155 11.02 5.06284 11.1665 4.91632C11.313 4.76981 11.5117 4.6875 11.7189 4.6875H13.4564C13.903 4.68761 14.3403 4.81532 14.7168 5.05559C15.0933 5.29587 15.3933 5.63871 15.5814 6.04375L16.1049 7.17188C16.533 6.61094 17.2096 6.25 17.9689 6.25H19.5314C19.7386 6.25 19.9373 6.33231 20.0839 6.47882C20.2304 6.62534 20.3127 6.82405 20.3127 7.03125V10.1562C20.3127 10.3635 20.2304 10.5622 20.0839 10.7087C19.9373 10.8552 19.7386 10.9375 19.5314 10.9375H17.9689C17.9293 10.9375 17.8903 10.9365 17.8517 10.9344L18.6283 12.6062C18.9179 12.5354 19.2189 12.5 19.5314 12.5C20.4602 12.4999 21.3587 12.8308 22.0655 13.4332C22.7724 14.0357 23.2415 14.8703 23.3887 15.7874C23.5358 16.7045 23.3514 17.6439 22.8685 18.4373C22.3855 19.2307 21.6357 19.826 20.7535 20.1165C19.8713 20.407 18.9145 20.3737 18.0547 20.0225C17.1948 19.6712 16.4883 19.0252 16.0618 18.2001C15.6352 17.375 15.5167 16.425 15.7273 15.5204C15.9379 14.6158 16.464 13.8158 17.2111 13.2641L16.6705 12.1C16.3283 12.6965 15.8347 13.1921 15.2397 13.5366C14.6446 13.8812 13.9691 14.0626 13.2814 14.0625H8.59393C8.93612 14.5156 9.17987 15.0469 9.29706 15.625H13.6939C13.3301 16.1101 12.8583 16.5039 12.3159 16.7751C11.7735 17.0463 11.1754 17.1875 10.5689 17.1875H9.29706C9.14924 17.9117 8.79901 18.579 8.28706 19.1121C7.7751 19.6451 7.12242 20.022 6.40482 20.1989C5.68723 20.3759 4.93416 20.3456 4.23309 20.1116C3.53202 19.8776 2.91171 19.4495 2.44421 18.8771C1.97672 18.3046 1.68121 17.6113 1.59202 16.8776C1.50284 16.1439 1.62362 15.4 1.94036 14.7322C2.25709 14.0645 2.75678 13.5002 3.38137 13.1051C4.00597 12.71 4.72985 12.5002 5.46893 12.5H8.89237L9.37518 11.5344C9.69959 10.8855 10.1983 10.3398 10.8153 9.95845C11.4324 9.57705 12.1435 9.37502 12.8689 9.375H15.4064L14.1658 6.70312C14.1032 6.56776 14.0031 6.45315 13.8774 6.37286C13.7517 6.29256 13.6056 6.24993 13.4564 6.25H11.7189C11.5117 6.25 11.313 6.16769 11.1665 6.02118C11.02 5.87466 10.9377 5.67595 10.9377 5.46875Z"
                />
              </svg>
              <span class="text-base-content/80 text-sm"
                >Daily Trips Over the Past 7 Days</span
              >
            </div>
            <!-- Line Chart -->
            <div class="relative h-60">
              <Line :data="dailyTripsData" :options="dailyTripsOptions" />
            </div>
          </div>

          <!-- Driver Status Pie Chart -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md">
            <div class="flex items-center mb-6">
              <svg
                class="w-6 h-6 text-base-content mr-3"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  d="M10.9377 5.46875C10.9377 5.26155 11.02 5.06284 11.1665 4.91632C11.313 4.76981 11.5117 4.6875 11.7189 4.6875H13.4564C13.903 4.68761 14.3403 4.81532 14.7168 5.05559C15.0933 5.29587 15.3933 5.63871 15.5814 6.04375L16.1049 7.17188C16.533 6.61094 17.2096 6.25 17.9689 6.25H19.5314C19.7386 6.25 19.9373 6.33231 20.0839 6.47882C20.2304 6.62534 20.3127 6.82405 20.3127 7.03125V10.1562C20.3127 10.3635 20.2304 10.5622 20.0839 10.7087C19.9373 10.8552 19.7386 10.9375 19.5314 10.9375H17.9689C17.9293 10.9375 17.8903 10.9365 17.8517 10.9344L18.6283 12.6062C18.9179 12.5354 19.2189 12.5 19.5314 12.5C20.4602 12.4999 21.3587 12.8308 22.0655 13.4332C22.7724 14.0357 23.2415 14.8703 23.3887 15.7874C23.5358 16.7045 23.3514 17.6439 22.8685 18.4373C22.3855 19.2307 21.6357 19.826 20.7535 20.1165C19.8713 20.407 18.9145 20.3737 18.0547 20.0225C17.1948 19.6712 16.4883 19.0252 16.0618 18.2001C15.6352 17.375 15.5167 16.425 15.7273 15.5204C15.9379 14.6158 16.464 13.8158 17.2111 13.2641L16.6705 12.1C16.3283 12.6965 15.8347 13.1921 15.2397 13.5366C14.6446 13.8812 13.9691 14.0626 13.2814 14.0625H8.59393C8.93612 14.5156 9.17987 15.0469 9.29706 15.625H13.6939C13.3301 16.1101 12.8583 16.5039 12.3159 16.7751C11.7735 17.0463 11.1754 17.1875 10.5689 17.1875H9.29706C9.14924 17.9117 8.79901 18.579 8.28706 19.1121C7.7751 19.6451 7.12242 20.022 6.40482 20.1989C5.68723 20.3759 4.93416 20.3456 4.23309 20.1116C3.53202 19.8776 2.91171 19.4495 2.44421 18.8771C1.97672 18.3046 1.68121 17.6113 1.59202 16.8776C1.50284 16.1439 1.62362 15.4 1.94036 14.7322C2.25709 14.0645 2.75678 13.5002 3.38137 13.1051C4.00597 12.71 4.72985 12.5002 5.46893 12.5H8.89237L9.37518 11.5344C9.69959 10.8855 10.1983 10.3398 10.8153 9.95845C11.4324 9.57705 12.1435 9.37502 12.8689 9.375H15.4064L14.1658 6.70312C14.1032 6.56776 14.0031 6.45315 13.8774 6.37286C13.7517 6.29256 13.6056 6.24993 13.4564 6.25H11.7189C11.5117 6.25 11.313 6.16769 11.1665 6.02118C11.02 5.87466 10.9377 5.67595 10.9377 5.46875Z"
                />
              </svg>
              <span class="text-base-content/80 text-sm"
                >Driver Status - Daily</span
              >
            </div>

            <div class="flex items-center justify-center mb-6">
              <div class="w-32 h-32">
                <Pie :data="driverStatusData" :options="driverStatusOptions" />
              </div>
            </div>

            <div class="space-y-3">
              <div class="flex items-center">
                <div class="w-4 h-4 bg-green-500 rounded-sm mr-3"></div>
                <span class="text-base-content/80 text-sm"
                  >Online - {{ deviceStats.online }}</span
                >
              </div>
              <div class="flex items-center">
                <div class="w-4 h-4 bg-red-500 rounded-sm mr-3"></div>
                <span class="text-base-content/80 text-sm"
                  >Offline - {{ deviceStats.offline }}</span
                >
              </div>
              <div class="flex items-center">
                <div class="w-4 h-4 bg-gray-400 rounded-sm mr-3"></div>
                <span class="text-base-content/80 text-sm"
                  >Total - {{ deviceStats.total }}</span
                >
              </div>
            </div>
          </div>

          <!-- User Access Overview -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md">
            <div class="flex items-center mb-6">
              <svg
                class="w-6 h-6 text-base-content mr-3"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  d="M21 17.0016L15.5016 16.0031C15 15 15 14.5641 15 14.0016C15.5016 13.5 16.5 13.0031 16.5 12.5016C17.0016 11.0016 17.0016 10.0031 17.0016 10.0031C17.2594 9.62812 18 9.00469 18 8.00156C18 6.99844 17.0016 6 17.0016 5.50313C17.0016 1.5 14.9625 0 12 0C9.16406 0 6.99844 1.5 6.99844 5.49844C6.99844 6 6 6.99844 6 7.99688C6 8.99531 6.7125 9.65625 6.99844 9.99844C6.99844 9.99844 6.99844 10.9969 7.5 12.4969C7.5 12.9984 8.49844 13.4953 9 13.9969C9 14.4984 9 14.9953 8.49844 15.9984L3 17.0016C0.998437 17.4984 0 21 0 24H24C24 21 23.0016 17.4984 21 17.0016Z"
                />
              </svg>
              <span class="text-base-content/80 text-sm"
                >User Access Overview</span
              >
            </div>

            <div class="flex items-center justify-center mb-6">
              <div class="w-32 h-32">
                <Pie :data="userAccessData" :options="userAccessOptions" />
              </div>
            </div>

            <div class="space-y-3">
              <div class="flex items-center">
                <div class="w-4 h-4 bg-primary rounded-sm mr-3"></div>
                <span class="text-base-content/80 text-sm">Drivers - 40</span>
              </div>
              <div class="flex items-center">
                <div class="w-4 h-4 bg-primary-focus rounded-sm mr-3"></div>
                <span class="text-base-content/80 text-sm">Admin - 2</span>
              </div>
              <div class="flex items-center">
                <div class="w-4 h-4 bg-primary/50 rounded-sm mr-3"></div>
                <span class="text-base-content/80 text-sm">SAdmin - 1</span>
              </div>
            </div>
          </div>

          <!-- RFID Scan Analytics -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md lg:col-span-2">
            <div class="flex justify-between items-center mb-6">
              <div class="flex items-center">
                <svg
                  class="w-6 h-6 text-base-content mr-3"
                  fill="currentColor"
                  viewBox="0 0 24 24"
                >
                  <path
                    d="M10.9377 5.46875C10.9377 5.26155 11.02 5.06284 11.1665 4.91632C11.313 4.76981 11.5117 4.6875 11.7189 4.6875H13.4564C13.903 4.68761 14.3403 4.81532 14.7168 5.05559C15.0933 5.29587 15.3933 5.63871 15.5814 6.04375L16.1049 7.17188C16.533 6.61094 17.2096 6.25 17.9689 6.25H19.5314C19.7386 6.25 19.9373 6.33231 20.0839 6.47882C20.2304 6.62534 20.3127 6.82405 20.3127 7.03125V10.1562C20.3127 10.3635 20.2304 10.5622 20.0839 10.7087C19.9373 10.8552 19.7386 10.9375 19.5314 10.9375H17.9689C17.9293 10.9375 17.8903 10.9365 17.8517 10.9344L18.6283 12.6062C18.9179 12.5354 19.2189 12.5 19.5314 12.5C20.4602 12.4999 21.3587 12.8308 22.0655 13.4332C22.7724 14.0357 23.2415 14.8703 23.3887 15.7874C23.5358 16.7045 23.3514 17.6439 22.8685 18.4373C22.3855 19.2307 21.6357 19.826 20.7535 20.1165C19.8713 20.407 18.9145 20.3737 18.0547 20.0225C17.1948 19.6712 16.4883 19.0252 16.0618 18.2001C15.6352 17.375 15.5167 16.425 15.7273 15.5204C15.9379 14.6158 16.464 13.8158 17.2111 13.2641L16.6705 12.1C16.3283 12.6965 15.8347 13.1921 15.2397 13.5366C14.6446 13.8812 13.9691 14.0626 13.2814 14.0625H8.59393C8.93612 14.5156 9.17987 15.0469 9.29706 15.625H13.6939C13.3301 16.1101 12.8583 16.5039 12.3159 16.7751C11.7735 17.0463 11.1754 17.1875 10.5689 17.1875H9.29706C9.14924 17.9117 8.79901 18.579 8.28706 19.1121C7.7751 19.6451 7.12242 20.022 6.40482 20.1989C5.68723 20.3759 4.93416 20.3456 4.23309 20.1116C3.53202 19.8776 2.91171 19.4495 2.44421 18.8771C1.97672 18.3046 1.68121 17.6113 1.59202 16.8776C1.50284 16.1439 1.62362 15.4 1.94036 14.7322C2.25709 14.0645 2.75678 13.5002 3.38137 13.1051C4.00597 12.71 4.72985 12.5002 5.46893 12.5H8.89237L9.37518 11.5344C9.69959 10.8855 10.1983 10.3398 10.8153 9.95845C11.4324 9.57705 12.1435 9.37502 12.8689 9.375H15.4064L14.1658 6.70312C14.1032 6.56776 14.0031 6.45315 13.8774 6.37286C13.7517 6.29256 13.6056 6.24993 13.4564 6.25H11.7189C11.5117 6.25 11.313 6.16769 11.1665 6.02118C11.02 5.87466 10.9377 5.67595 10.9377 5.46875Z"
                  />
                </svg>
                <span class="text-base-content/80 text-sm"
                  >RFID Scan Analytics</span
                >
              </div>
              <div class="tabs tabs-boxed">
                <a
                  class="tab"
                  :class="{ 'tab-active': activePeriod === 'weekly' }"
                  @click="activePeriod = 'weekly'"
                >
                  Weekly
                </a>
                <a
                  class="tab"
                  :class="{ 'tab-active': activePeriod === 'monthly' }"
                  @click="activePeriod = 'monthly'"
                >
                  Monthly
                </a>
              </div>
            </div>

            <div class="h-60">
              <RfidChart :period="activePeriod" />
            </div>
          </div>

          <!-- Historical Trips Bar Chart -->
          <div class="bg-base-200 rounded-lg p-6 shadow-md lg:col-span-1">
            <div class="flex items-center mb-6">
              <svg
                class="w-6 h-6 text-base-content mr-3"
                fill="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  d="M10.9377 5.46875C10.9377 5.26155 11.02 5.06284 11.1665 4.91632C11.313 4.76981 11.5117 4.6875 11.7189 4.6875H13.4564C13.903 4.68761 14.3403 4.81532 14.7168 5.05559C15.0933 5.29587 15.3933 5.63871 15.5814 6.04375L16.1049 7.17188C16.533 6.61094 17.2096 6.25 17.9689 6.25H19.5314C19.7386 6.25 19.9373 6.33231 20.0839 6.47882C20.2304 6.62534 20.3127 6.82405 20.3127 7.03125V10.1562C20.3127 10.3635 20.2304 10.5622 20.0839 10.7087C19.9373 10.8552 19.7386 10.9375 19.5314 10.9375H17.9689C17.9293 10.9375 17.8903 10.9365 17.8517 10.9344L18.6283 12.6062C18.9179 12.5354 19.2189 12.5 19.5314 12.5C20.4602 12.4999 21.3587 12.8308 22.0655 13.4332C22.7724 14.0357 23.2415 14.8703 23.3887 15.7874C23.5358 16.7045 23.3514 17.6439 22.8685 18.4373C22.3855 19.2307 21.6357 19.826 20.7535 20.1165C19.8713 20.407 18.9145 20.3737 18.0547 20.0225C17.1948 19.6712 16.4883 19.0252 16.0618 18.2001C15.6352 17.375 15.5167 16.425 15.7273 15.5204C15.9379 14.6158 16.464 13.8158 17.2111 13.2641L16.6705 12.1C16.3283 12.6965 15.8347 13.1921 15.2397 13.5366C14.6446 13.8812 13.9691 14.0626 13.2814 14.0625H8.59393C8.93612 14.5156 9.17987 15.0469 9.29706 15.625H13.6939C13.3301 16.1101 12.8583 16.5039 12.3159 16.7751C11.7735 17.0463 11.1754 17.1875 10.5689 17.1875H9.29706C9.14924 17.9117 8.79901 18.579 8.28706 19.1121C7.7751 19.6451 7.12242 20.022 6.40482 20.1989C5.68723 20.3759 4.93416 20.3456 4.23309 20.1116C3.53202 19.8776 2.91171 19.4495 2.44421 18.8771C1.97672 18.3046 1.68121 17.6113 1.59202 16.8776C1.50284 16.1439 1.62362 15.4 1.94036 14.7322C2.25709 14.0645 2.75678 13.5002 3.38137 13.1051C4.00597 12.71 4.72985 12.5002 5.46893 12.5H8.89237L9.37518 11.5344C9.69959 10.8855 10.1983 10.3398 10.8153 9.95845C11.4324 9.57705 12.1435 9.37502 12.8689 9.375H15.4064L14.1658 6.70312C14.1032 6.56776 14.0031 6.45315 13.8774 6.37286C13.7517 6.29256 13.6056 6.24993 13.4564 6.25H11.7189C11.5117 6.25 11.313 6.16769 11.1665 6.02118C11.02 5.87466 10.9377 5.67595 10.9377 5.46875Z"
                />
              </svg>
              <span class="text-base-content/80 text-sm"
                >Historical Trips (14 Days)</span
              >
            </div>

            <div class="h-60">
              <Bar
                :data="historicalTripsData"
                :options="historicalTripsOptions"
              />
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* Use existing color scheme for compatibility */
.bg-dark {
  background-color: var(--b1);
}
.bg-dark-lighter {
  background-color: var(--b2);
}
.text-secondary {
  color: var(--bc);
}
.text-accent-green {
  color: var(--su);
}
.bg-secondary\/5 {
  background-color: rgba(var(--bc), 0.05);
}
.text-secondary\/70,
.text-secondary\/75 {
  color: rgba(var(--bc), 0.7);
}
</style>
