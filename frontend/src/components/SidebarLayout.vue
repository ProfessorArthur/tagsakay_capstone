<script setup lang="ts">
import { ref, watchEffect, computed } from "vue";
import { useRouter, useRoute } from "vue-router";
import authService from "../services/auth";

const router = useRouter();
const route = useRoute();
const user = ref(authService.getUser());
const sidebarOpen = ref(false);

// Update user info reactively
watchEffect(() => {
  user.value = authService.getUser();
});

// Computed properties for role checking
const isAdmin = computed(
  () => user.value?.role === "admin" || user.value?.role === "superadmin"
);

const userRoleDisplay = computed(() => {
  const role = user.value?.role;
  if (role === "superadmin") return "Super Admin";
  if (role === "admin") return "Admin";
  return "Driver";
});

// Navigation items based on role
const navigationItems = computed(() => {
  const baseItems = [
    {
      name: "Dashboard",
      path: "/dashboard",
      icon: "M3 13h8V3H3v10zm0 8h8v-6H3v6zm10 0h8V11h-8v10zm0-18v6h8V3h-8z",
    },
  ];

  if (isAdmin.value) {
    baseItems.push(
      {
        name: "Register Device",
        path: "/devices/register",
        icon: "M12 2C8.13 2 5 5.13 5 9c0 5.25 7 13 7 13s7-7.75 7-13c0-3.87-3.13-7-7-7zm0 9.5c-1.38 0-2.5-1.12-2.5-2.5S10.62 6.5 12 6.5s2.5 1.12 2.5 2.5S13.38 11.5 12 11.5z",
      },
      {
        name: "RFID Cards",
        path: "/rfid",
        icon: "M20 4H4c-1.11 0-1.99.89-1.99 2L2 18c0 1.11.89 2 2 2h16c1.11 0 2-.89 2-2V6c0-1.11-.89-2-2-2zm0 14H4v-6h16v6zm0-10H4V6h16v2z",
      },
      {
        name: "User Management",
        path: "/users",
        icon: "M12 12c2.21 0 4-1.79 4-4s-1.79-4-4-4-4 1.79-4 4 1.79 4 4 4zm0 2c-2.67 0-8 1.34-8 4v2h16v-2c0-2.66-5.33-4-8-4z",
      },
      {
        name: "ESP32 Devices",
        path: "/devices",
        icon: "M9 4C9 3.45 9.45 3 10 3H14C14.55 3 15 3.45 15 4V6H17C18.1 6 19 6.9 19 8V19C19 20.1 18.1 21 17 21H7C5.9 21 5 20.1 5 19V8C5 6.9 5.9 6 7 6H9V4ZM7 8V19H17V8H7ZM10 10H14V12H10V10ZM10 14H14V16H10V14Z",
      },
      {
        name: "API Keys",
        path: "/apikeys",
        icon: "M12.65 10C11.7 7.31 8.9 5.5 5.77 6.12C2.1 6.8-.5 9.7.18 13.37C.85 17.04 3.75 19.64 7.42 18.97C10.55 18.35 12.35 15.55 11.65 12.37L14.6 10.2C14.8 10.05 15.05 10 15.3 10H16.3C16.85 10 17.3 10.45 17.3 11V12C17.3 12.55 17.75 13 18.3 13S19.3 12.55 19.3 12V10.5C19.3 9.12 18.18 8 16.8 8H15.8C15.35 8 14.9 8.2 14.6 8.5L12.65 10ZM7.77 15.97C6.95 15.97 6.27 15.29 6.27 14.47S6.95 12.97 7.77 12.97 9.27 13.65 9.27 14.47 8.59 15.97 7.77 15.97Z",
      },
      {
        name: "WebSocket Test",
        path: "/websocket-test",
        icon: "M12 2C17.52 2 22 6.48 22 12C22 17.52 17.52 22 12 22C6.48 22 2 17.52 2 12C2 6.48 6.48 2 12 2ZM12 20C16.42 20 20 16.42 20 12C20 7.58 16.42 4 12 4C7.58 4 4 7.58 4 12C4 16.42 7.58 20 12 20ZM13 7H11V11H7V13H11V17H13V13H17V11H13V7Z",
      }
    );
  }

  return baseItems;
});

// Check if route is active
const isActiveRoute = (path: string) => {
  return route.path === path;
};

// Toggle sidebar for mobile
const toggleSidebar = () => {
  sidebarOpen.value = !sidebarOpen.value;
};

// Close sidebar when clicking outside (mobile)
const closeSidebar = () => {
  sidebarOpen.value = false;
};

// Handle logout
const handleLogout = () => {
  authService.logout();
  router.push("/login");
  window.dispatchEvent(new Event("storage"));
};

// Search functionality
const searchQuery = ref("");
</script>

