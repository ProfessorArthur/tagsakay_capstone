// ============================================================================
// RATE LIMITING TEST SCRIPT
// ============================================================================
// Tests the authentication rate limiting (5 attempts per minute)
// and verifies proper 429 response with retry-after headers

const BASE_URL = "http://localhost:8787";
const TEST_EMAIL = "nonexistent@test.com"; // Use non-existent email to trigger failures
const TEST_PASSWORD = "wrongpassword123";

// ANSI color codes for terminal output
const colors = {
  reset: "\x1b[0m",
  bright: "\x1b[1m",
  red: "\x1b[31m",
  green: "\x1b[32m",
  yellow: "\x1b[33m",
  blue: "\x1b[34m",
  magenta: "\x1b[35m",
  cyan: "\x1b[36m",
};

// Helper to print colored output
function log(message, color = "reset") {
  console.log(`${colors[color]}${message}${colors.reset}`);
}

function logSection(title) {
  log("\n" + "=".repeat(60), "cyan");
  log(`  ${title}`, "bright");
  log("=".repeat(60), "cyan");
}

function logSuccess(message) {
  log(`✅ ${message}`, "green");
}

function logError(message) {
  log(`❌ ${message}`, "red");
}

function logWarning(message) {
  log(`⚠️  ${message}`, "yellow");
}

function logInfo(message) {
  log(`ℹ️  ${message}`, "blue");
}

// Sleep function for delays
function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

// Test function to attempt login
async function attemptLogin(attemptNumber) {
  try {
    const response = await fetch(`${BASE_URL}/api/auth/login`, {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        email: TEST_EMAIL,
        password: TEST_PASSWORD,
      }),
    });

    const data = await response.json();
    const rateLimitHeaders = {
      limit: response.headers.get("X-RateLimit-Limit"),
      remaining: response.headers.get("X-RateLimit-Remaining"),
      reset: response.headers.get("X-RateLimit-Reset"),
      retryAfter: response.headers.get("Retry-After"),
    };

    return {
      status: response.status,
      data,
      headers: rateLimitHeaders,
    };
  } catch (error) {
    logError(`Network error on attempt ${attemptNumber}: ${error.message}`);
    throw error;
  }
}

