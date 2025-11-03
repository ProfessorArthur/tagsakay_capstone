<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from "vue";
import rfidService from "../services/rfid";
import userService from "../services/user";
import deviceService from "../services/device";
import type { RegisterRfidData } from "../services/rfid";

// State
const rfidCards = ref<any[]>([]);
const users = ref<any[]>([]);
const loading = ref(true);
const error = ref("");
const success = ref("");
const tab = ref("all"); // 'all', 'registered', 'unregistered'
const activeDevices = ref<any[]>([]);

// Modal state
const showRegisterModal = ref(false);
const selectedCard = ref<any>(null);
const awaitingConfirmation = ref(false);
const pendingRegistration = ref<string | null>(null);

// Form data for registering/editing
const formData = ref<RegisterRfidData & { metadata: { notes: string } }>({
  tagId: "",
  userId: undefined,
  metadata: {
    notes: "",
  },
});

// Computed properties for filtering cards
const registeredCards = computed(() =>
  rfidCards.value.filter((card) => card.isRegistered)
);

const unregisteredCards = computed(() =>
  rfidCards.value.filter((card) => !card.isRegistered)
);

const displayedCards = computed(() => {
  if (tab.value === "registered") return registeredCards.value;
  if (tab.value === "unregistered") return unregisteredCards.value;
  return rfidCards.value;
});

// Variable to store the scan check interval
let scanCheckInterval: number | null = null;

// Load initial data
onMounted(async () => {
  await Promise.all([loadRfidCards(), loadUsers(), loadActiveDevices()]);

  // Set up polling for new scans every 3 seconds
  scanCheckInterval = window.setInterval(async () => {
    await checkForNewScans();
  }, 3000);
});

// Clean up when the component is unmounted
onUnmounted(() => {
  // Clean up polling interval
  if (pollingInterval) {
    clearInterval(pollingInterval);
    pollingInterval = null;
  }

  // Clean up scan check interval
  if (scanCheckInterval) {
    clearInterval(scanCheckInterval);
    scanCheckInterval = null;
  }
});

// Handle modal close
const handleModalClose = async () => {
  showRegisterModal.value = false;
  pendingRegistration.value = null;
  awaitingConfirmation.value = false;

  // Stop polling if active
  stopPollingForNewTag();

  // Turn off registration mode on devices if needed
  if (activeDevices.value.length > 0) {
    const onlineDevices = activeDevices.value.filter(
      (device: any) => device.status === "online"
    );

    if (onlineDevices.length > 0) {
      try {
        await deviceService.disableRegistrationMode(onlineDevices[0].id);

        console.log("Registration mode disabled on device");
      } catch (err) {
        console.error("Error disabling registration mode:", err);
      }
    }
  }
}; // Load active devices
const loadActiveDevices = async () => {
  try {
    const response = await deviceService.getActiveDevices();
    activeDevices.value = response;
  } catch (err) {
    console.error("Error fetching active devices:", err);
  }
};

// Load RFID cards
const loadRfidCards = async () => {
  loading.value = true;
  error.value = "";

  try {
    const response = await rfidService.getAllRfidCards();
    rfidCards.value = response;
  } catch (err) {
    console.error("Error fetching RFID cards:", err);
    error.value = "Failed to load RFID cards";
  } finally {
    loading.value = false;
  }
};

// Load users for assignment
const loadUsers = async () => {
  try {
    const response = await userService.getUsers();
    users.value = response;
  } catch (err) {
    console.error("Error fetching users:", err);
    error.value = "Failed to load users";
  }
};

// Open modal to register an unregistered card
const openRegisterModal = async (card: any = null) => {
  selectedCard.value = card;

  if (card) {
    formData.value = {
      tagId: card.tagId,
      userId: card.user?.id || undefined,
      metadata: {
        notes: card.metadata?.notes || "",
      },
    };
    showRegisterModal.value = true;
  } else {
    // For new card registration, we'll wait for a scan from ESP32
    formData.value = {
      tagId: "",
      userId: undefined,
      metadata: {
        notes: "",
      },
    };

    // Show the modal with scanning state
    pendingRegistration.value = "new";
    awaitingConfirmation.value = true;
    showRegisterModal.value = true;

    // Notify device to enter scanning mode
    if (activeDevices.value.length > 0) {
      const onlineDevices = activeDevices.value.filter(
        (device) => device.status === "online"
      );

      if (onlineDevices.length > 0) {
        try {
          const deviceId = onlineDevices[0].id;
          await deviceService.enableRegistrationMode(deviceId, "new"); // "new" is a special value indicating we're waiting for any card

          success.value = "Waiting for new RFID tag scan...";
          console.log(
            `Notified device ${deviceId} to enter scan mode for new tag`
          );

          // Start polling for new scans
          startPollingForNewTag();
        } catch (err) {
          console.error("Error setting device to scan mode:", err);
          error.value = "Failed to activate scanner. Please try again.";
          showRegisterModal.value = false;
          pendingRegistration.value = null;
          awaitingConfirmation.value = false;
        }
      } else {
        error.value =
          "No online RFID devices found. Please check device connections.";
        showRegisterModal.value = false;
      }
    } else {
      error.value = "No RFID devices found. Please check your hardware setup.";
      showRegisterModal.value = false;
    }
  }
};

