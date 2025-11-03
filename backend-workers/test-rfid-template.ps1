# Test Auth Endpoints
$baseUrl = "http://127.0.0.1:8787/api"
$testEmail = "newuser$(Get-Random -Minimum 100 -Maximum 999)@test.com"
$testPassword = "testpass123"

Write-Host "`n🧪 Testing Auth Endpoints" -ForegroundColor Cyan
Write-Host ("=" * 50)

# Test 1: Register new user
Write-Host " POST /api/auth/register - Register new user..." -ForegroundColor Yellow
try {
    $registerBody = @{
        name = "Test User"
        email = $testEmail
        password = $testPassword
        role = "driver"
    } | ConvertTo-Json

    $registerResponse = Invoke-RestMethod -Uri "$baseUrl/auth/register" `
        -Method POST `
        -ContentType "application/json" `
        -Body $registerBody

    if ($registerResponse.success) {
        Write-Host "✅ Registration successful!" -ForegroundColor Green
        Write-Host "   User: $($registerResponse.data.user.name)" -ForegroundColor Gray
        Write-Host "   Email: $($registerResponse.data.user.email)" -ForegroundColor Gray
        Write-Host "   Role: $($registerResponse.data.user.role)" -ForegroundColor Gray
        $token = $registerResponse.data.token
        $userId = $registerResponse.data.user.id
    }
} catch {
    Write-Host "❌ Registration failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Test 2: Login with existing user
Write-Host "`n2️⃣  POST /api/auth/login - Login with credentials..." -ForegroundColor Yellow
try {
    $loginBody = @{
        email = "admin@tagsakay.com"
        password = "admin123"
    } | ConvertTo-Json

    $loginResponse = Invoke-RestMethod -Uri "$baseUrl/auth/login" `
        -Method POST `
        -ContentType "application/json" `
        -Body $loginBody

    if ($loginResponse.success) {
        Write-Host "✅ Login successful!" -ForegroundColor Green
        Write-Host "   User: $($loginResponse.data.user.name)" -ForegroundColor Gray
        Write-Host "   Role: $($loginResponse.data.user.role)" -ForegroundColor Gray
        # Token retrieved but not used in this script
        $null = $loginResponse.data.token
    }
} catch {
    Write-Host "❌ Login failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 3: Refresh token
Write-Host "`n3️⃣  POST /api/auth/refresh - Refresh access token..." -ForegroundColor Yellow
try {
    $refreshResponse = Invoke-RestMethod -Uri "$baseUrl/auth/refresh" `
        -Method POST `
        -Headers @{ "Authorization" = "Bearer $token" }

    if ($refreshResponse.success) {
        Write-Host "✅ Token refreshed successfully!" -ForegroundColor Green
        Write-Host "   User: $($refreshResponse.data.user.name)" -ForegroundColor Gray
        $newToken = $refreshResponse.data.token
    }
} catch {
    Write-Host "❌ Token refresh failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 4: Verify refreshed token works
Write-Host "`n4️⃣  GET /api/users - Test refreshed token..." -ForegroundColor Yellow
try {
    # Try to access protected endpoint with new token
    $verifyResponse = Invoke-RestMethod -Uri "$baseUrl/users/$userId" `
        -Method GET `
        -Headers @{ "Authorization" = "Bearer $newToken" }

    if ($verifyResponse.success) {
        Write-Host "✅ Refreshed token is valid!" -ForegroundColor Green
        Write-Host "   Retrieved user: $($verifyResponse.data.user.name)" -ForegroundColor Gray
    }
} catch {
    Write-Host "❌ Token verification failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 5: Logout
Write-Host "`n5️⃣  POST /api/auth/logout - Logout user..." -ForegroundColor Yellow
try {
    $logoutResponse = Invoke-RestMethod -Uri "$baseUrl/auth/logout" `
        -Method POST `
        -Headers @{ "Authorization" = "Bearer $newToken" }

    if ($logoutResponse.success) {
        Write-Host "✅ Logout successful!" -ForegroundColor Green
    }
} catch {
    Write-Host "❌ Logout failed: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 6: Try to register with duplicate email
Write-Host "`n6️⃣  POST /api/auth/register - Test duplicate email..." -ForegroundColor Yellow
try {
    $duplicateBody = @{
        name = "Duplicate User"
        email = "admin@tagsakay.com"  # Existing email
        password = "password123"
    } | ConvertTo-Json

    Invoke-RestMethod -Uri "$baseUrl/auth/register" `
        -Method POST `
        -ContentType "application/json" `
        -Body $duplicateBody `
        -ErrorAction Stop

    Write-Host "❌ Should have failed with duplicate email" -ForegroundColor Red
} catch {
    if ($_.Exception.Response.StatusCode -eq 409) {
        Write-Host "✅ Duplicate email correctly rejected (409 Conflict)" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Unexpected error: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# Test 7: Login with invalid credentials
Write-Host "`n7️⃣  POST /api/auth/login - Test invalid credentials..." -ForegroundColor Yellow
try {
    $invalidBody = @{
        email = "admin@tagsakay.com"
        password = "wrongpassword"
    } | ConvertTo-Json

    Invoke-RestMethod -Uri "$baseUrl/auth/login" `
        -Method POST `
        -ContentType "application/json" `
        -Body $invalidBody `
        -ErrorAction Stop

    Write-Host "❌ Should have failed with invalid credentials" -ForegroundColor Red
} catch {
    if ($_.Exception.Response.StatusCode -eq 401) {
        Write-Host "✅ Invalid credentials correctly rejected (401 Unauthorized)" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Unexpected error: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# Test 8: Register with short password
Write-Host "`n8️⃣  POST /api/auth/register - Test password validation..." -ForegroundColor Yellow
try {
    $shortPassBody = @{
        name = "Test User"
        email = "shortpass@test.com"
        password = "123"  # Too short
    } | ConvertTo-Json

    Invoke-RestMethod -Uri "$baseUrl/auth/register" `
        -Method POST `
        -ContentType "application/json" `
        -Body $shortPassBody `
        -ErrorAction Stop

    Write-Host "❌ Should have failed with short password" -ForegroundColor Red
} catch {
    if ($_.Exception.Response.StatusCode -eq 400) {
        Write-Host "✅ Short password correctly rejected (400 Bad Request)" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Unexpected error: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

Write-Host "`n" + ("=" * 50)
Write-Host "🎉 Auth endpoint tests complete!" -ForegroundColor Cyan
Write-Host ""
