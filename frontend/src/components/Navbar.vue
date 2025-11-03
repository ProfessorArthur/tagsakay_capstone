<script setup lang="ts">
import { ref, watchEffect } from "vue";
import { useRouter } from "vue-router";
import authService from "../services/auth";

const router = useRouter();
const user = ref(authService.getUser());
const isAdmin = ref(authService.isAdmin());

// Update user info reactively
watchEffect(() => {
  user.value = authService.getUser();
  isAdmin.value = authService.isAdmin();
});

const logout = () => {
  authService.logout();
  user.value = null;
  isAdmin.value = false;

  // Explicitly navigate to login page
  router.push("/login");

  // Force page refresh to ensure all reactive state is updated
  window.dispatchEvent(new Event("storage"));
};
</script>

<template>
  <div class="navbar bg-base-200">
    <div class="navbar-start">
      <div class="dropdown">
        <div tabindex="0" role="button" class="btn btn-ghost lg:hidden">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            class="h-5 w-5"
            fill="none"
            viewBox="0 0 24 24"
            stroke="currentColor"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M4 6h16M4 12h8m-8 6h16"
            />
          </svg>
        </div>
        <ul
          tabindex="0"
          class="menu menu-sm dropdown-content mt-3 z-10 p-2 shadow bg-base-100 rounded-box w-52"
        >
          <li><router-link to="/dashboard">Dashboard</router-link></li>
          <li v-if="isAdmin">
            <router-link to="/rfid">RFID Management</router-link>
          </li>
          <li v-if="isAdmin">
            <router-link to="/rfid-cards">RFID Cards</router-link>
          </li>
          <li v-if="isAdmin">
            <router-link to="/users">User Management</router-link>
          </li>
          <li v-if="isAdmin">
            <router-link to="/devices">ESP32 Devices</router-link>
          </li>
          <li v-if="isAdmin">
            <router-link to="/apikeys">API Keys</router-link>
          </li>
        </ul>
      </div>
      <router-link to="/" class="btn btn-ghost text-xl">TagSakay</router-link>
    </div>
    <div class="navbar-center hidden lg:flex">
      <ul class="menu menu-horizontal px-1">
        <li><router-link to="/dashboard">Dashboard</router-link></li>
        <li v-if="isAdmin">
          <router-link to="/rfid">RFID Management</router-link>
        </li>
        <li v-if="isAdmin">
          <router-link to="/rfid-cards">RFID Cards</router-link>
        </li>
        <li v-if="isAdmin">
          <router-link to="/users">User Management</router-link>
        </li>
        <li v-if="isAdmin">
          <router-link to="/devices">ESP32 Devices</router-link>
        </li>
        <li v-if="isAdmin">
          <router-link to="/apikeys">API Keys</router-link>
        </li>
      </ul>
    </div>
    <div class="navbar-end">
      <div class="dropdown dropdown-end">
        <div tabindex="0" role="button" class="btn btn-ghost">
          <div class="avatar placeholder">
            <div class="bg-neutral text-neutral-content rounded-full w-8">
              <span>{{ user?.name.charAt(0).toUpperCase() }}</span>
            </div>
          </div>
          <span class="ml-2">{{ user?.name }}</span>
        </div>
        <ul
          tabindex="0"
          class="menu menu-sm dropdown-content mt-3 z-10 p-2 shadow bg-base-100 rounded-box w-52"
        >
          <li>
            <span class="font-bold">{{ user?.email }}</span>
            <span class="badge badge-sm">{{ user?.role }}</span>
          </li>
          <li><a @click="logout">Logout</a></li>
        </ul>
      </div>
    </div>
  </div>
</template>
