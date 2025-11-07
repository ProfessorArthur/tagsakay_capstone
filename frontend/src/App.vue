<script setup lang="ts">
import { RouterView, useRouter, useRoute } from "vue-router";
import { ref, watchEffect, onMounted, computed } from "vue";
import SidebarLayout from "./components/SidebarLayout.vue";
import authService from "./services/auth";

const router = useRouter();
const route = useRoute();
const isLoggedIn = ref(authService.isLoggedIn());

// Check if current route requires authentication
const isAuthRoute = computed(() => {
  return Boolean(route.meta.requiresAuth);
});

// Listen for changes in localStorage to update isLoggedIn reactively
window.addEventListener("storage", () => {
  isLoggedIn.value = authService.isLoggedIn();
  checkAuthentication();
});

// Function to check authentication and redirect if needed
const checkAuthentication = () => {
  isLoggedIn.value = authService.isLoggedIn();

  const requiresAuth = Boolean(route.meta.requiresAuth);
  const requiresGuest = Boolean(route.meta.requiresGuest);

  if (!isLoggedIn.value && requiresAuth) {
    router.push("/login");
    return;
  }

  if (isLoggedIn.value && requiresGuest) {
    router.push("/dashboard");
  }
};

// Watch for route changes and update isLoggedIn
watchEffect(checkAuthentication);

// Initial check on component mount
onMounted(checkAuthentication);
</script>

<template>
  <div class="min-h-screen bg-base-100">
    <!-- Use sidebar layout for authenticated routes -->
    <SidebarLayout v-if="isLoggedIn && isAuthRoute">
      <RouterView />
    </SidebarLayout>

    <!-- Use simple layout for login/register pages -->
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
