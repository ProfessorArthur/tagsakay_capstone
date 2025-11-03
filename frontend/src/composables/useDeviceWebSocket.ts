import { ref, reactive, onUnmounted } from "vue";
import { useWebSocket } from "./useWebSocket";

interface DeviceWebSocketConfig {
  deviceId: string;
  apiKey?: string;
}

interface DeviceState {
  deviceId: string;
  connected: boolean;
  lastHeartbeat: Date | null;
  scanCount: number;
  registrationMode: boolean;
  offlineScans: number;
}

export const useDeviceWebSocket = (config: DeviceWebSocketConfig) => {
  const deviceState = reactive<DeviceState>({
    deviceId: config.deviceId,
    connected: false,
    lastHeartbeat: null,
    scanCount: 0,
    registrationMode: false,
    offlineScans: 0,
  });

  const lastMessage = ref<any>(null);
  const error = ref<string | null>(null);

  // Get WebSocket URL with device ID
  const getWebSocketUrl = () => {
    const apiUrl = import.meta.env.VITE_API_URL || "http://localhost:8787";
    const wsUrl = apiUrl.replace(/^http/, "ws");
    return `${wsUrl}/ws/device?deviceId=${config.deviceId}`;
  };

  // Create WebSocket connection
  const {
    state,
    connect,
    disconnect,
    send,
    onMessage,
    onConnect,
    onDisconnect,
    onError,
  } = useWebSocket({
    url: getWebSocketUrl(),
    autoReconnect: true,
    reconnectDelay: 5000,
    maxReconnectAttempts: 10,
    heartbeatInterval: 30000, // 30 seconds
  });

  // Handle incoming messages from backend
  onMessage((data: any) => {
    lastMessage.value = data;
    console.log("Device WebSocket message:", data);

    // Handle welcome message
    if (data.success && data.message === "Connected to TagSakay API") {
      deviceState.connected = true;
      deviceState.lastHeartbeat = new Date();
    }

    // Handle offline scans notification
    if (data.offlineScans && data.offlineScans > 0) {
      deviceState.offlineScans = data.offlineScans;
    }

    // Handle heartbeat acknowledgment
    if (data.action === "heartbeat_ack") {
      deviceState.lastHeartbeat = new Date(data.timestamp);
      if (data.scanCount !== undefined) {
        deviceState.scanCount = data.scanCount;
      }
    }

    // Handle configuration confirmations
    if (data.config) {
      if (data.config.registrationMode !== undefined) {
        deviceState.registrationMode = data.config.registrationMode;
      }
    }

    // Handle scan responses
    if (data.scan) {
      deviceState.scanCount++;
      // Emit scan event for components to listen to
      window.dispatchEvent(
        new CustomEvent("device-scan-response", {
          detail: {
            deviceId: config.deviceId,
            scan: data.scan,
            user: data.user,
            device: data.device,
          },
        })
      );
    }

    // Handle errors
    if (!data.success && data.error) {
      error.value = data.error;
    }
  });

  // Handle connection events
  onConnect(() => {
    deviceState.connected = true;
    error.value = null;
    console.log(`Device ${config.deviceId} connected to WebSocket`);
  });

  onDisconnect(() => {
    deviceState.connected = false;
    console.log(`Device ${config.deviceId} disconnected from WebSocket`);
  });

  onError((errorMsg: string) => {
    error.value = errorMsg;
    deviceState.connected = false;
    console.error(`Device ${config.deviceId} WebSocket error:`, errorMsg);
  });

  // Device actions
  const scanRfid = (tagId: string, location?: string): boolean => {
    return send({
      action: "scan",
      tagId,
      location,
      timestamp: Date.now(),
    });
  };

  const sendHeartbeat = (): boolean => {
    return send({
      action: "heartbeat",
      timestamp: Date.now(),
    });
  };

  const setRegistrationMode = (enabled: boolean): boolean => {
    return send({
      action: "config",
      registrationMode: enabled,
    });
  };

  const setScanMode = (enabled: boolean): boolean => {
    return send({
      action: "config",
      scanMode: enabled,
    });
  };

  // Connection management
  const connectDevice = () => {
    connect();
  };

  const disconnectDevice = () => {
    disconnect();
  };

  // Cleanup on component unmount
  onUnmounted(() => {
    disconnect();
  });

  return {
    // State
    deviceState: deviceState,
    isConnected: state.connected,
    isConnecting: state.connecting,
    connectionError: error,
    lastMessage,

    // Connection management
    connectDevice,
    disconnectDevice,

    // Device actions
    scanRfid,
    sendHeartbeat,
    setRegistrationMode,
    setScanMode,
  };
};
