# RFID Card Registration Enhancement

## Overview

This update adds a more intuitive RFID card registration process to the TagSakay system. The new implementation allows administrators to easily register new RFID cards by automatically detecting unregistered cards that are scanned by any connected ESP32 device.

## Features

1. **Live Unregistered Card Detection**:

   - New `RfidUnregisteredScans` component that displays recently scanned cards that are not yet registered
   - Automatic polling to detect new card scans in real-time
   - "Select" button that automatically fills the registration form with the scanned card ID

2. **Enhanced Registration Modal**:

   - Split-view layout with registration form on the left and unregistered scans on the right
   - Improved visual feedback and user experience
   - Error handling for API failures

3. **Backend Integration**:
   - Uses the existing `/rfid/recent-unregistered-scans` endpoint to fetch unregistered scans
   - Improved error handling to prevent UI crashes

## How It Works

1. When a user opens the registration modal, the system begins polling the backend for any unregistered card scans
2. As unregistered cards are scanned by any RFID reader in the system, they appear in the right panel
3. The user can click "Select" on any detected card to automatically fill the Tag ID field in the registration form
4. The user completes the form with user ID and any metadata, then clicks Register
5. Upon successful registration, the modal shows a success message and closes after a short delay

## Technical Implementation

- The `RfidUnregisteredScans` component is based on the existing `RfidLiveScans` component but specifically filters for unregistered scans
- Polling interval is faster (2 seconds instead of 5) for better responsiveness during registration
- Improved error handling with graceful fallbacks to prevent UI crashes if the backend API fails

## Future Improvements

- User selection dropdown instead of manual user ID entry
- Form validation for the Tag ID field
- Visual confirmation when a tag is selected from the unregistered scans list
- Animation to show when new unregistered tags are detected