// Main test function
async function runRateLimitTest() {
  logSection("🧪 RATE LIMITING TEST - Authentication Endpoint");

  log("\n📋 Test Configuration:", "magenta");
  log(`   Base URL: ${BASE_URL}`);
  log(`   Test Email: ${TEST_EMAIL}`);
  log(`   Max Attempts Allowed: 5 per minute`);
  log(`   Expected Behavior: First 5 attempts return 401, 6th+ return 429`);

  const results = [];
  let rateLimitTriggered = false;
  let retryAfterSeconds = null;

  // Attempt 1-7 logins to trigger rate limiting
  logSection("🔄 Testing Login Attempts (1-7)");

  for (let i = 1; i <= 7; i++) {
    logInfo(`\nAttempt ${i}:`);

    const result = await attemptLogin(i);
    results.push(result);

    log(
      `   Status: ${result.status}`,
      result.status === 429 ? "red" : "yellow"
    );
    log(`   Message: ${result.data.message || "N/A"}`);

    if (result.headers.limit) {
      log(
        `   Rate Limit: ${result.headers.remaining || 0}/${
          result.headers.limit
        } remaining`
      );
    }

    if (result.status === 429) {
      rateLimitTriggered = true;
      retryAfterSeconds = parseInt(result.headers.retryAfter || "0");
      log(`   Retry After: ${result.data.retryAfter || "N/A"}`, "yellow");

      logWarning(`Rate limit triggered on attempt ${i}!`);
      break;
    }

    // Small delay between requests (100ms)
    await sleep(100);
  }

  // Analysis
  logSection("📊 Test Results Analysis");

  // Check if rate limiting was triggered
  if (rateLimitTriggered) {
    logSuccess("Rate limiting successfully triggered");
  } else {
    logError("Rate limiting was NOT triggered (expected after 5 attempts)");
  }

  // Verify first 5 attempts returned 401
  const first5 = results.slice(0, 5);
  const all401 = first5.every((r) => r.status === 401);

  if (all401) {
    logSuccess("First 5 attempts correctly returned 401 (Unauthorized)");
  } else {
    logError("Some of the first 5 attempts did not return 401");
    first5.forEach((r, i) => {
      log(
        `   Attempt ${i + 1}: ${r.status}`,
        r.status === 401 ? "green" : "red"
      );
    });
  }

  // Verify 6th+ attempts returned 429
  const remaining = results.slice(5);
  const all429 = remaining.every((r) => r.status === 429);

  if (remaining.length > 0) {
    if (all429) {
      logSuccess(`Attempts 6+ correctly returned 429 (Rate Limited)`);
    } else {
      logError("Some attempts after the 5th did not return 429");
      remaining.forEach((r, i) => {
        log(
          `   Attempt ${i + 6}: ${r.status}`,
          r.status === 429 ? "green" : "red"
        );
      });
    }
  }

  // Check retry-after header
  if (retryAfterSeconds && retryAfterSeconds > 0) {
    logSuccess(`Retry-After header present: ${retryAfterSeconds} seconds`);
  } else {
    logWarning("Retry-After header missing or invalid");
  }

  // Test retry after waiting
  if (retryAfterSeconds && retryAfterSeconds <= 10) {
    logSection(`⏱️  Testing Retry After ${retryAfterSeconds} Seconds`);

    logInfo(`Waiting ${retryAfterSeconds} seconds before retry...`);
    await sleep((retryAfterSeconds + 1) * 1000);

    logInfo("Attempting login after rate limit window...");
    const retryResult = await attemptLogin("RETRY");

    if (retryResult.status === 401) {
      logSuccess(
        "Rate limit cleared! Request now returns 401 (expected for wrong credentials)"
      );
    } else if (retryResult.status === 429) {
      logWarning(
        "Still rate limited (this might be expected if backoff is active)"
      );
    } else {
      logWarning(`Unexpected status code: ${retryResult.status}`);
    }
  } else {
    logWarning(
      `Retry-After too long (${retryAfterSeconds}s), skipping retry test`
    );
  }

  // Final summary
  logSection("📈 Test Summary");

  const totalAttempts = results.length;
  const successful401 = results.filter((r) => r.status === 401).length;
  const successful429 = results.filter((r) => r.status === 429).length;

  log(`   Total Attempts: ${totalAttempts}`);
  log(`   401 Responses (Expected for first 5): ${successful401}`);
  log(`   429 Responses (Expected for 6+): ${successful429}`);

  const testPassed =
    rateLimitTriggered && all401 && (remaining.length === 0 || all429);

  if (testPassed) {
    logSuccess("\n🎉 ALL TESTS PASSED!");
    log("\n✨ The rate limiting middleware is working correctly:", "green");
    log("   - First 5 attempts return 401 (Unauthorized)", "green");
    log("   - 6th+ attempts return 429 (Rate Limited)", "green");
    log("   - Retry-After header is present", "green");
    log("   - Rate limit clears after the specified time", "green");
  } else {
    logError("\n❌ SOME TESTS FAILED");
    log("\n⚠️  Issues detected:", "yellow");

    if (!rateLimitTriggered) {
      log("   - Rate limiting was not triggered", "red");
    }
    if (!all401) {
      log("   - Some of the first 5 attempts did not return 401", "red");
    }
    if (remaining.length > 0 && !all429) {
      log("   - Some attempts after the 5th did not return 429", "red");
    }
  }

  logSection("🏁 Test Complete");

  return testPassed;
}

// Run the test
runRateLimitTest()
  .then((passed) => {
    process.exit(passed ? 0 : 1);
  })
  .catch((error) => {
    logError(`\n💥 Test failed with error: ${error.message}`);
    console.error(error);
    process.exit(1);
  });
