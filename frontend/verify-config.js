/**
 * Frontend configuration verification
 * Simple test to verify environment configuration
 */

import { CONFIG } from "./src/config/env";

console.log("🔍 Verifying frontend environment configuration...");

try {
  console.log(`✅ API Base URL: ${CONFIG.API.BASE_URL}`);
  console.log(`✅ API Timeout: ${CONFIG.API.TIMEOUT}ms`);
  console.log(`✅ App Environment: ${CONFIG.APP.NODE_ENV}`);
  console.log(`✅ Development Mode: ${CONFIG.APP.DEV}`);
  console.log(`✅ Production Mode: ${CONFIG.APP.PROD}`);

  console.log("🎉 Frontend configuration loaded successfully!");
} catch (error) {
  console.error(`❌ Configuration error: ${error.message}`);
}