<template>
  <div class="min-h-screen bg-base-100 flex">
    <!-- Mobile overlay -->
    <div
      v-if="sidebarOpen"
      class="fixed inset-0 bg-black bg-opacity-50 z-40 lg:hidden"
      @click="closeSidebar"
    ></div>

    <!-- Sidebar -->
    <div
      class="fixed inset-y-0 left-0 z-50 w-64 bg-base-200 transform transition-transform duration-300 ease-in-out lg:translate-x-0 lg:static lg:inset-0"
      :class="{
        'translate-x-0': sidebarOpen,
        '-translate-x-full': !sidebarOpen,
      }"
    >
      <!-- Sidebar Header -->
      <div class="flex items-center justify-between h-16 px-6 bg-base-300">
        <router-link to="/dashboard" class="text-xl font-bold text-primary">
          TagSakay
        </router-link>
        <!-- Close button for mobile -->
        <button @click="closeSidebar" class="lg:hidden btn btn-ghost btn-sm">
          <svg
            class="w-5 h-5"
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M6 18L18 6M6 6l12 12"
            ></path>
          </svg>
        </button>
      </div>

      <!-- User Profile Section -->
      <div class="p-6 border-b border-base-300">
        <div class="flex items-center space-x-3">
          <div class="avatar placeholder">
            <div class="bg-primary text-primary-content rounded-full w-10">
              <span class="text-sm font-medium">{{
                user?.name?.charAt(0).toUpperCase()
              }}</span>
            </div>
          </div>
          <div class="flex-1 min-w-0">
            <div class="text-sm font-medium text-base-content truncate">
              {{ user?.name || "User" }}
            </div>
            <div class="text-xs text-base-content/70">
              {{ userRoleDisplay }}
            </div>
          </div>
        </div>
      </div>

      <!-- Navigation Menu -->
      <nav class="flex-1 px-4 py-6 space-y-2">
        <router-link
          v-for="item in navigationItems"
          :key="item.path"
          :to="item.path"
          @click="closeSidebar"
          class="flex items-center px-4 py-3 text-sm font-medium rounded-lg transition-colors duration-200"
          :class="{
            'bg-primary text-primary-content': isActiveRoute(item.path),
            'text-base-content/70 hover:text-base-content hover:bg-base-300':
              !isActiveRoute(item.path),
          }"
        >
          <svg class="w-5 h-5 mr-3" fill="currentColor" viewBox="0 0 24 24">
            <path :d="item.icon" />
          </svg>
          {{ item.name }}
        </router-link>
      </nav>

      <!-- Logout Button -->
      <div class="p-4 border-t border-base-300">
        <button
          @click="handleLogout"
          class="flex items-center w-full px-4 py-3 text-sm font-medium text-base-content/70 hover:text-base-content hover:bg-base-300 rounded-lg transition-colors duration-200"
        >
          <svg
            class="w-5 h-5 mr-3"
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1"
            ></path>
          </svg>
          Logout
        </button>
      </div>
    </div>

    <!-- Main Content Area -->
    <div class="flex-1 flex flex-col lg:ml-0">
      <!-- Top Bar (Mobile Menu + Actions) -->
      <header
        class="h-16 bg-base-100 border-b border-base-300 flex items-center justify-between px-4 lg:px-8"
      >
        <!-- Mobile menu button -->
        <button @click="toggleSidebar" class="lg:hidden btn btn-ghost btn-sm">
          <svg
            class="w-6 h-6"
            fill="none"
            stroke="currentColor"
            viewBox="0 0 24 24"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M4 6h16M4 12h16M4 18h16"
            ></path>
          </svg>
        </button>

        <!-- Search Bar (Desktop) -->
        <div class="hidden lg:flex flex-1 max-w-lg">
          <div class="relative w-full">
            <svg
              class="absolute left-3 top-1/2 transform -translate-y-1/2 w-5 h-5 text-base-content/50"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M21 21l-6-6m2-5a7 7 0 11-14 0 7 7 0 0114 0z"
              ></path>
            </svg>
            <input
              v-model="searchQuery"
              type="text"
              placeholder="Search..."
              class="input input-bordered w-full pl-10 bg-base-200"
            />
          </div>
        </div>

        <!-- Actions -->
        <div class="flex items-center space-x-3">
          <!-- Refresh Button -->
          <button class="btn btn-ghost btn-sm">
            <svg
              class="w-5 h-5"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
              ></path>
            </svg>
          </button>

          <!-- User Menu -->
          <div class="dropdown dropdown-end">
            <div tabindex="0" role="button" class="btn btn-ghost btn-sm">
              <div class="avatar placeholder">
                <div class="bg-primary text-primary-content rounded-full w-8">
                  <span class="text-xs">{{
                    user?.name?.charAt(0).toUpperCase()
                  }}</span>
                </div>
              </div>
            </div>
            <ul
              tabindex="0"
              class="dropdown-content z-[1] menu p-2 shadow bg-base-200 rounded-box w-52"
            >
              <li>
                <span class="font-medium">{{ user?.email }}</span>
              </li>
              <li>
                <span class="badge badge-sm">{{ user?.role }}</span>
              </li>
              <div class="divider my-1"></div>
              <li><a>Profile</a></li>
              <li><a>Settings</a></li>
              <li><a @click="handleLogout">Logout</a></li>
            </ul>
          </div>
        </div>
      </header>

      <!-- Page Content -->
      <main class="flex-1 overflow-y-auto bg-base-100">
        <div class="p-4 lg:p-8">
          <slot />
        </div>
      </main>
    </div>
  </div>
</template>

<style scoped>
/* Ensure smooth transitions */
.transition-transform {
  transition-property: transform;
  transition-timing-function: cubic-bezier(0.4, 0, 0.2, 1);
  transition-duration: 300ms;
}

/* Custom scrollbar for sidebar */
nav::-webkit-scrollbar {
  width: 4px;
}

nav::-webkit-scrollbar-track {
  background: transparent;
}

nav::-webkit-scrollbar-thumb {
  background: rgba(var(--bc), 0.2);
  border-radius: 2px;
}

nav::-webkit-scrollbar-thumb:hover {
  background: rgba(var(--bc), 0.3);
}
</style>