// Poll specifically for a new, unknown tag
let pollingInterval: number | null = null;
let pollFailCount = 0; // Counter for consecutive failed polling attempts

const startPollingForNewTag = () => {
  // Clear any existing interval
  if (pollingInterval) {
    clearInterval(pollingInterval);
  }

  // Reset fail count
  pollFailCount = 0;

  // Start a new polling interval
  pollingInterval = window.setInterval(async () => {
    try {
      // Get the most recent unregistered scans
      const response = await rfidService.getRecentUnregisteredScans();

      // Reset fail count on success
      pollFailCount = 0;

      if (response && response.length > 0) {
        // Take the most recent scan
        const latestScan = response[0];

        // Stop polling
        stopPollingForNewTag();

        // Update form with the scanned tag
        formData.value.tagId = latestScan.tagId;

        // Update state
        pendingRegistration.value = latestScan.tagId;
        awaitingConfirmation.value = false;
        success.value = `New tag detected: ${latestScan.tagId}! Please complete the registration form.`;

        // Turn off registration mode on devices
        if (activeDevices.value.length > 0) {
          const onlineDevices = activeDevices.value.filter(
            (device: any) => device.status === "online"
          );

          if (onlineDevices.length > 0) {
            try {
              // Disable registration mode on the first online device
              const deviceId = onlineDevices[0].id;
              await deviceService.disableRegistrationMode(deviceId);
            } catch (err) {
              console.error("Error disabling registration mode:", err);
            }
          }
        }
      }
    } catch (err) {
      console.error("Error polling for new tags:", err);

      // Increment fail count
      pollFailCount++;

      // If we've failed 5 consecutive times (5 seconds), show error message
      if (pollFailCount >= 5) {
        error.value = "Lost connection to the server. Please try again.";
        stopPollingForNewTag();
        handleModalClose();
      }
    }
  }, 1000); // Check every second
};
const stopPollingForNewTag = () => {
  if (pollingInterval) {
    clearInterval(pollingInterval);
    pollingInterval = null;
  }
};

// Check for newly scanned cards (polling)
const checkForNewScans = async () => {
  // Skip if we're not awaiting confirmation
  if (!awaitingConfirmation.value || !pendingRegistration.value) return;

  // Skip for new tag detection as that's handled by startPollingForNewTag
  if (pendingRegistration.value === "new") return;

  try {
    // Check if there's been a recent scan of the pending tag
    const response = await rfidService.checkRecentScan(
      pendingRegistration.value
    );

    // If a recent scan was found, complete the registration
    if (response.success && response.found) {
      // It's a confirmation tap - complete the registration
      await registerRfid();
      awaitingConfirmation.value = false;
      pendingRegistration.value = null;
      success.value = "RFID card confirmed and registered successfully!";

      // Update cards list
      await loadRfidCards();
    }
  } catch (err) {
    console.error("Error checking for recent scans:", err);
  }
};

// Start the registration process for an unregistered card
const startRegistration = async (card: any) => {
  // Store the card we're registering
  selectedCard.value = card;
  pendingRegistration.value = card.tagId;

  // Set up the form data
  formData.value = {
    tagId: card.tagId,
    userId: undefined,
    metadata: {
      notes: "",
    },
  };

  // Find online devices to notify about registration mode
  if (activeDevices.value.length > 0) {
    const onlineDevices = activeDevices.value.filter(
      (device) => device.status === "online"
    );

    if (onlineDevices.length > 0) {
      try {
        // Notify the first online device to go into registration mode
        // In a more complex system, you might want to select a specific device or notify multiple devices
        const deviceId = onlineDevices[0].id;

        await deviceService.enableRegistrationMode(deviceId, card.tagId);

        console.log(
          `Notified device ${deviceId} to enter registration mode for tag ${card.tagId}`
        );
      } catch (err) {
        console.error("Error setting device to registration mode:", err);
      }
    }
  }

  // Open the registration modal
  showRegisterModal.value = true;
};

