<template>
  <div class="p-6">
    <div class="mb-8">
      <h1 class="text-3xl font-bold text-base-content mb-2">
        WebSocket Testing
      </h1>
      <p class="text-base-content/70">Test real-time RFID scan functionality</p>
    </div>

    <!-- Connection Status -->
    <div class="bg-base-200 rounded-lg p-6 shadow-sm mb-6">
      <h3 class="text-lg font-semibold mb-4">Connection Status</h3>
      <div class="flex items-center gap-4">
        <div class="flex items-center gap-2">
          <div
            class="w-3 h-3 rounded-full"
            :class="{
              'bg-success animate-pulse': isConnected,
              'bg-warning animate-spin': isConnecting,
              'bg-error': !isConnected && !isConnecting,
            }"
          ></div>
          <span class="font-medium">{{ connectionStatus }}</span>
        </div>
        <div class="text-sm text-base-content/60">
          {{
            reconnectAttempts > 0
              ? `Reconnect attempts: ${reconnectAttempts}`
              : ""
          }}
        </div>
      </div>
      <div v-if="connectionError" class="mt-2 text-error text-sm">
        Error: {{ connectionError }}
      </div>
    </div>

    <!-- Device Simulator -->
    <div class="bg-base-200 rounded-lg p-6 shadow-sm mb-6">
      <h3 class="text-lg font-semibold mb-4">Device Simulator</h3>
      <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
        <div>
          <label class="block text-sm font-medium mb-2">Device ID</label>
          <input
            v-model="deviceId"
            type="text"
            class="input input-bordered w-full"
            placeholder="e.g., TEST_DEVICE_001"
          />
        </div>
        <div>
          <label class="block text-sm font-medium mb-2">RFID Tag ID</label>
          <input
            v-model="tagId"
            type="text"
            class="input input-bordered w-full"
            placeholder="e.g., RFID123456"
          />
        </div>
      </div>

      <div class="mt-4">
        <label class="block text-sm font-medium mb-2"
          >Location (Optional)</label
        >
        <input
          v-model="location"
          type="text"
          class="input input-bordered w-full"
          placeholder="e.g., Main Gate"
        />
      </div>

      <div class="flex gap-4 mt-6">
        <button
          @click="connectDevice"
          :disabled="isConnected || isConnecting || !deviceId"
          class="btn btn-primary"
        >
          <span
            v-if="isConnecting"
            class="loading loading-spinner loading-sm"
          ></span>
          Connect Device
        </button>

        <button
          @click="disconnectDevice"
          :disabled="!isConnected"
          class="btn btn-secondary"
        >
          Disconnect
        </button>

        <button
          @click="simulateScan"
          :disabled="!isConnected || !tagId"
          class="btn btn-accent"
        >
          Simulate RFID Scan
        </button>
      </div>
    </div>

    <!-- Recent Messages -->
    <div class="bg-base-200 rounded-lg p-6 shadow-sm mb-6">
      <h3 class="text-lg font-semibold mb-4">WebSocket Messages</h3>
      <div class="max-h-80 overflow-y-auto space-y-2">
        <div
          v-for="(message, index) in messages"
          :key="index"
          class="p-3 bg-base-100 rounded-lg text-sm font-mono"
        >
          <div class="flex justify-between items-start mb-1">
            <span
              class="font-bold"
              :class="
                message.type === 'sent' ? 'text-primary' : 'text-secondary'
              "
            >
              {{ message.type === "sent" ? "SENT" : "RECEIVED" }}
            </span>
            <span class="text-xs text-base-content/40">{{
              message.timestamp
            }}</span>
          </div>
          <pre class="whitespace-pre-wrap text-xs">{{
            JSON.stringify(message.data, null, 2)
          }}</pre>
        </div>
      </div>
      <button @click="clearMessages" class="btn btn-outline btn-sm mt-4">
        Clear Messages
      </button>
    </div>

    <!-- Real-time Stats -->
    <div class="bg-base-200 rounded-lg p-6 shadow-sm">
      <h3 class="text-lg font-semibold mb-4">Real-time Statistics</h3>
      <div class="grid grid-cols-2 md:grid-cols-4 gap-4">
        <div class="text-center">
          <p class="text-2xl font-bold text-primary">{{ stats.totalScans }}</p>
          <p class="text-sm text-base-content/60">Total Scans</p>
        </div>
        <div class="text-center">
          <p class="text-2xl font-bold text-success">
            {{ stats.successfulScans }}
          </p>
          <p class="text-sm text-base-content/60">Successful</p>
        </div>
        <div class="text-center">
          <p class="text-2xl font-bold text-warning">
            {{ stats.unregisteredScans }}
          </p>
          <p class="text-sm text-base-content/60">Unregistered</p>
        </div>
        <div class="text-center">
          <p class="text-2xl font-bold text-info">
            {{ stats.connectedDevicesCount }}
          </p>
          <p class="text-sm text-base-content/60">Connected Devices</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from "vue";
