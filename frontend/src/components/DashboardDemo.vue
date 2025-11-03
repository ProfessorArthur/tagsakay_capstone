<script setup lang="ts">
// This component is a more comprehensive dashboard that could be used in place of the simpler Dashboard.vue
// It includes various statistics and visualizations relevant to the RFID system
</script>

<template>
  <div>
    <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4 mb-8">
      <div class="stats shadow">
        <div class="stat">
          <div class="stat-figure text-primary">
            <svg
              xmlns="http://www.w3.org/2000/svg"
              fill="none"
              viewBox="0 0 24 24"
              class="inline-block w-8 h-8 stroke-current"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M4.318 6.318a4.5 4.5 0 000 6.364L12 20.364l7.682-7.682a4.5 4.5 0 00-6.364-6.364L12 7.636l-1.318-1.318a4.5 4.5 0 00-6.364 0z"
              ></path>
            </svg>
          </div>
          <div class="stat-title">Total Scans</div>
          <div class="stat-value text-primary">125</div>
          <div class="stat-desc">↗︎ 14% (30 days)</div>
        </div>
      </div>

      <div class="stats shadow">
        <div class="stat">
          <div class="stat-figure text-secondary">
            <svg
              xmlns="http://www.w3.org/2000/svg"
              fill="none"
              viewBox="0 0 24 24"
              class="inline-block w-8 h-8 stroke-current"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M13 10V3L4 14h7v7l9-11h-7z"
              ></path>
            </svg>
          </div>
          <div class="stat-title">Active RFID Tags</div>
          <div class="stat-value text-secondary">42</div>
          <div class="stat-desc">↗︎ 7% (30 days)</div>
        </div>
      </div>

      <div class="stats shadow">
        <div class="stat">
          <div class="stat-figure text-info">
            <svg
              xmlns="http://www.w3.org/2000/svg"
              fill="none"
              viewBox="0 0 24 24"
              class="inline-block w-8 h-8 stroke-current"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M12 6V4m0 2a2 2 0 100 4m0-4a2 2 0 110 4m-6 8a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4m6 6v10m6-2a2 2 0 100-4m0 4a2 2 0 110-4m0 4v2m0-6V4"
              ></path>
            </svg>
          </div>
          <div class="stat-title">Registered Drivers</div>
          <div class="stat-value text-info">28</div>
          <div class="stat-desc">↗︎ 10% (30 days)</div>
        </div>
      </div>

      <div class="stats shadow">
        <div class="stat">
          <div class="stat-figure text-success">
            <svg
              xmlns="http://www.w3.org/2000/svg"
              fill="none"
              viewBox="0 0 24 24"
              class="inline-block w-8 h-8 stroke-current"
            >
              <path
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="2"
                d="M5 8h14M5 8a2 2 0 110-4h14a2 2 0 110 4M5 8v10a2 2 0 002 2h10a2 2 0 002-2V8m-9 4h4"
              ></path>
            </svg>
          </div>
          <div class="stat-title">API Requests</div>
          <div class="stat-value text-success">1,200</div>
          <div class="stat-desc">↗︎ 25% (30 days)</div>
        </div>
      </div>
    </div>

    <div class="grid grid-cols-1 lg:grid-cols-3 gap-6 mb-8">
      <div class="card bg-base-200 shadow-xl lg:col-span-2">
        <div class="card-body">
          <h2 class="card-title">Recent RFID Scans</h2>
          <div class="overflow-x-auto">
            <table class="table table-zebra">
              <thead>
                <tr>
                  <th>Timestamp</th>
                  <th>RFID Tag</th>
                  <th>User</th>
                  <th>Event Type</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td>2025-09-04 08:15:22</td>
                  <td>RFID12345</td>
                  <td>John Doe</td>
                  <td>entry</td>
                  <td><span class="badge badge-success">success</span></td>
                </tr>
                <tr>
                  <td>2025-09-04 08:10:15</td>
                  <td>RFID67890</td>
                  <td>Jane Smith</td>
                  <td>exit</td>
                  <td><span class="badge badge-success">success</span></td>
                </tr>
                <tr>
                  <td>2025-09-04 08:05:47</td>
                  <td>RFID24680</td>
                  <td>Bob Johnson</td>
                  <td>entry</td>
                  <td><span class="badge badge-success">success</span></td>
                </tr>
                <tr>
                  <td>2025-09-04 08:01:33</td>
                  <td>RFID13579</td>
                  <td>Alice Brown</td>
                  <td>unknown</td>
                  <td><span class="badge badge-error">failed</span></td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <div class="card bg-base-200 shadow-xl">
        <div class="card-body">
          <h2 class="card-title">System Status</h2>
          <div class="my-4">
            <div class="flex justify-between mb-2">
              <span>API Server</span>
              <span class="badge badge-success">Online</span>
            </div>
            <div class="flex justify-between mb-2">
              <span>Database</span>
              <span class="badge badge-success">Connected</span>
            </div>
            <div class="flex justify-between mb-2">
              <span>RFID Reader #1</span>
              <span class="badge badge-success">Active</span>
            </div>
            <div class="flex justify-between mb-2">
              <span>RFID Reader #2</span>
              <span class="badge badge-success">Active</span>
            </div>
            <div class="flex justify-between">
              <span>RFID Reader #3</span>
              <span class="badge badge-error">Offline</span>
            </div>
          </div>

          <div class="divider">System Performance</div>

          <div class="flex flex-col gap-2">
            <div>
              <div class="flex justify-between mb-1">
                <span>CPU Usage</span>
                <span>45%</span>
              </div>
              <progress
                class="progress progress-primary w-full"
                value="45"
                max="100"
              ></progress>
            </div>

            <div>
              <div class="flex justify-between mb-1">
                <span>Memory Usage</span>
                <span>62%</span>
              </div>
              <progress
                class="progress progress-secondary w-full"
                value="62"
                max="100"
              ></progress>
            </div>

            <div>
              <div class="flex justify-between mb-1">
                <span>Disk Usage</span>
                <span>28%</span>
              </div>
              <progress
                class="progress progress-success w-full"
                value="28"
                max="100"
              ></progress>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
      <div class="card bg-base-200 shadow-xl">
        <div class="card-body">
          <h2 class="card-title">Scan Distribution by Hour</h2>
          <div class="h-64 flex items-end justify-between gap-1 pt-4">
            <div v-for="hour in 24" :key="hour" class="relative group">
              <div
                class="bg-primary w-4 mx-auto"
                :style="`height: ${Math.floor(Math.random() * 100)}%`"
              ></div>
              <div
                class="opacity-0 group-hover:opacity-100 absolute bottom-full mb-2 left-1/2 transform -translate-x-1/2 bg-base-300 text-base-content px-2 py-1 rounded text-xs whitespace-nowrap"
              >
                {{ hour - 1 }}:00 - {{ hour }}:00:
                {{ Math.floor(Math.random() * 20) }} scans
              </div>
            </div>
          </div>
          <div class="text-center mt-2">Hour of Day</div>
        </div>
      </div>

      <div class="card bg-base-200 shadow-xl">
        <div class="card-body">
          <h2 class="card-title">Active API Keys</h2>
          <div class="overflow-x-auto">
            <table class="table table-zebra">
              <thead>
                <tr>
                  <th>Name</th>
                  <th>Device ID</th>
                  <th>Last Used</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td>ESP32 Device 1</td>
                  <td>ESP32-001</td>
                  <td>2025-09-04 08:15:22</td>
                </tr>
                <tr>
                  <td>ESP32 Device 2</td>
                  <td>ESP32-002</td>
                  <td>2025-09-04 08:10:15</td>
                </tr>
                <tr>
                  <td>Admin Panel</td>
                  <td>ADMIN-001</td>
                  <td>2025-09-03 16:45:10</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>