// Prepare for confirmation tap
const prepareForConfirmation = async () => {
  // Validate form before proceeding
  if (!formData.value.tagId) {
    error.value = "Tag ID is required";
    return;
  }

  // Set awaiting confirmation state
  awaitingConfirmation.value = true;

  // Find online devices to notify about registration mode
  if (activeDevices.value.length > 0) {
    const onlineDevices = activeDevices.value.filter(
      (device) => device.status === "online"
    );

    if (onlineDevices.length > 0) {
      try {
        // Notify the first online device to go into registration mode
        const deviceId = onlineDevices[0].id;

        await deviceService.setRegistrationMode({
          deviceId,
          enabled: true,
          tagId: formData.value.tagId,
        });

        console.log(
          `Notified device ${deviceId} to enter registration mode for tag ${formData.value.tagId}`
        );
      } catch (err) {
        console.error("Error setting device to registration mode:", err);
      }
    }
  }

  // Show instructions to the user
  success.value = `Form data saved! Please tap the RFID card "${pendingRegistration.value}" again to confirm and complete registration.`;
};

// Register or update an RFID card
const registerRfid = async () => {
  try {
    await rfidService.registerRfid(formData.value);
    showRegisterModal.value = false;
    awaitingConfirmation.value = false;
    pendingRegistration.value = null;
    success.value = "RFID card registered successfully";

    // Turn off registration mode on devices
    if (activeDevices.value.length > 0) {
      const onlineDevices = activeDevices.value.filter(
        (device) => device.status === "online"
      );

      if (onlineDevices.length > 0) {
        try {
          // Disable registration mode on the first online device
          const deviceId = onlineDevices[0].id;

          await deviceService.setRegistrationMode({
            deviceId,
            enabled: false,
          });

          console.log(`Notified device ${deviceId} to exit registration mode`);
        } catch (err) {
          console.error("Error disabling registration mode:", err);
        }
      }
    }

    await loadRfidCards();
  } catch (err) {
    console.error("Error registering RFID card:", err);
    error.value = "Failed to register RFID card";
  }
};

// Toggle RFID card active status
const toggleCardStatus = async (card: any) => {
  try {
    await rfidService.updateRfidStatus(card.id, !card.isActive);
    success.value = `Card ${card.tagId} ${
      card.isActive ? "deactivated" : "activated"
    } successfully`;
    await loadRfidCards();
  } catch (err) {
    console.error("Error updating RFID status:", err);
    error.value = "Failed to update RFID status";
  }
};

// Format date for display
const formatDate = (dateString: string | null | undefined) => {
  if (!dateString) return "Never";
  const date = new Date(dateString);
  return date.toLocaleString();
};
</script>

