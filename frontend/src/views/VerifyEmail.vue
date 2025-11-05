<script setup lang="ts">
import { ref, onMounted, computed } from "vue";
import { useRouter, useRoute } from "vue-router";
import authService from "../services/auth";

const router = useRouter();
const route = useRoute();

const email = ref("");
const code = ref("");
const loading = ref(false);
const error = ref("");
const verifyingFromLink = ref(false);
const resendLoading = ref(false);
const resendCountdown = ref(0);

onMounted(async () => {
  // If email in query params, auto-fill and start verification
  if (route.query.email && route.query.code) {
    email.value = route.query.email as string;
    code.value = route.query.code as string;
    verifyingFromLink.value = true;

    // Auto-verify
    setTimeout(() => {
      verify();
    }, 500);
  } else if (route.query.email) {
    email.value = route.query.email as string;
  }
});

const verify = async () => {
  if (!email.value || !code.value) {
    error.value = "Email and verification code are required";
    return;
  }

  loading.value = true;
  error.value = "";

  try {
    const response = await authService.verifyEmail(email.value, code.value);

    // Save user data and token
    authService.saveUserData(response);

    // Redirect to dashboard
    router.push("/dashboard");
  } catch (err: any) {
    error.value = err.message || "Verification failed. Please try again.";
    verifyingFromLink.value = false;
  } finally {
    loading.value = false;
  }
};

const resendCode = async () => {
  if (!email.value) {
    error.value = "Email is required";
    return;
  }

  resendLoading.value = true;
  error.value = "";

  try {
    // Note: You'll need to create a /api/auth/resend-verification endpoint
    // For now, just show a message
    error.value =
      "Resend functionality coming soon. Please check your spam folder.";
    resendCountdown.value = 60;

    const interval = setInterval(() => {
      resendCountdown.value--;
      if (resendCountdown.value <= 0) {
        clearInterval(interval);
      }
    }, 1000);
  } catch (err: any) {
    error.value = err.message || "Failed to resend code";
  } finally {
    resendLoading.value = false;
  }
};

const codeLength = computed(() => code.value.length);
const canVerify = computed(() => email.value && code.value.length === 6);
</script>

<template>
  <div class="flex min-h-[85vh] items-center justify-center p-6">
    <div class="card bg-base-200 shadow-xl w-full max-w-lg">
      <div class="card-body p-8">
        <h2 class="card-title text-3xl font-bold justify-center mb-2">
          Verify Your Email
        </h2>
        <p class="text-center text-sm opacity-70 mb-6">
          We've sent a 6-digit code to your email address
        </p>

        <div v-if="error" class="alert alert-error mb-6" role="alert">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            class="h-6 w-6 shrink-0 stroke-current"
            fill="none"
            viewBox="0 0 24 24"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M10 14l2-2m0 0l2-2m-2 2l-2-2m2 2l2 2m7-2a9 9 0 11-18 0 9 9 0 0118 0z"
            />
          </svg>
          <span>{{ error }}</span>
        </div>

        <div
          v-if="verifyingFromLink"
          class="alert alert-info mb-6"
          role="alert"
        >
          <svg
            xmlns="http://www.w3.org/2000/svg"
            class="h-6 w-6 shrink-0 stroke-current"
            fill="none"
            viewBox="0 0 24 24"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
            />
          </svg>
          <span>Verifying your email...</span>
        </div>

        <form @submit.prevent="verify" class="space-y-6">
          <div class="form-control w-full">
            <label class="label" for="email">
              <span class="label-text text-base font-medium"
                >Email Address</span
              >
            </label>
            <input
              type="email"
              id="email"
              v-model="email"
              placeholder="your@email.com"
              class="input input-bordered input-primary w-full"
              required
              :disabled="verifyingFromLink"
            />
          </div>

          <div class="form-control w-full">
            <label class="label" for="code">
              <span class="label-text text-base font-medium"
                >Verification Code</span
              >
              <span class="label-text-alt">{{ codeLength }}/6</span>
            </label>
            <input
              type="text"
              id="code"
              v-model="code"
              placeholder="000000"
              maxlength="6"
              class="input input-bordered input-primary w-full text-center tracking-widest text-2xl font-mono"
              required
              :disabled="verifyingFromLink"
              pattern="[0-9]*"
              inputmode="numeric"
            />
            <label class="label">
              <span class="label-text-alt"
                >Enter the 6-digit code from your email</span
              >
            </label>
          </div>

          <div class="card-actions justify-end mt-8 space-x-2">
            <button
              type="submit"
              class="btn btn-primary flex-1"
              :disabled="loading || !canVerify || verifyingFromLink"
            >
              <span
                class="loading loading-spinner loading-sm"
                v-if="loading || verifyingFromLink"
              ></span>
              <span v-if="loading || verifyingFromLink">Verifying...</span>
              <span v-else>Verify Email</span>
            </button>
          </div>
        </form>

        <div class="divider my-6">OR</div>

        <div class="space-y-4">
          <p class="text-sm text-center opacity-70">Didn't receive the code?</p>
          <button
            type="button"
            class="btn btn-outline btn-primary w-full"
            @click="resendCode"
            :disabled="resendLoading || resendCountdown > 0"
          >
            <span
              class="loading loading-spinner loading-sm"
              v-if="resendLoading"
            ></span>
            <span v-if="resendCountdown > 0"
              >Resend in {{ resendCountdown }}s</span
            >
            <span v-else>Resend Code</span>
          </button>
        </div>

        <div class="text-center mt-6">
          <router-link to="/login" class="link link-hover link-primary text-sm">
            Back to Login
          </router-link>
        </div>
      </div>
    </div>
  </div>
</template>
