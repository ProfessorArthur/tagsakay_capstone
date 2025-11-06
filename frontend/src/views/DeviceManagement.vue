<template>
  <div class="bg-blue-50 min-h-screen">
    <div class="container mx-auto px-4 py-8">
      <h1 class="text-2xl font-bold mb-6 text-gray-900">
        ESP32 Device Management
      </h1>

      <!-- Quick Registration Link -->
      <div
        class="bg-white p-6 rounded-lg shadow-md mb-8 border border-gray-200"
      >
        <h2 class="text-xl font-semibold mb-4 text-gray-900">
          Device Registration
        </h2>
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
            <span class="font-bold">Dedicated Registration Flow:</span>
            <p class="mt-1">
              Use our streamlined device registration wizard for the best
              experience with enhanced validation, API key management, and
              connectivity testing.
            </p>
          </div>
        </div>
        <div class="flex items-center justify-center">
          <router-link
            to="/devices/register"
            class="inline-flex items-center px-6 py-3 border border-transparent text-base font-medium rounded-md text-white bg-indigo-600 hover:bg-indigo-700 focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-indigo-500"
          >
            <svg
              xmlns="http://www.w3.org/2000/svg"
              class="h-5 w-5 mr-2"
              fill="none"
              viewBox="0 0 24 24"
              stroke="currentColor"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M12 6v6m0 0v6m0-6h6m-6 0H6"
              />
            </svg>
            Register New Device
          </router-link>
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
                  :key="device.deviceId"
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
import deviceService, { type Device } from "../services/device";

export default defineComponent({
  name: "DeviceManagement",

  setup() {
    const devices = ref<Device[]>([]);
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

        await deviceService.deleteDevice(deviceToDelete.value.deviceId);

        // Show success message
        success.value = `Device "${deviceToDelete.value.name}" has been deleted successfully`;

        // Remove from local array
        devices.value = devices.value.filter(
          (d) => d.deviceId !== deviceToDelete.value?.deviceId
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
        await deviceService.updateDeviceStatus(device.deviceId, {
          isActive: newStatus,
        });

        // Update local state
        const updatedDevice = devices.value.find(
          (d) => d.deviceId === device.deviceId
        );
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
      loading,
      error,
      success,
      deviceToDelete,
      showDeleteModal,
      fetchDevices,
      enableRegistrationMode,
      disableRegistrationMode,
      formatDate,
      confirmDelete,
      deleteDevice,
      toggleDeviceStatus,
    };
  },
});
</script>
