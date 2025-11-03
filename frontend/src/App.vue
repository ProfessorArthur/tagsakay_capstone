<script setup lang="ts">
import { RouterView, useRouter, useRoute } from "vue-router";
import { ref, watchEffect, onMounted, computed } from "vue";
import Navbar from "./components/Navbar.vue";
import authService from "./services/auth";

const router = useRouter();
const route = useRoute();
const isLoggedIn = ref(authService.isLoggedIn());

// Check if current route is the dashboard (for special layout)
const isDashboardRoute = computed(() => {
  return route.path === "/dashboard";
});

// Listen for changes in localStorage to update isLoggedIn reactively
window.addEventListener("storage", () => {
  isLoggedIn.value = authService.isLoggedIn();
  checkAuthentication();
});

// Function to check authentication and redirect if needed
const checkAuthentication = () => {
  isLoggedIn.value = authService.isLoggedIn();

  // If not logged in and not on login or register page, redirect to login
  if (
    !isLoggedIn.value &&
    router.currentRoute.value.path !== "/login" &&
    router.currentRoute.value.path !== "/register"
  ) {
    router.push("/login");
  }
};

// Watch for route changes and update isLoggedIn
watchEffect(checkAuthentication);

// Initial check on component mount
onMounted(checkAuthentication);
</script>

<template>
  <div class="min-h-screen bg-base-100">
    <!-- Show the navbar only when logged in AND not on dashboard route -->
    <Navbar v-if="isLoggedIn && !isDashboardRoute" />

    <!-- For dashboard, don't wrap in container since it has its own layout -->
    <template v-if="isDashboardRoute">
      <RouterView />
    </template>

    <!-- For other pages, maintain current layout with container -->
    <main v-else class="container mx-auto px-4 py-8">
      <RouterView />
    </main>
  </div>
</template>

<style>
body {
  font-family: "Inter", sans-serif;
}
</style>
