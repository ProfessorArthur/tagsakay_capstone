# 🧪 Manual UI Testing Guide - TagSakay Frontend

## Prerequisites

1. **Backend Server Running:**

   ```powershell
   cd backend-workers
   npm run dev
   # Should be running on http://localhost:8787
   ```

2. **Frontend Server Running:**
   ```powershell
   cd frontend
   npm run dev
   # Should be running on http://localhost:5173
   ```

---

## Test 1: Rate Limiting UI (Login.vue)

### Objective

Verify that the login page displays rate limiting feedback after 5 failed attempts.

### Steps

1. **Navigate to Login Page**

   - Open browser: `http://localhost:5173/login`
   - You should see the login form

2. **Trigger Rate Limit (5 Failed Attempts)**

   - Enter any email (e.g., `test@example.com`)
   - Enter any wrong password (e.g., `wrongpassword`)
   - Click "Login" button **5 times** in rapid succession
   - Wait for each attempt to return an error before clicking again

3. **Attempt 6th Login**

   - Click "Login" button again (6th time)

4. **Verify Rate Limiting UI**

   - ✅ Yellow/warning alert should appear (not red)
   - ✅ Warning triangle icon (not X icon)
   - ✅ Message: "Too many login attempts. Please try again in X seconds"
   - ✅ Additional text: "⏱️ Rate limit will reset automatically. Please wait before trying again."
   - ✅ Submit button should be **disabled**
   - ✅ Button text should show: "⏱️ Please Wait"

5. **Wait for Rate Limit to Clear**

   - Wait approximately 60 seconds
   - ✅ Warning message should automatically disappear
   - ✅ Submit button should become enabled again
   - ✅ Button text should return to "Login"

6. **Try Login Again**
   - Enter credentials and click "Login"
   - ✅ Should work normally (show 401 error if credentials wrong)

### Expected Behavior

- Rate limit warning displays after 5 failed attempts
- UI clearly communicates wait time
- Button is disabled during rate limit
- Message auto-clears after timeout
- Normal operation resumes after timeout

### Screenshots to Capture

- [ ] Login form (normal state)
- [ ] After 5th attempt (still red error)
- [ ] After 6th attempt (yellow warning with timer)
- [ ] Disabled button with "⏱️ Please Wait"
- [ ] After timeout (normal state restored)

---

## Test 2: Account Lockout UI (Login.vue)

### Objective

Verify that the login page displays account lockout warnings.

### Steps

1. **Trigger Account Lockout**

   - Use the same email for all attempts (e.g., `locked@example.com`)
   - Enter wrong password
   - Click "Login" **5 times** rapidly
   - On the 5th attempt with wrong password, account should be locked

2. **Verify Account Lockout UI**

   - ✅ Red/error alert should appear
   - ✅ X icon (not warning triangle)
   - ✅ Message: "Account temporarily locked..."
   - ✅ Additional text: "🔒 Your account has been temporarily locked for security. Please wait 15 minutes."
   - ✅ Submit button may be disabled or show error state

3. **Try to Login Again**
   - Even with correct password, should show lockout message
   - Should return 403 (Forbidden) status

### Expected Behavior

- Account lockout triggers after 5 failed password attempts
- Clear message explaining the situation
- Mentions 15-minute wait time
- Prevents further login attempts

---

## Test 3: Password Strength Indicator (Register.vue)

### Objective

Verify real-time password strength validation with color-coded feedback.

### Steps

1. **Navigate to Register Page**

   - Open browser: `http://localhost:5173/register`
   - You should see the registration form

2. **Test Very Weak Passwords (Red Bar)**

   - Type: `pass` (too short)

     - ✅ Progress bar: **Red** background
     - ✅ Text: "Very Weak"
     - ✅ Width: ~10-20%
     - ✅ Feedback: "At least 8 characters", "Contains uppercase letter", etc.

   - Type: `password` (only lowercase)
     - ✅ Progress bar: **Red** background
     - ✅ Text: "Very Weak"
     - ✅ Feedback shows missing requirements

3. **Test Weak Passwords (Yellow Bar)**

   - Type: `Password1` (missing special chars)
     - ✅ Progress bar: **Yellow** background
     - ✅ Text: "Weak"
     - ✅ Width: ~30-40%
     - ✅ Feedback: "Contains special character (!@#$%^&\*)"

4. **Test Fair Passwords (Blue Bar)**

   - Type: `Password1!` (basic complexity)
     - ✅ Progress bar: **Blue** background
     - ✅ Text: "Fair"
     - ✅ Width: ~50-60%
     - ✅ Some requirements still showing

5. **Test Good Passwords (Light Green Bar)**

   - Type: `MyP@ssw0rd123` (good mix)
     - ✅ Progress bar: **Light Green** background
     - ✅ Text: "Good"
     - ✅ Width: ~70-80%
     - ✅ Most requirements met

6. **Test Strong Passwords (Dark Green Bar)**
   - Type: `Tr1cYcl3!Qu3u3$yst3m#2024` (excellent)
     - ✅ Progress bar: **Dark Green** background
     - ✅ Text: "Strong"
     - ✅ Width: 100%
     - ✅ All requirements met