<template>
  <div class="p-4">
    <h1 class="text-2xl font-bold mb-6">RFID Card Management</h1>

    <!-- Success/Error alerts -->
    <div v-if="success" class="alert alert-success mb-4">
      {{ success }}
    </div>

    <div v-if="error" class="alert alert-error mb-4">
      {{ error }}
    </div>

    <!-- Tab navigation -->
    <div class="tabs mb-4">
      <a
        class="tab tab-bordered"
        :class="{ 'tab-active': tab === 'all' }"
        @click="tab = 'all'"
      >
        All Cards
      </a>
      <a
        class="tab tab-bordered"
        :class="{ 'tab-active': tab === 'registered' }"
        @click="tab = 'registered'"
      >
        Registered Cards
      </a>
      <a
        class="tab tab-bordered"
        :class="{ 'tab-active': tab === 'unregistered' }"
        @click="tab = 'unregistered'"
      >
        Unregistered Cards
      </a>
    </div>

    <!-- Action buttons -->
    <div class="flex justify-between mb-4">
      <div>
        <span class="text-sm"> {{ displayedCards.length }} cards found </span>
      </div>
      <div class="flex space-x-2">
        <button @click="loadRfidCards" class="btn btn-sm">Refresh</button>
        <button @click="openRegisterModal()" class="btn btn-sm btn-primary">
          Register New Card
        </button>
      </div>
    </div>

    <!-- Loading state -->
    <div v-if="loading" class="flex justify-center my-8">
      <span class="loading loading-spinner loading-lg"></span>
    </div>

    <!-- RFID cards table -->
    <div v-else-if="displayedCards.length > 0" class="overflow-x-auto">
      <table class="table w-full">
        <thead>
          <tr>
            <th>Tag ID</th>
            <th>Status</th>
            <th>User</th>
            <th>Last Seen</th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="card in displayedCards"
            :key="card.tagId"
            :class="{ 'opacity-50': card.isRegistered && !card.isActive }"
          >
            <td>{{ card.tagId }}</td>
            <td>
              <div v-if="card.isRegistered">
                <span
                  class="badge"
                  :class="card.isActive ? 'badge-success' : 'badge-error'"
                >
                  {{ card.isActive ? "Active" : "Inactive" }}
                </span>
              </div>
              <div v-else>
                <span class="badge badge-warning">Unregistered</span>
              </div>
            </td>
            <td>
              <div v-if="card.user">
                {{ card.user.name }}
                <span class="badge badge-sm ml-1">{{ card.user.role }}</span>
              </div>
              <div v-else>Not assigned</div>
            </td>
            <td>
              {{ formatDate(card.lastSeen || card.updatedAt) }}
            </td>
            <td>
              <div class="flex space-x-2">
                <!-- Register unregistered card -->
                <button
                  v-if="!card.isRegistered"
                  @click="startRegistration(card)"
                  class="btn btn-sm btn-primary"
                >
                  Register
                </button>

                <!-- Toggle registered card status -->
                <button
                  v-if="card.isRegistered"
                  @click="toggleCardStatus(card)"
                  class="btn btn-sm"
                  :class="card.isActive ? 'btn-warning' : 'btn-success'"
                >
                  {{ card.isActive ? "Deactivate" : "Activate" }}
                </button>

                <!-- Edit registered card -->
                <button
                  v-if="card.isRegistered"
                  @click="openRegisterModal(card)"
                  class="btn btn-sm btn-info"
                >
                  Edit
                </button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <!-- Empty state -->
    <div v-else class="text-center my-8 p-8 bg-base-200 rounded-lg">
      <p v-if="tab === 'unregistered'">
        No unregistered RFID cards detected yet.
      </p>
      <p v-else-if="tab === 'registered'">No registered RFID cards found.</p>
      <p v-else>No RFID cards found.</p>
    </div>

    <!-- Register/Edit modal -->
    <div v-if="showRegisterModal" class="modal modal-open">
      <div class="modal-box">
        <h3 class="font-bold text-lg">
          {{
            pendingRegistration === "new"
              ? "Scan New RFID Card"
              : selectedCard
              ? "Register RFID Card"
              : "Register New RFID Card"
          }}
        </h3>

        <!-- Waiting for new card scan -->
        <div
          v-if="pendingRegistration === 'new' && awaitingConfirmation"
          class="my-4 p-4 bg-info text-info-content rounded-lg"
        >
          <h4 class="font-bold">Waiting for RFID card scan...</h4>
          <p class="mt-2">
            Please tap any unregistered RFID card on the scanner.
          </p>
          <div class="mt-4 flex justify-center">
            <span class="loading loading-spinner loading-lg"></span>
          </div>
        </div>

        <!-- Waiting for confirmation tap -->
        <div
          v-else-if="
            awaitingConfirmation &&
            pendingRegistration &&
            pendingRegistration !== 'new'
          "
          class="my-4 p-4 bg-warning text-warning-content rounded-lg"
        >
          <h4 class="font-bold">Waiting for confirmation tap...</h4>
          <p class="mt-2">
            Please tap the RFID card "{{ pendingRegistration }}" again to
            confirm and complete registration.
          </p>
        </div>

        <!-- Registration form -->
        <form
          v-if="
            !pendingRegistration ||
            pendingRegistration !== 'new' ||
            !awaitingConfirmation
          "
          @submit.prevent="
            awaitingConfirmation ? registerRfid() : prepareForConfirmation()
          "
          class="mt-4 space-y-4"
        >
          <div class="form-control">
            <label class="label">
              <span class="label-text">Tag ID</span>
            </label>
            <input
              v-model="formData.tagId"
              type="text"
              placeholder="RFID Tag ID"
              class="input input-bordered"
              :readonly="!!selectedCard || !!formData.tagId"
              required
            />
          </div>

          <div class="form-control">
            <label class="label">
              <span class="label-text">Assign to User (Optional)</span>
            </label>
            <select v-model="formData.userId" class="select select-bordered">
              <option :value="undefined">Not assigned</option>
              <option v-for="user in users" :key="user.id" :value="user.id">
                {{ user.name }} ({{ user.email }})
              </option>
            </select>
          </div>

          <div class="form-control">
            <label class="label">
              <span class="label-text">Notes</span>
            </label>
            <textarea
              v-model="formData.metadata.notes"
              class="textarea textarea-bordered"
              placeholder="Optional notes about this card"
            ></textarea>
          </div>

          <div class="modal-action">
            <button
              type="button"
              @click="handleModalClose"
              class="btn btn-ghost"
            >
              Cancel
            </button>
            <button type="submit" class="btn btn-primary">
              {{
                awaitingConfirmation
                  ? "Complete Registration"
                  : "Continue & Wait for Tap"
              }}
            </button>
          </div>
        </form>
      </div>
    </div>
  </div>
</template>