import { useDeviceWebSocket } from "../composables/useDeviceWebSocket";
import { useRealTimeScans } from "../composables/useRealTimeScans";

const deviceId = ref("TEST_DEVICE_001");
const tagId = ref("RFID123456");
const location = ref("Test Location");

// Messages log
const messages = ref<
  Array<{
    type: "sent" | "received";
    data: any;
    timestamp: string;
  }>
>([]);

// Device WebSocket connection
const deviceWebSocket = ref<ReturnType<typeof useDeviceWebSocket> | null>(null);

// Real-time scans monitoring
const {
  stats,
  startListening: startRealTimeListening,
  stopListening: stopRealTimeListening,
} = useRealTimeScans();

// Computed properties
const isConnected = computed(() => deviceWebSocket.value?.isConnected || false);
const isConnecting = computed(
  () => deviceWebSocket.value?.isConnecting || false
);
const connectionError = computed(
  () => deviceWebSocket.value?.connectionError || null
);
const reconnectAttempts = computed(() => 0); // TODO: Get from WebSocket state

const connectionStatus = computed(() => {
  if (isConnected.value) return "Connected";
  if (isConnecting.value) return "Connecting...";
  return "Disconnected";
});

// Device connection methods
const connectDevice = () => {
  if (!deviceId.value) return;

  deviceWebSocket.value = useDeviceWebSocket({
    deviceId: deviceId.value,
  });

  if (deviceWebSocket.value) {
    deviceWebSocket.value.connectDevice();
  }

  addMessage("sent", {
    action: "connect",
    deviceId: deviceId.value,
  });
};

const disconnectDevice = () => {
  if (deviceWebSocket.value) {
    deviceWebSocket.value.disconnectDevice();
    deviceWebSocket.value = null;
  }

  addMessage("sent", {
    action: "disconnect",
  });
};

const simulateScan = () => {
  if (!deviceWebSocket.value || !tagId.value) return;

  const success = deviceWebSocket.value.scanRfid(tagId.value, location.value);

  if (success) {
    addMessage("sent", {
      action: "scan",
      tagId: tagId.value,
      location: location.value,
      timestamp: Date.now(),
    });
  }
};

// Message handling
const addMessage = (type: "sent" | "received", data: any) => {
  messages.value.unshift({
    type,
    data,
    timestamp: new Date().toLocaleTimeString(),
  });

  // Keep only last 50 messages
  if (messages.value.length > 50) {
    messages.value = messages.value.slice(0, 50);
  }
};

const clearMessages = () => {
  messages.value = [];
};

// Event listeners for WebSocket messages
const handleDeviceScanResponse = (event: Event) => {
  const customEvent = event as CustomEvent;
  addMessage("received", customEvent.detail);
};

const handleRfidScan = (event: Event) => {
  const customEvent = event as CustomEvent;
  addMessage("received", {
    type: "real_time_scan",
    ...customEvent.detail,
  });
};

onMounted(() => {
  // Start real-time monitoring
  startRealTimeListening();

  // Add event listeners
  window.addEventListener("device-scan-response", handleDeviceScanResponse);
  window.addEventListener("rfid-scan", handleRfidScan);
});

onUnmounted(() => {
  // Clean up
  stopRealTimeListening();
  if (deviceWebSocket.value) {
    deviceWebSocket.value.disconnectDevice();
  }

  // Remove event listeners
  window.removeEventListener("device-scan-response", handleDeviceScanResponse);
  window.removeEventListener("rfid-scan", handleRfidScan);
});
</script>