### Expected Behavior

- Password strength updates **in real-time** as you type
- Progress bar color changes: Red → Yellow → Blue → Light Green → Dark Green
- Feedback list shows which requirements are missing
- Submit button disabled until password is strong enough

### Screenshots to Capture

- [ ] Very Weak password (red bar)
- [ ] Weak password (yellow bar)
- [ ] Fair password (blue bar)
- [ ] Good password (light green bar)
- [ ] Strong password (dark green bar)
- [ ] Feedback list showing validation errors

---

## Test 4: Password Match Validation (Register.vue)

### Objective

Verify that confirm password field validates matching passwords.

### Steps

1. **Enter Strong Password**

   - In "Password" field, type: `MyP@ssw0rd123!`
   - ✅ Should show green "Strong" indicator

2. **Test Matching Passwords**

   - In "Confirm Password" field, type: `MyP@ssw0rd123!` (same)
   - ✅ Input border should turn **green/primary color**
   - ✅ Below field: "✅ Passwords match" (green text)

3. **Test Non-Matching Passwords**

   - In "Confirm Password" field, type: `MyP@ssw0rd456!` (different)
   - ✅ Input border should turn **red/error color**
   - ✅ Below field: "❌ Passwords do not match" (red text)

4. **Fix the Mismatch**
   - Correct the confirm password to match
   - ✅ Should immediately show ✅ "Passwords match"
   - ✅ Border should turn green
   - ✅ Submit button should become enabled

### Expected Behavior

- Real-time validation as you type
- Visual feedback (border color changes)
- Clear checkmark (✅) or X (❌) icons
- Submit button disabled until passwords match

---

## Test 5: Password Visibility Toggle (Both Forms)

### Objective

Verify that password visibility toggles work correctly.

### Steps (Login.vue)

1. **Navigate to Login Page**

   - Open: `http://localhost:5173/login`

2. **Test Password Visibility**
   - Type a password in the "Password" field
   - ✅ By default, password should be masked: `••••••••`
   - Click the **eye icon** (👁️) on the right side
   - ✅ Password should become visible: `MyPassword123`
   - ✅ Icon should change to **eye-slash** (👁️‍🗨️)
   - Click the eye-slash icon again
   - ✅ Password should be masked again

### Steps (Register.vue)

1. **Navigate to Register Page**

   - Open: `http://localhost:5173/register`

2. **Test Password Visibility (Password Field)**

   - Type in "Password" field
   - Click eye icon
   - ✅ Should toggle visibility (same as login)

3. **Test Password Visibility (Confirm Password Field)**
   - Type in "Confirm Password" field
   - Click eye icon
   - ✅ Should toggle visibility independently
   - ✅ Both fields can have different visibility states

### Expected Behavior

- Eye icon toggles between eye (show) and eye-slash (hide)
- Password text visibility toggles correctly
- Each field has independent toggle (Register page)
- Smooth transition between states

---

## Test 6: Submit Button States (Register.vue)

### Objective

Verify that the submit button is disabled until all validation passes.

### Steps

1. **Test with Empty Form**

   - Navigate to register page with no input
   - ✅ Submit button should be **enabled** (will validate on submit)

2. **Test with Weak Password**

   - Enter weak password: `password123`
   - ✅ Button text should show: "⚠️ Weak Password"
   - ✅ Button should be **disabled** (grayed out)
   - ✅ Clicking should do nothing

3. **Test with Password Mismatch**

   - Enter strong password: `MyP@ssw0rd123!`
   - Enter different confirm: `MyP@ssw0rd456!`
   - ✅ Button text should show: "❌ Passwords Don't Match"
   - ✅ Button should be **disabled**

4. **Test with Valid Data**

   - Enter strong password: `MyP@ssw0rd123!`
   - Enter matching confirm: `MyP@ssw0rd123!`
   - Enter name: `Test User`
   - Enter email: `test@example.com`
   - ✅ Button should be **enabled** (blue/primary color)
   - ✅ Button text should show: "Register"
   - ✅ Clicking should submit the form

5. **Test Loading State**
   - Click "Register" button
   - ✅ Button should show: "Registering..."
   - ✅ Loading spinner should appear
   - ✅ Button should be disabled during loading

### Expected Behavior

- Button dynamically changes text based on validation state
- Button disabled when validation fails
- Button enabled only when all requirements met
- Loading state prevents double-submission

---

## Test 7: Error Messages (Both Forms)

### Objective

Verify that error messages are displayed correctly.

### Steps (Login.vue)

1. **Test Invalid Email**

   - Enter: `notanemail`
   - Enter any password
   - Click "Login"
   - ✅ Should show error: "Invalid email or password"

2. **Test Wrong Credentials**
   - Enter: `wrong@example.com`
   - Enter: `wrongpassword`
   - Click "Login"
   - ✅ Should show red error alert with X icon
   - ✅ Message: "Invalid email or password"

### Steps (Register.vue)

