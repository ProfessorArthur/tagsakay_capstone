<template>
  <div class="bg-blue-50 min-h-screen">
    <div class="container mx-auto px-4 py-8">
      <h1 class="text-2xl font-bold mb-6 text-gray-900">
        ESP32 Device Management
      </h1>

      <!-- Registration Form -->
      <div
        class="bg-white p-6 rounded-lg shadow-md mb-8 border border-gray-200"
      >
        <h2 class="text-xl font-semibold mb-4 text-gray-900">
          Register New Device
        </h2>
        <!-- Error message -->
        <div
          v-if="error"
          class="mb-4 p-4 bg-red-50 border border-red-200 rounded-md"
        >
          <div class="flex">
            <svg
              xmlns="http://www.w3.org/2000/svg"
              class="h-5 w-5 text-red-400 mr-2"
              viewBox="0 0 20 20"
              fill="currentColor"
            >
              <path
                fill-rule="evenodd"
                d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z"
                clip-rule="evenodd"
              />
            </svg>
            <p class="text-red-800">{{ error }}</p>
          </div>
        </div>

        <div class="alert alert-info mb-4">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            fill="none"
            viewBox="0 0 24 24"
            class="stroke-current shrink-0 w-6 h-6"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z"
            ></path>
          </svg>
          <div>
            <span class="font-bold">Automatic API Key Generation:</span>
            <p class="mt-1">
              When you register a device, an API key will be automatically
              generated. You'll see the key after successful registration - make
              sure to save it.
            </p>
          </div>
        </div>
        <form @submit.prevent="registerDevice">
          <div class="grid grid-cols-1 gap-6 md:grid-cols-2">
            <div>
              <label class="block text-sm font-medium text-gray-900"
                >MAC Address</label
              >
              <input
                v-model="newDevice.macAddress"
                type="text"
                class="mt-1 block w-full rounded-md border-2 border-gray-400 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 bg-white text-gray-900"
                placeholder="Format: XX:XX:XX:XX:XX:XX"
                pattern="^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$"
                title="Please enter a valid MAC address (format: XX:XX:XX:XX:XX:XX)"
                required
              />
              <p class="mt-1 text-sm text-gray-800">
                Enter the MAC address of the ESP32 device
              </p>
            </div>

            <div>
              <label class="block text-sm font-medium text-gray-900"
                >Device Name</label
              >
              <input
                v-model="newDevice.name"
                type="text"
                class="mt-1 block w-full rounded-md border-2 border-gray-400 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 bg-white text-gray-900"
                required
              />
            </div>

            <div>
              <label class="block text-sm font-medium text-gray-900"
                >Location</label
              >
              <input
                v-model="newDevice.location"
                type="text"
                class="mt-1 block w-full rounded-md border-2 border-gray-400 shadow-sm focus:border-indigo-500 focus:ring-indigo-500 bg-white text-gray-900"
                required
              />
              <p class="mt-1 text-sm text-gray-800">
                Where this device is installed
              </p>
            </div>
          </div>

          <div class="mt-6">
            <button
              type="submit"
              class="inline-flex justify-center py-2 px-4 border border-transparent shadow-sm text-sm font-medium rounded-md text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500"
              :disabled="loading"
            >
              <span v-if="loading" class="mr-2">
                <svg
                  class="animate-spin -ml-1 mr-2 h-4 w-4 text-white"
                  xmlns="http://www.w3.org/2000/svg"
                  fill="none"
                  viewBox="0 0 24 24"
                >
                  <circle
                    class="opacity-25"
                    cx="12"
                    cy="12"
                    r="10"
                    stroke="currentColor"
                    stroke-width="4"
                  ></circle>
                  <path
                    class="opacity-75"
                    fill="currentColor"
                    d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
                  ></path>
                </svg>
              </span>
              Register Device
            </button>
          </div>
        </form>

        <!-- API Key Display -->
        <div
          v-if="createdApiKey"
          class="mt-6 p-4 bg-green-50 border border-green-200 rounded-lg"
        >
          <div class="flex items-center mb-2">
            <svg
              xmlns="http://www.w3.org/2000/svg"
              class="h-5 w-5 text-green-500 mr-2"
              viewBox="0 0 20 20"
              fill="currentColor"
            >
              <path
                fill-rule="evenodd"
                d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z"
                clip-rule="evenodd"
              />
            </svg>
            <h3 class="text-lg font-medium text-green-800">
              Device Registered Successfully!
            </h3>
          </div>
          <p class="text-sm text-green-700 mb-3">
            An API key was automatically generated for your device. Make sure to
            save it now - it won't be shown again!
          </p>
          <div
            class="bg-white p-3 rounded border border-green-300 font-mono text-sm break-all text-gray-900"
          >
            {{ createdApiKey }}
          </div>
          <div class="mt-3 flex justify-end">
            <button
              @click="copyToClipboard(createdApiKey)"
              class="inline-flex items-center px-3 py-1.5 border border-green-500 text-xs font-medium rounded text-green-700 bg-white hover:bg-green-50"
            >
              <svg
                xmlns="http://www.w3.org/2000/svg"
                class="h-4 w-4 mr-1"
                fill="none"
                viewBox="0 0 24 24"
                stroke="currentColor"
              >
                <path
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="2"
                  d="M8 5H6a2 2 0 00-2 2v12a2 2 0 002 2h10a2 2 0 002-2v-1M8 5a2 2 0 002 2h2a2 2 0 002-2M8 5a2 2 0 012-2h2a2 2 0 012 2m0 0h2a2 2 0 012 2v3m2 4H10m0 0l3-3m-3 3l3 3"
                />
              </svg>
              Copy API Key
            </button>
          </div>
        </div>
      </div>

      <!-- Devices List -->
      <div
        class="bg-white shadow-md rounded-lg overflow-hidden border border-gray-200"
      >
        <div class="px-4 py-5 sm:px-6 flex justify-between items-center">
          <div>
            <h2 class="text-lg font-medium text-gray-900">
              Registered Devices
            </h2>
            <p class="mt-1 max-w-2xl text-sm text-gray-800">
              List of all registered RFID readers
            </p>
          </div>
          <button
            @click="fetchDevices"
            class="inline-flex items-center px-3 py-2 border border-gray-300 shadow-sm text-sm leading-4 font-medium rounded-md text-gray-900 bg-gray-200 hover:bg-gray-300 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500"
            :disabled="loading"
          >
            <svg
              xmlns="http://www.w3.org/2000/svg"
              class="h-4 w-4 mr-1"
              :class="{ 'animate-spin': loading }"
              fill="none"
              viewBox="0 0 24 24"
              stroke="currentColor"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"
              />
            </svg>
            Refresh
          </button>
        </div>

        <div class="border-t border-gray-200">
          <div class="overflow-x-auto">
            <table class="min-w-full divide-y divide-gray-200">
              <thead class="bg-gray-100">
                <tr>
                  <th
                    scope="col"
                    class="px-6 py-3 text-left text-xs font-medium text-gray-900 uppercase tracking-wider"
                  >
                    Name
                  </th>
                  <th
                    scope="col"
                    class="px-6 py-3 text-left text-xs font-medium text-gray-900 uppercase tracking-wider"
                  >
                    MAC Address
                  </th>
                  <th
                    scope="col"
                    class="px-6 py-3 text-left text-xs font-medium text-gray-900 uppercase tracking-wider"
                  >
                    Location
                  </th>
                  <th
                    scope="col"
                    class="px-6 py-3 text-left text-xs font-medium text-gray-900 uppercase tracking-wider w-32"
                  >
                    API Status
                  </th>
                  <th
                    scope="col"
                    class="px-6 py-3 text-left text-xs font-medium text-gray-900 uppercase tracking-wider w-36"
                  >
                    Last Seen
                  </th>
                  <th
                    scope="col"
                    class="px-6 py-3 text-left text-xs font-medium text-gray-900 uppercase tracking-wider w-56"
                  >
                    Actions
                  </th>
                </tr>
              </thead>
              <tbody class="bg-white divide-y divide-gray-200">
                <tr
                  v-for="device in devices"
                  :key="device.id"
                  class="hover:bg-gray-50"
                >
                  <td class="px-6 py-4 whitespace-nowrap text-gray-900">
                    {{ device.name }}
                  </td>
                  <td
                    class="px-6 py-4 whitespace-nowrap font-mono text-sm text-gray-900"
                  >
                    {{ device.macAddress }}
                  </td>
                  <td class="px-6 py-4 whitespace-nowrap text-gray-900">
                    {{ device.location }}
                  </td>
                  <td class="px-6 py-4 whitespace-nowrap">
                    <div class="flex items-center">
                      <button
                        @click="toggleDeviceStatus(device)"
                        :class="[
                          'px-2 inline-flex text-xs leading-5 font-semibold rounded-full cursor-pointer',
                          device.isActive
                            ? 'bg-green-100 text-green-800 hover:bg-green-200'
                            : 'bg-red-100 text-red-800 hover:bg-red-200',
                        ]"
                      >
                        {{ device.isActive ? "Active" : "Inactive" }}
                      </button>
                      <span
                        v-if="device.registrationMode"
                        class="ml-2 px-2 inline-flex text-xs leading-5 font-semibold rounded-full bg-blue-100 text-blue-800"
                      >
                        Registration Mode
                      </span>
                    </div>
                  </td>
                  <td class="px-6 py-4 whitespace-nowrap">
                    <span v-if="device.lastSeen">{{
                      formatDate(device.lastSeen)
                    }}</span>
                    <span v-else class="text-red-600 font-medium">Never</span>
                  </td>
                  <td class="px-6 py-4 whitespace-nowrap">
                    <div class="flex space-x-2">
                      <button
                        v-if="!device.registrationMode"
                        @click="enableRegistrationMode(device.deviceId)"
                        class="px-3 py-1 text-xs font-medium rounded-md bg-indigo-100 text-indigo-800 hover:bg-indigo-200"
                      >
                        Enable Registration
                      </button>
                      <button
                        v-else
                        @click="disableRegistrationMode(device.deviceId)"
                        class="px-3 py-1 text-xs font-medium rounded-md bg-amber-100 text-amber-800 hover:bg-amber-200"
                      >
                        Disable Registration
                      </button>
                      <button
                        @click="confirmDelete(device)"
                        class="px-3 py-1 text-xs font-medium rounded-md bg-red-100 text-red-800 hover:bg-red-200"
                      >
                        Delete
                      </button>
                    </div>
                  </td>
                </tr>
                <tr v-if="devices.length === 0">
                  <td
                    colspan="6"
                    class="px-6 py-4 text-center text-sm text-gray-900 bg-white"
                  >
                    No devices registered yet
                  </td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <!-- API Keys Link -->
      <div class="mt-8 text-center p-6 bg-indigo-100 rounded-lg shadow-sm">
        <p class="mb-2 text-gray-900 font-medium">
          Need to manage device API keys or create keys for other services?
        </p>
        <router-link to="/apikeys" class="btn btn-primary">
          Go to API Key Management
        </router-link>
      </div>

      <!-- Success message -->
      <div
        v-if="success"
        class="fixed top-4 right-4 p-4 bg-green-100 border border-green-400 text-green-700 rounded-md shadow-md z-50 max-w-md"
      >
        <div class="flex">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            class="h-6 w-6 mr-2"
            fill="none"
            viewBox="0 0 24 24"
            stroke="currentColor"
          >
            <path
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="2"
              d="M5 13l4 4L19 7"
            />
          </svg>
          <p>{{ success }}</p>
        </div>
      </div>

      <!-- Delete Confirmation Modal -->
      <div
        v-if="showDeleteModal"
        class="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50"
      >
        <div
          class="bg-white rounded-lg max-w-md w-full p-6 shadow-xl border border-gray-200"
        >
          <h3 class="text-lg font-bold text-gray-900 mb-4">
            Confirm Device Deletion
          </h3>
          <p class="mb-6 text-gray-800">
            Are you sure you want to delete the device "<span
              class="font-semibold text-gray-900"
              >{{ deviceToDelete?.name }}</span
            >"? This action will also remove its associated API key and cannot
            be undone.
          </p>
          <div class="flex justify-end space-x-3">
            <button
              @click="showDeleteModal = false"
              class="px-4 py-2 bg-gray-200 hover:bg-gray-300 text-gray-900 rounded-md"
            >
              Cancel
            </button>
            <button
              @click="deleteDevice"
              class="px-4 py-2 bg-red-600 hover:bg-red-700 text-white rounded-md flex items-center"
              :disabled="loading"
            >
              <span v-if="loading" class="mr-2">
                <svg
                  class="animate-spin h-4 w-4"
                  xmlns="http://www.w3.org/2000/svg"
                  fill="none"
                  viewBox="0 0 24 24"
                >
                  <circle
                    class="opacity-25"
                    cx="12"
                    cy="12"
                    r="10"
                    stroke="currentColor"
                    stroke-width="4"
                  ></circle>
                  <path
                    class="opacity-75"
                    fill="currentColor"
                    d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
                  ></path>
                </svg>
              </span>
              Delete Device
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onMounted, onUnmounted } from "vue";
import deviceService, {
  type Device,
  type RegisterDeviceRequest,
} from "../services/device";

