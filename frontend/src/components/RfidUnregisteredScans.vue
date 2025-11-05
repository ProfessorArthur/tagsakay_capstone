<template>
  <div class="bg-white shadow rounded-lg p-4">
    <div class="flex justify-between items-center mb-4">
      <h3 class="text-lg font-semibold text-gray-700">
        Unregistered RFID Scans
      </h3>
      <span v-if="loading" class="inline-flex items-center">
        <span class="animate-pulse relative inline-flex h-3 w-3 mr-2">
          <span
            class="animate-ping absolute inline-flex h-full w-full rounded-full bg-green-400 opacity-75"
          ></span>
          <span
            class="relative inline-flex rounded-full h-3 w-3 bg-green-500"
          ></span>
        </span>
        <span class="text-sm text-green-600">Scanning</span>
      </span>
    </div>

    <div
      v-if="error"
      class="bg-red-100 border-l-4 border-red-500 text-red-700 p-4 mb-4"
    >
      <p>{{ error }}</p>
    </div>

    <div v-if="scans.length === 0" class="text-center py-6 text-gray-500">
      <svg
        xmlns="http://www.w3.org/2000/svg"
        class="h-12 w-12 mx-auto mb-2 text-gray-400"
        fill="none"
        viewBox="0 0 24 24"
        stroke="currentColor"
      >
        <path
          stroke-linecap="round"
          stroke-linejoin="round"
          stroke-width="2"
          d="M12 4v1m6 11h2m-6 0h-2v4m0-11v3m0 0h.01M12 12h4.01M16 20h4M4 12h4m12 0h.01M5 8h2a1 1 0 001-1V5a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1zm12 0h2a1 1 0 001-1V5a1 1 0 00-1-1h-2a1 1 0 00-1 1v2a1 1 0 001 1zM5 20h2a1 1 0 001-1v-2a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1z"
        />
      </svg>
      <p>Scan an unregistered RFID card to detect it</p>
      <p class="text-xs mt-2">New scans will appear here automatically</p>
    </div>

    <div v-else class="relative overflow-x-auto max-h-96 overflow-y-auto">
      <table class="w-full text-sm text-left text-gray-500">
        <thead class="text-xs text-gray-700 uppercase bg-gray-100 sticky top-0">
          <tr>
            <th scope="col" class="px-4 py-3">Time</th>
            <th scope="col" class="px-4 py-3">Tag ID</th>
            <th scope="col" class="px-4 py-3">Location</th>
            <th scope="col" class="px-4 py-3">Actions</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="scan in scans"
            :key="scan.id"
            class="border-b transition-colors duration-300 hover:bg-green-50"
          >
            <td class="px-4 py-3">{{ formatTime(scan.scanTime) }}</td>
            <td class="px-4 py-3 font-mono">{{ scan.tagId }}</td>
            <td class="px-4 py-3">{{ scan.location || "Unknown" }}</td>
            <td class="px-4 py-3">
              <button
                @click="selectTag(scan.tagId)"
                class="bg-blue-500 hover:bg-blue-600 text-white px-3 py-1 rounded-md text-xs font-semibold transition-colors duration-150"
              >
                Select
              </button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted, ref, defineEmits } from "vue";
import rfidService, { type RfidScan } from "../services/rfid";

const props = defineProps({
  refreshInterval: {
    type: Number,
    default: 2000, // 2 seconds - faster than the live scans component for better responsiveness
  },
});

const emit = defineEmits(["selectTag"]);

// Reactive state
const scans = ref<any[]>([]);
const loading = ref(false);
const error = ref<string | null>(null);

// Polling interval
let pollingInterval: number | null = null;

// Format timestamp to readable format
const formatTime = (timestamp: string) => {
  const date = new Date(timestamp);
  return date.toLocaleTimeString();
};

// Function to get recent unregistered scans
const getRecentUnregisteredScans = async () => {
  try {
    loading.value = true;
    error.value = null;

    const response = await rfidService.getRecentUnregisteredScans();

    if (response.success && response.data && Array.isArray(response.data)) {
      // Add new scans without removing old ones, but avoid duplicates
      const existingIds = new Set(scans.value.map((scan) => scan.id));
      const newScans = response.data.filter(
        (scan: RfidScan) => !existingIds.has(scan.id)
      );

      // Add new scans to the beginning of the array
      if (newScans.length > 0) {
        scans.value = [...newScans, ...scans.value];

        // Limit the total number of displayed scans
        if (scans.value.length > 10) {
          scans.value = scans.value.slice(0, 10);
        }
      }
    }
  } catch (err: any) {
    error.value =
      err.response?.data?.message || "Failed to fetch unregistered scans";
    console.error("Error fetching unregistered RFID scans:", err);
  } finally {
    loading.value = false;
  }
};

// Function to select a tag and emit the event to the parent component
const selectTag = (tagId: string) => {
  emit("selectTag", tagId);
};

// Start polling when component is mounted
onMounted(() => {
  getRecentUnregisteredScans();

  pollingInterval = setInterval(() => {
    getRecentUnregisteredScans();
  }, props.refreshInterval) as unknown as number;
});

// Stop polling when component is unmounted
onUnmounted(() => {
  if (pollingInterval) {
    clearInterval(pollingInterval);
  }
});
</script>