1. **Test Invalid Email Format**

   - Enter invalid email: `notanemail`
   - Try to submit
   - ✅ Should show validation error

2. **Test Duplicate Email**

   - Enter email that already exists in database
   - Try to register
   - ✅ Should show error message

3. **Test Weak Password**
   - Enter weak password: `pass`
   - Try to submit
   - ✅ Should show error: "Password does not meet security requirements"
   - ✅ Should list specific requirements

### Expected Behavior

- Errors display in red alert boxes
- Clear, user-friendly error messages
- Specific guidance on how to fix the error
- Errors clear when user fixes the issue

---

## 🎯 Test Checklist

### Login Page (Login.vue)

- [ ] Rate limiting UI displays after 5 attempts
- [ ] Warning message shows retry time (60 seconds)
- [ ] Submit button disabled during rate limit
- [ ] Rate limit auto-clears after timeout
- [ ] Account lockout warning displays (15 minutes)
- [ ] Password visibility toggle works
- [ ] Error messages display correctly

### Register Page (Register.vue)

- [ ] Password strength indicator shows all 5 levels (0-4)
- [ ] Progress bar color changes correctly (red→yellow→blue→green)
- [ ] Strength text updates in real-time
- [ ] Feedback list shows validation requirements
- [ ] Password match validation works (✅/❌)
- [ ] Both password visibility toggles work independently
- [ ] Submit button disabled for weak passwords
- [ ] Submit button disabled for password mismatch
- [ ] Submit button enabled when all valid
- [ ] Button text changes based on validation state
- [ ] Loading state displays during submission
- [ ] Error messages display correctly

---

## 📸 Evidence Collection

### Required Screenshots/Videos

1. **Rate Limiting Demo** (Video recommended)

   - Show 5 failed login attempts
   - Show rate limiting warning on 6th attempt
   - Show auto-clearing after 60 seconds

2. **Password Strength Indicator** (Screenshots)

   - One screenshot per strength level (5 total)
   - Show color progression and feedback

3. **Password Match Validation** (Screenshots)

   - Matching passwords (green, ✅)
   - Non-matching passwords (red, ❌)

4. **Button States** (Screenshots)
   - Disabled with "⚠️ Weak Password"
   - Disabled with "❌ Passwords Don't Match"
   - Enabled with "Register"
   - Loading with "Registering..."

---

## 🐛 Known Issues to Watch For

1. **Rate Limit Persistence**

   - Rate limit is IP-based (in-memory)
   - Restarting backend server clears rate limit
   - Browser incognito mode won't help (same IP)

2. **Auto-Clear Timer**

   - Should clear after 60 seconds
   - If not working, check browser console for errors

3. **Password Strength Calculation**

   - Updates in real-time as you type
   - Should use `authService.validatePasswordStrength()`
   - If not updating, check Vue reactivity

4. **Rate Limit During Registration**
   - If you tested login first, might hit rate limit
   - Wait 60+ seconds before testing registration
   - Or restart backend server

---

## 💡 Tips for Testing

1. **Use Browser DevTools**

   - Open Network tab to see API responses
   - Check status codes (401, 403, 429)
   - Verify headers (X-RateLimit-\*, Retry-After)

2. **Test in Multiple Browsers**

   - Chrome, Firefox, Edge
   - Verify consistency across browsers

3. **Test Responsive Design**

   - Desktop view
   - Tablet view (768px)
   - Mobile view (375px)

4. **Test Keyboard Navigation**

   - Tab through form fields
   - Submit with Enter key
   - Verify accessibility

5. **Check Console for Errors**
   - Open browser console (F12)
   - Should see no JavaScript errors
   - Check for warning messages

---

## ✅ Success Criteria

All tests pass when:

- ✅ Rate limiting UI displays correctly with countdown timer
- ✅ Account lockout warning shows clear 15-minute message
- ✅ Password strength indicator displays all 5 levels with correct colors
- ✅ Password match validation works in real-time
- ✅ Password visibility toggles work for all fields
- ✅ Submit buttons disabled/enabled based on validation
- ✅ Error messages are clear and actionable
- ✅ No JavaScript errors in console
- ✅ All UI animations smooth and responsive
- ✅ Works across different browsers and screen sizes

---

## 🚀 After Testing

1. **Document Results**

   - Take screenshots of each test
   - Note any issues or unexpected behavior
   - Record browser/OS versions tested

2. **Report Issues**

   - Create GitHub issues for any bugs found
   - Include steps to reproduce
   - Attach screenshots/videos

3. **Update Test Results**

   - Mark tests as passed/failed in TEST_RESULTS.md
   - Add any new findings or observations

4. **Plan Fixes**
   - Prioritize critical issues
   - Schedule fixes before production deployment

---

## 📞 Need Help?

If you encounter any issues:

1. Check browser console for errors
2. Verify backend server is running (http://localhost:8787)
3. Check Network tab for API responses
4. Review TEST_RESULTS.md for known issues
5. Restart both frontend and backend servers

Good luck with testing! 🧪✨
