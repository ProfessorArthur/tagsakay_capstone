<script setup lang="ts">
import { ref, onMounted } from "vue";
import userService from "../services/user";

interface User {
  id: number;
  name: string;
  email: string;
  role: string;
  isActive: boolean;
  rfids?: Array<{
    id: number;
    tagId: string;
    isActive: boolean;
  }>;
}

// State management
const users = ref<User[]>([]);
const loading = ref(true);
const error = ref<string | null>(null);
const showModal = ref(false);
const editingUser = ref<User | null>(null);
const isCreating = ref(false);

// Form data
const formData = ref({
  name: "",
  email: "",
  password: "",
  role: "driver",
  isActive: true,
});

// Fetch users on component mount
onMounted(async () => {
  await fetchUsers();
});

// Fetch users from API
async function fetchUsers() {
  loading.value = true;
  error.value = null;

  try {
    const response = await userService.getUsers();
    users.value = response.data;
  } catch (err) {
    error.value = "Failed to load users. Please try again.";
    console.error("Error fetching users:", err);
  } finally {
    loading.value = false;
  }
}

// Open modal for creating a new user
function openCreateModal() {
  isCreating.value = true;
  editingUser.value = null;
  formData.value = {
    name: "",
    email: "",
    password: "",
    role: "driver",
    isActive: true,
  };
  showModal.value = true;
}

// Open modal for editing an existing user
function openEditModal(user: User) {
  isCreating.value = false;
  editingUser.value = user;
  formData.value = {
    name: user.name,
    email: user.email,
    password: "", // Don't prefill password for security
    role: user.role,
    isActive: user.isActive,
  };
  showModal.value = true;
}

// Handle form submission (create or update)
async function submitForm() {
  try {
    if (isCreating.value) {
      // Create new user
      await userService.createUser(formData.value);
    } else if (editingUser.value) {
      // Update existing user
      await userService.updateUser(editingUser.value.id, formData.value);
    }

    // Close modal and refresh data
    showModal.value = false;
    await fetchUsers();
  } catch (err) {
    console.error("Error saving user:", err);
    error.value = "Failed to save user. Please try again.";
  }
}

// Delete a user
async function deleteUser(userId: number) {
  if (!confirm("Are you sure you want to delete this user?")) {
    return;
  }

  try {
    await userService.deleteUser(userId);
    await fetchUsers();
  } catch (err) {
    console.error("Error deleting user:", err);
    error.value = "Failed to delete user. Please try again.";
  }
}

// Toggle user active status
async function toggleUserStatus(user: User) {
  try {
    await userService.updateUser(user.id, { isActive: !user.isActive });
    await fetchUsers();
  } catch (err) {
    console.error("Error updating user status:", err);
    error.value = "Failed to update user status. Please try again.";
  }
}
</script>

<template>
  <div class="p-4">
    <div class="flex justify-between items-center mb-6">
      <h1 class="text-2xl font-bold">User Management</h1>
      <button @click="openCreateModal" class="btn btn-primary">
        <i class="fas fa-plus mr-2"></i> Add User
      </button>
    </div>

    <!-- Alert for errors -->
    <div v-if="error" class="alert alert-error mb-4">
      <div class="flex-1">
        <label>{{ error }}</label>
      </div>
    </div>

    <!-- Loading state -->
    <div v-if="loading" class="flex justify-center my-8">
      <span class="loading loading-spinner loading-lg"></span>
    </div>

    <!-- Users table -->
    <div v-else class="overflow-x-auto">
      <table class="table w-full">
        <thead>
          <tr>
            <th>Name</th>
            <th>Email</th>
            <th>Role</th>
            <th>Status</th>
            <th>RFID Tags</th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="user in users"
            :key="user.id"
            :class="{ 'opacity-50': !user.isActive }"
          >
            <td>{{ user.name }}</td>
            <td>{{ user.email }}</td>
            <td>
              <span
                class="badge"
                :class="{
                  'badge-primary': user.role === 'admin',
                  'badge-secondary': user.role === 'superadmin',
                  'badge-accent': user.role === 'driver',
                }"
                >{{ user.role }}</span
              >
            </td>
            <td>
              <span
                class="badge"
                :class="user.isActive ? 'badge-success' : 'badge-error'"
              >
                {{ user.isActive ? "Active" : "Inactive" }}
              </span>
            </td>
            <td>
              <span
                v-if="user.rfids && user.rfids.length"
                class="badge badge-info"
              >
                {{ user.rfids.length }} tags
              </span>
              <span v-else>No tags</span>
            </td>
            <td>
              <div class="flex space-x-2">
                <button
                  @click="openEditModal(user)"
                  class="btn btn-sm btn-info"
                >
                  Edit
                </button>
                <button
                  @click="toggleUserStatus(user)"
                  class="btn btn-sm"
                  :class="user.isActive ? 'btn-warning' : 'btn-success'"
                >
                  {{ user.isActive ? "Disable" : "Enable" }}
                </button>
                <button
                  @click="deleteUser(user.id)"
                  class="btn btn-sm btn-error"
                >
                  Delete
                </button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <!-- User Modal (Create/Edit) -->
    <div v-if="showModal" class="modal modal-open">
      <div class="modal-box">
        <h3 class="font-bold text-lg">
          {{ isCreating ? "Create New User" : "Edit User" }}
        </h3>

        <form @submit.prevent="submitForm" class="mt-4 space-y-4">
          <div class="form-control">
            <label class="label">
              <span class="label-text">Name</span>
            </label>
            <input
              v-model="formData.name"
              type="text"
              placeholder="Full Name"
              class="input input-bordered"
              required
            />
          </div>

          <div class="form-control">
            <label class="label">
              <span class="label-text">Email</span>
            </label>
            <input
              v-model="formData.email"
              type="email"
              placeholder="Email"
              class="input input-bordered"
              required
            />
          </div>

          <div class="form-control">
            <label class="label">
              <span class="label-text"
                >Password
                {{ !isCreating ? "(Leave empty to keep current)" : "" }}</span
              >
            </label>
            <input
              v-model="formData.password"
              type="password"
              placeholder="Password"
              class="input input-bordered"
              :required="isCreating"
            />
          </div>

          <div class="form-control">
            <label class="label">
              <span class="label-text">Role</span>
            </label>
            <select v-model="formData.role" class="select select-bordered">
              <option value="driver">Driver</option>
              <option value="admin">Admin</option>
              <option value="superadmin">Super Admin</option>
            </select>
          </div>

          <div class="form-control">
            <label class="label cursor-pointer">
              <span class="label-text">Active</span>
              <input
                v-model="formData.isActive"
                type="checkbox"
                class="toggle toggle-success"
              />
            </label>
          </div>

          <div class="modal-action">
            <button
              type="button"
              @click="showModal = false"
              class="btn btn-ghost"
            >
              Cancel
            </button>
            <button type="submit" class="btn btn-primary">Save</button>
          </div>
        </form>
      </div>
    </div>
  </div>
</template>
