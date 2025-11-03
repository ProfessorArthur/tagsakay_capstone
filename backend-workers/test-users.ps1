# Test User Endpoints
# Make sure server is running: npm run dev

$baseUrl = "http://127.0.0.1:8787"

Write-Host "🧪 Testing User Endpoints" -ForegroundColor Cyan
Write-Host "=" * 50

# Step 1: Login as admin
Write-Host "` Login as admin..." -ForegroundColor Yellow
$loginBody = @{
    email = "admin@tagsakay.com"
    password = "admin123"
} | ConvertTo-Json

try {
    $loginResponse = Invoke-RestMethod -Uri "$baseUrl/api/auth/login" -Method Post -Body $loginBody -ContentType "application/json"
    $token = $loginResponse.data.token
    Write-Host "✅ Login successful!" -ForegroundColor Green
    Write-Host "   User: $($loginResponse.data.user.name) ($($loginResponse.data.user.role))"
} catch {
    Write-Host "❌ Login failed: $_" -ForegroundColor Red
    exit
}

$headers = @{
    "Authorization" = "Bearer $token"
    "Content-Type" = "application/json"
}

# Step 2: Get all users
Write-Host "`n2️⃣ GET /api/users - Get all users..." -ForegroundColor Yellow
try {
    $users = Invoke-RestMethod -Uri "$baseUrl/api/users" -Method Get -Headers $headers
    Write-Host "✅ Retrieved $($users.count) users" -ForegroundColor Green
    $users.data | ForEach-Object {
        Write-Host "   - $($_.name) ($($_.email)) - Role: $($_.role)"
    }
} catch {
    Write-Host "❌ Failed: $_" -ForegroundColor Red
}

# Step 3: Get specific user
Write-Host "`n3️⃣ GET /api/users/1 - Get user by ID..." -ForegroundColor Yellow
try {
    $user = Invoke-RestMethod -Uri "$baseUrl/api/users/1" -Method Get -Headers $headers
    Write-Host "✅ Retrieved user: $($user.data.name)" -ForegroundColor Green
    Write-Host "   Email: $($user.data.email)"
    Write-Host "   Role: $($user.data.role)"
    Write-Host "   Active: $($user.data.isActive)"
    Write-Host "   RFID Tags: $($user.data.rfidTags.Count)"
} catch {
    Write-Host "❌ Failed: $_" -ForegroundColor Red
}

# Step 4: Create new user
Write-Host "`n4️⃣ POST /api/users - Create new user..." -ForegroundColor Yellow
$newUserBody = @{
    name = "Test User $(Get-Random -Minimum 100 -Maximum 999)"
    email = "testuser$(Get-Random -Minimum 100 -Maximum 999)@test.com"
    password = "test123"
    role = "driver"
    isActive = $true
} | ConvertTo-Json

try {
    $newUser = Invoke-RestMethod -Uri "$baseUrl/api/users" -Method Post -Headers $headers -Body $newUserBody
    Write-Host "✅ User created!" -ForegroundColor Green
    Write-Host "   ID: $($newUser.data.id)"
    Write-Host "   Name: $($newUser.data.name)"
    Write-Host "   Email: $($newUser.data.email)"
    $newUserId = $newUser.data.id
} catch {
    Write-Host "❌ Failed: $_" -ForegroundColor Red
    $newUserId = $null
}

# Step 5: Update user
if ($newUserId) {
    Write-Host "`n5️⃣ PUT /api/users/$newUserId - Update user..." -ForegroundColor Yellow
    $updateBody = @{
        name = "Updated Test User"
        isActive = $false
    } | ConvertTo-Json

    try {
        $updatedUser = Invoke-RestMethod -Uri "$baseUrl/api/users/$newUserId" -Method Put -Headers $headers -Body $updateBody
        Write-Host "✅ User updated!" -ForegroundColor Green
        Write-Host "   Name: $($updatedUser.data.name)"
        Write-Host "   Active: $($updatedUser.data.isActive)"
    } catch {
        Write-Host "❌ Failed: $_" -ForegroundColor Red
    }

    # Step 6: Delete user
    Write-Host "`n6️⃣ DELETE /api/users/$newUserId - Delete user..." -ForegroundColor Yellow
    try {
        $deleteResponse = Invoke-RestMethod -Uri "$baseUrl/api/users/$newUserId" -Method Delete -Headers $headers
        Write-Host "✅ $($deleteResponse.message)" -ForegroundColor Green
    } catch {
        Write-Host "❌ Failed: $_" -ForegroundColor Red
    }
}

# Step 7: Verify user was deleted
if ($newUserId) {
    Write-Host "`n7️⃣ Verify user was deleted..." -ForegroundColor Yellow
    try {
        $verifyUser = Invoke-RestMethod -Uri "$baseUrl/api/users/$newUserId" -Method Get -Headers $headers
        Write-Host "❌ User still exists (should be deleted)" -ForegroundColor Red
    } catch {
        Write-Host "✅ User successfully deleted (404 Not Found)" -ForegroundColor Green
    }
}

Write-Host "`n" + ("=" * 50)
Write-Host "🎉 User endpoint tests complete!" -ForegroundColor Cyan
