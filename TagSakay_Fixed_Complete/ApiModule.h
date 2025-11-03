#ifndef API_MODULE_H
#define API_MODULE_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "NetworkModule.h"  // Use ApiResponse from NetworkModule

// Request retry configuration
struct RetryConfig {
  int maxRetries;
  unsigned long retryDelay;
  bool exponentialBackoff;
};

class ApiModule {
private:
  HTTPClient http;
  String apiKey;
  String deviceId;
  String baseUrl;
  bool initialized;
  unsigned long lastRequestTime;
  int consecutiveFailures;
  RetryConfig retryConfig;
  
  // Statistics
  unsigned long totalRequests;
  unsigned long successfulRequests;
  unsigned long failedRequests;
  unsigned long totalResponseTime;
  
  String buildUrl(const String& endpoint);
  ApiResponse sendRequest(const String& method, const String& endpoint, 
                         const String& payload, bool useRetry = true);
  ApiResponse sendRequestWithRetry(const String& method, const String& endpoint, 
                                   const String& payload);
  bool validateResponse(const String& response);
  
public:
  ApiModule();
  
  // Initialization
  bool initialize(const String& url, const String& key, const String& devId);
  void setRetryConfig(int maxRetries, unsigned long retryDelay, bool exponentialBackoff = true);
  
  // Core endpoints
  ApiResponse sendScan(const String& tagId, const String& location = "");
  ApiResponse sendHeartbeat(bool includeStats = true);
  ApiResponse checkConnection();
  ApiResponse getRegistrationStatus();
  ApiResponse sendQueueOverride(int queueNumber, const String& reason);
  ApiResponse reportStatus(const String& status, const String& reason);
  
  // New endpoints
  ApiResponse registerDevice(const String& macAddress, const String& name, const String& location);
  ApiResponse updateDeviceConfig(const String& config);
  ApiResponse getDeviceConfig();
  ApiResponse reportError(const String& errorType, const String& errorMessage);
  ApiResponse syncTime();
  
  // Batch operations (for offline queue)
  ApiResponse sendBatchScans(const String scans[], int count);
  
  // State management
  bool isInitialized() const { return initialized; }
  int getConsecutiveFailures() const { return consecutiveFailures; }
  void resetFailureCount() { consecutiveFailures = 0; }
  unsigned long getLastRequestTime() const { return lastRequestTime; }
  
  // Statistics
  void getStatistics(unsigned long& total, unsigned long& success, 
                    unsigned long& failed, unsigned long& avgResponseTime);
  void resetStatistics();
  float getSuccessRate() const;
  
  // Diagnostics
  bool testEndpoint(const String& endpoint);
  String getLastError() const;
};

#endif // API_MODULE_H