export default defineComponent({
  name: "DeviceManagement",

  setup() {
    const devices = ref<Device[]>([]);
    const newDevice = ref<RegisterDeviceRequest>({
      macAddress: "",
      name: "",
      location: "",
    });
    const loading = ref(false);
    const error = ref<string | null>(null);
    const success = ref<string | null>(null);
    const deviceToDelete = ref<Device | null>(null);
    const showDeleteModal = ref(false);

    // Fetch all devices when component mounts
    const fetchDevices = async () => {
      try {
        loading.value = true;
        devices.value = await deviceService.getAllDevices();
      } catch (err) {
        error.value = "Failed to fetch devices";
        console.error(err);
      } finally {
        loading.value = false;
      }
    };

    // For storing the newly created API key
    const createdApiKey = ref("");

    // Copy API key to clipboard
    const copyToClipboard = async (text: string) => {
      try {
        await navigator.clipboard.writeText(text);
        success.value = "API key copied to clipboard!";
        setTimeout(() => {
          if (success.value === "API key copied to clipboard!") {
            success.value = null;
          }
        }, 3000);
      } catch (err) {
        console.error("Failed to copy text: ", err);
        error.value =
          "Failed to copy to clipboard. Please try manually selecting and copying the text.";
      }
    };

    // Register a new device
    const registerDevice = async () => {
      try {
        // Basic validation
        if (
          !newDevice.value.macAddress.match(
            /^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/
          )
        ) {
          error.value =
            "Please enter a valid MAC address in format XX:XX:XX:XX:XX:XX";
          return;
        }

        if (!newDevice.value.name || newDevice.value.name.trim().length < 2) {
          error.value =
            "Please enter a valid device name (minimum 2 characters)";
          return;
        }

        if (
          !newDevice.value.location ||
          newDevice.value.location.trim().length < 2
        ) {
          error.value = "Please specify a location for this device";
          return;
        }

        loading.value = true;
        error.value = null;
        const response = await deviceService.registerDevice(newDevice.value);

        // Store API key for display
        if (response?.apiKey?.fullKey) {
          createdApiKey.value = response.apiKey.fullKey;
        }

        // Reset form
        newDevice.value = {
          macAddress: "",
          name: "",
          location: "",
        };

        // Refresh device list
        await fetchDevices();
      } catch (err) {
        error.value = "Failed to register device";
        console.error(err);
      } finally {
        loading.value = false;
      }
    };

    // Enable registration mode
    const enableRegistrationMode = async (deviceId: string) => {
      try {
        loading.value = true;
        await deviceService.enableRegistrationMode(deviceId);
        await fetchDevices();
      } catch (err) {
        error.value = "Failed to enable registration mode";
        console.error(err);
      } finally {
        loading.value = false;
      }
    };

    // Disable registration mode
    const disableRegistrationMode = async (deviceId: string) => {
      try {
        loading.value = true;
        await deviceService.disableRegistrationMode(deviceId);
        await fetchDevices();
      } catch (err) {
        error.value = "Failed to disable registration mode";
        console.error(err);
      } finally {
        loading.value = false;
      }
    };

    // Format date for display
    const formatDate = (dateString: string): string => {
      const date = new Date(dateString);
      return new Intl.DateTimeFormat("en-US", {
        dateStyle: "medium",
        timeStyle: "short",
      }).format(date);
    };

    // Confirm device deletion
    const confirmDelete = (device: Device) => {
      deviceToDelete.value = device;
      showDeleteModal.value = true;
    };

    // Delete a device
    const deleteDevice = async () => {
      if (!deviceToDelete.value) return;

      try {
        loading.value = true;
        error.value = null;

        await deviceService.deleteDevice(deviceToDelete.value.id);

        // Show success message
        success.value = `Device "${deviceToDelete.value.name}" has been deleted successfully`;

        // Remove from local array
        devices.value = devices.value.filter(
          (d) => d.id !== deviceToDelete.value?.id
        );

        // Close modal
        showDeleteModal.value = false;
        deviceToDelete.value = null;

        // Clear success message after 5 seconds
        setTimeout(() => {
          success.value = null;
        }, 5000);
      } catch (err) {
        console.error(err);
        error.value = "Failed to delete device. Please try again.";
      } finally {
        loading.value = false;
      }
    };

    // Toggle device active status
    const toggleDeviceStatus = async (device: Device) => {
      try {
        loading.value = true;
        error.value = null;

        // Toggle the status
        const newStatus = !device.isActive;
        await deviceService.updateDeviceStatus(device.id, {
          isActive: newStatus,
        });

        // Update local state
        const updatedDevice = devices.value.find((d) => d.id === device.id);
        if (updatedDevice) {
          updatedDevice.isActive = newStatus;
        }

        // Show brief success message
        success.value = `Device "${device.name}" ${
          newStatus ? "activated" : "deactivated"
        } successfully`;

        // Clear success message after 3 seconds
        setTimeout(() => {
          success.value = null;
        }, 3000);
      } catch (err) {
        console.error(err);
        error.value = `Failed to ${
          device.isActive ? "deactivate" : "activate"
        } device. Please try again.`;
      } finally {
        loading.value = false;
      }
    };

    // Set up auto-refresh every 60 seconds
    let refreshInterval: number | null = null;
    onMounted(() => {
      fetchDevices();
      refreshInterval = window.setInterval(() => {
        // Only refresh if not in the middle of another operation
        if (!loading.value) {
          fetchDevices();
        }
      }, 60000); // Refresh every minute
    });

    // Clean up interval when component is unmounted
    onUnmounted(() => {
      if (refreshInterval) {
        clearInterval(refreshInterval);
        refreshInterval = null;
      }
    });

    return {
      devices,
      newDevice,
      loading,
      error,
      success,
      createdApiKey,
      deviceToDelete,
      showDeleteModal,
      fetchDevices,
      registerDevice,
      enableRegistrationMode,
      disableRegistrationMode,
      formatDate,
      copyToClipboard,
      confirmDelete,
      deleteDevice,
      toggleDeviceStatus,
    };
  },
});
</script>
