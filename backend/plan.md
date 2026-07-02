# Backend Implementation Plan — Firebase Cloud Infrastructure

## Overview

The backend is **entirely serverless**, running on **Firebase** services. No VPS, no Mosquitto, no custom server. Everything is managed by Google Cloud via Firebase.

---

## Firebase Services Used

| Service | Purpose |
|---|---|
| **Firebase Realtime DB** | Live station data, config, status (real-time sync) |
| **Cloud Firestore** | Historical readings, alerts, user profiles (queryable) |
| **Firebase Auth** | Google Sign-In (dashboard) + Custom Tokens (devices) |
| **Cloud Functions** | Alert triggers, data archival, token generation, data purge |
| **Firebase Hosting** | Serve the dashboard SPA |
| **Firebase Storage** | OTA firmware binaries |

---

## Firebase Project Setup

### Step 1: Create Firebase Project

```
1. Go to https://console.firebase.google.com
2. Create project: "pumping-station-iot"
3. Enable Google Analytics (optional, useful for dashboard usage tracking)
4. Region: europe-west1 (closest to Austria/customer)
```

### Step 2: Enable Services

```
1. Authentication → Sign-in method → Enable Google
2. Realtime Database → Create database → Region: europe-west1
3. Cloud Firestore → Create database → Region: europe-west1
4. Hosting → Set up hosting
5. Storage → Set up storage
6. Functions → Initialize (requires Blaze plan for outbound networking)
```

### Step 3: Upgrade to Blaze Plan
- Required for: Cloud Functions with external networking (sending emails)
- Set budget alert at $50/month
- Free tier still applies — only pay for overages

---

## Database Schema — Detailed

### Firebase Realtime DB

```
Root: /
├── stations/
│   └── {stationId}/                     ← e.g., "STATION_001"
│       ├── live/
│       │   ├── current: 12.4            ← float, Amps RMS
│       │   ├── alert: false             ← bool, any active alert
│       │   ├── alertType: null          ← string: "HIGH_CURRENT" | "LOW_CURRENT" | "NO_CURRENT" | null
│       │   ├── rssi: -67               ← int, signal strength dBm
│       │   ├── timestamp: 1719484800000 ← server timestamp
│       │   ├── firmwareVersion: "1.0.0" ← string
│       │   └── uptimeSeconds: 86400     ← int
│       │
│       ├── config/
│       │   ├── highThreshold: 18.0      ← float, Amps
│       │   ├── lowThreshold: 2.0        ← float, Amps
│       │   ├── reportIntervalSec: 30    ← int, seconds
│       │   ├── configPollIntervalSec: 300 ← int, seconds
│       │   ├── stationName: "Pumpstation Graz-Ost" ← string
│       │   ├── pumpPowerKW: 1.5         ← float
│       │   ├── lat: 47.0707             ← float (for map)
│       │   ├── lng: 15.4395             ← float (for map)
│       │   ├── calibration: 20.0        ← float, sensor calibration
│       │   └── latestFirmware/
│       │       ├── version: "1.0.0"
│       │       ├── url: "https://..."
│       │       └── checksum: "sha256..."
│       │
│       └── status/
│           ├── online: true             ← bool
│           ├── lastSeen: 1719484800000  ← server timestamp
│           └── provisionedAt: 1719398400000 ← timestamp
│
└── metadata/
    └── totalStations: 12                ← int, counter
```

### Cloud Firestore

#### Collection: `history/{stationId}/readings/{autoId}`
```json
{
  "current": 12.4,
  "alert": false,
  "alertType": null,
  "rssi": -67,
  "timestamp": "2026-06-27T10:00:00Z",  // Firestore Timestamp
  "stationId": "STATION_001"
}
```
**Index:** Composite index on `(stationId, timestamp)` for range queries.

**30-Day Retention:** Documents older than 30 days are deleted by scheduled Cloud Function.

#### Collection: `alerts/{autoId}`
```json
{
  "stationId": "STATION_001",
  "stationName": "Pumpstation Graz-Ost",
  "type": "HIGH_CURRENT",
  "currentValue": 22.1,
  "threshold": 18.0,
  "timestamp": "2026-06-27T10:00:00Z",
  "acknowledged": false,
  "acknowledgedBy": null,
  "acknowledgedAt": null,
  "notifiedVia": ["email"],
  "emailSentTo": ["christoph@example.com"]
}
```

#### Collection: `users/{uid}`
```json
{
  "email": "christoph@example.com",
  "displayName": "Christoph Barta",
  "role": "admin",
  "assignedStations": ["STATION_001", "STATION_002"],
  "alertEmail": "christoph@example.com",
  "createdAt": "2026-06-23T11:00:00Z",
  "lastLogin": "2026-06-27T10:00:00Z"
}
```

#### Collection: `devices/{stationId}`
```json
{
  "stationId": "STATION_001",
  "deviceSecret": "hashed_secret_here",
  "customToken": "encrypted_token_here",
  "provisionedAt": "2026-06-23T11:00:00Z",
  "provisionedBy": "admin_uid",
  "active": true
}
```

---

## Security Rules

### Realtime DB Rules (`database.rules.json`)

```json
{
  "rules": {
    "stations": {
      "$stationId": {
        "live": {
          ".read": "auth != null && (
            auth.uid === $stationId ||
            root.child('users/' + auth.uid).exists()
          )",
          ".write": "auth != null && auth.uid === $stationId"
        },
        "config": {
          ".read": "auth != null && (
            auth.uid === $stationId ||
            root.child('users/' + auth.uid).exists()
          )",
          ".write": "auth != null && (
            root.child('users/' + auth.uid + '/role').val() === 'admin'
          )"
        },
        "status": {
          ".read": "auth != null && root.child('users/' + auth.uid).exists()",
          ".write": "auth != null && auth.uid === $stationId"
        }
      }
    },
    "metadata": {
      ".read": "auth != null && root.child('users/' + auth.uid).exists()",
      ".write": false
    }
  }
}
```

### Firestore Rules (`firestore.rules`)

```javascript
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    
    // Helper functions
    function isAuthenticated() {
      return request.auth != null;
    }
    function isAdmin() {
      return isAuthenticated() && 
             get(/databases/$(database)/documents/users/$(request.auth.uid)).data.role == 'admin';
    }
    function isUser() {
      return isAuthenticated() && 
             exists(/databases/$(database)/documents/users/$(request.auth.uid));
    }
    
    // History — readable by any user, writable only by Cloud Functions (admin SDK)
    match /history/{stationId}/readings/{readingId} {
      allow read: if isUser();
      allow write: if false; // Only Cloud Functions write here
    }
    
    // Alerts — readable by any user, writable by admin (acknowledge)
    match /alerts/{alertId} {
      allow read: if isUser();
      allow update: if isAdmin() && 
                       request.resource.data.diff(resource.data).affectedKeys()
                       .hasOnly(['acknowledged', 'acknowledgedBy', 'acknowledgedAt']);
      allow create, delete: if false; // Only Cloud Functions
    }
    
    // Users — admins manage, users read own profile
    match /users/{userId} {
      allow read: if isAuthenticated() && 
                    (request.auth.uid == userId || isAdmin());
      allow create, update, delete: if isAdmin();
    }
    
    // Devices — admin only
    match /devices/{stationId} {
      allow read, write: if isAdmin();
    }
  }
}
```

---

## Cloud Functions — Detailed

### Function 1: `onLiveDataWrite` — Alert Trigger

**Trigger:** Realtime DB `onValueWritten` at `/stations/{stationId}/live`

**Logic:**
```javascript
// functions/src/alerts.js

const functions = require('firebase-functions/v2');
const admin = require('firebase-admin');
const { sendAlertEmail } = require('./email');

exports.onLiveDataWrite = functions.database
  .onValueWritten('/stations/{stationId}/live', async (event) => {
    const stationId = event.params.stationId;
    const data = event.data.after.val();
    
    if (!data || !data.alert) return; // No alert, skip
    
    // Get station config for context
    const configSnap = await admin.database()
      .ref(`stations/${stationId}/config`)
      .once('value');
    const config = configSnap.val();
    
    // Check for recent alert (cooldown: 5 min)
    const recentAlert = await admin.firestore()
      .collection('alerts')
      .where('stationId', '==', stationId)
      .where('timestamp', '>', new Date(Date.now() - 300000))
      .limit(1)
      .get();
    
    if (!recentAlert.empty) return; // Cooldown active
    
    // Create alert document
    const alert = {
      stationId,
      stationName: config.stationName || stationId,
      type: data.alertType,
      currentValue: data.current,
      threshold: data.alertType === 'HIGH_CURRENT' 
        ? config.highThreshold 
        : config.lowThreshold,
      timestamp: admin.firestore.FieldValue.serverTimestamp(),
      acknowledged: false,
      acknowledgedBy: null,
      acknowledgedAt: null,
      notifiedVia: ['email'],
      emailSentTo: []
    };
    
    await admin.firestore().collection('alerts').add(alert);
    
    // Get admin emails for notification
    const admins = await admin.firestore()
      .collection('users')
      .where('role', '==', 'admin')
      .get();
    
    const emails = admins.docs.map(doc => doc.data().alertEmail);
    
    // Send alert emails
    for (const email of emails) {
      await sendAlertEmail(email, alert);
    }
    
    // Update alert doc with sent emails
    // (update the doc we just created)
  });
```

---

### Function 2: `archiveReading` — Save to History

**Trigger:** Realtime DB `onValueWritten` at `/stations/{stationId}/live`

**Logic:**
```javascript
// functions/src/archive.js

exports.archiveReading = functions.database
  .onValueWritten('/stations/{stationId}/live', async (event) => {
    const stationId = event.params.stationId;
    const data = event.data.after.val();
    
    if (!data || !data.timestamp) return;
    
    // Write to Firestore history
    await admin.firestore()
      .collection('history')
      .doc(stationId)
      .collection('readings')
      .add({
        current: data.current,
        alert: data.alert,
        alertType: data.alertType || null,
        rssi: data.rssi,
        timestamp: admin.firestore.FieldValue.serverTimestamp(),
        stationId
      });
  });
```

---

### Function 3: `purgeOldData` — 30-Day Retention

**Trigger:** Cloud Scheduler — runs daily at 02:00 UTC

**Logic:**
```javascript
// functions/src/purge.js

exports.purgeOldData = functions.scheduler
  .onSchedule('0 2 * * *', async (context) => {
    const cutoff = new Date(Date.now() - 30 * 24 * 60 * 60 * 1000);
    
    // Get all station IDs
    const stationsSnap = await admin.database()
      .ref('stations')
      .once('value');
    const stationIds = Object.keys(stationsSnap.val() || {});
    
    for (const stationId of stationIds) {
      const oldReadings = await admin.firestore()
        .collection('history')
        .doc(stationId)
        .collection('readings')
        .where('timestamp', '<', cutoff)
        .limit(500) // Batch delete limit
        .get();
      
      if (oldReadings.empty) continue;
      
      const batch = admin.firestore().batch();
      oldReadings.docs.forEach(doc => batch.delete(doc.ref));
      await batch.commit();
      
      console.log(`Purged ${oldReadings.size} readings from ${stationId}`);
    }
    
    // Also purge old acknowledged alerts (>30 days)
    const oldAlerts = await admin.firestore()
      .collection('alerts')
      .where('acknowledged', '==', true)
      .where('timestamp', '<', cutoff)
      .limit(500)
      .get();
    
    if (!oldAlerts.empty) {
      const batch = admin.firestore().batch();
      oldAlerts.docs.forEach(doc => batch.delete(doc.ref));
      await batch.commit();
    }
  });
```

---

### Function 4: `provisionDevice` — Device Auth Token Generator

**Trigger:** HTTPS callable (called from dashboard admin UI)

**Logic:**
```javascript
// functions/src/provision.js

exports.provisionDevice = functions.https
  .onCall(async (data, context) => {
    // Verify caller is admin
    if (!context.auth) throw new functions.https.HttpsError('unauthenticated');
    
    const userDoc = await admin.firestore()
      .collection('users')
      .doc(context.auth.uid)
      .get();
    
    if (!userDoc.exists || userDoc.data().role !== 'admin') {
      throw new functions.https.HttpsError('permission-denied');
    }
    
    const { stationId, stationName, pumpPowerKW, lat, lng } = data;
    
    // Validate input
    if (!stationId || !stationName) {
      throw new functions.https.HttpsError('invalid-argument', 
        'stationId and stationName are required');
    }
    
    // Create custom token for the device
    // The UID of the device IS the stationId
    const customToken = await admin.auth().createCustomToken(stationId, {
      deviceType: 'esp32',
      stationId
    });
    
    // Initialize station in RTDB
    await admin.database().ref(`stations/${stationId}`).set({
      live: {
        current: 0,
        alert: false,
        alertType: null,
        rssi: 0,
        timestamp: admin.database.ServerValue.TIMESTAMP,
        firmwareVersion: "0.0.0",
        uptimeSeconds: 0
      },
      config: {
        highThreshold: 18.0,
        lowThreshold: 2.0,
        reportIntervalSec: 30,
        configPollIntervalSec: 300,
        stationName,
        pumpPowerKW: pumpPowerKW || 1.5,
        lat: lat || 0,
        lng: lng || 0,
        calibration: 20.0,
        latestFirmware: {
          version: "1.0.0",
          url: "",
          checksum: ""
        }
      },
      status: {
        online: false,
        lastSeen: admin.database.ServerValue.TIMESTAMP,
        provisionedAt: admin.database.ServerValue.TIMESTAMP
      }
    });
    
    // Store device record in Firestore
    await admin.firestore().collection('devices').doc(stationId).set({
      stationId,
      provisionedAt: admin.firestore.FieldValue.serverTimestamp(),
      provisionedBy: context.auth.uid,
      active: true
    });
    
    // Update metadata counter
    await admin.database().ref('metadata/totalStations')
      .set(admin.database.ServerValue.increment(1));
    
    return {
      success: true,
      stationId,
      customToken,
      message: `Station ${stationId} provisioned. Flash this token to the device.`
    };
  });
```

> [!IMPORTANT]
> The `customToken` returned by `provisionDevice` has a 1-hour expiry. The device must exchange it for an ID token + refresh token within that hour. After that, it uses the refresh token indefinitely. During provisioning, flash the token and boot the device immediately.

---

### Function 5: `checkOfflineStations` — Offline Detection

**Trigger:** Cloud Scheduler — runs every 5 minutes

**Logic:**
```javascript
// functions/src/offline.js

exports.checkOfflineStations = functions.scheduler
  .onSchedule('*/5 * * * *', async (context) => {
    const offlineThreshold = Date.now() - 5 * 60 * 1000; // 5 min
    
    const stationsSnap = await admin.database()
      .ref('stations')
      .once('value');
    
    const stations = stationsSnap.val() || {};
    
    for (const [stationId, station] of Object.entries(stations)) {
      const lastSeen = station.status?.lastSeen || 0;
      const wasOnline = station.status?.online || false;
      
      if (wasOnline && lastSeen < offlineThreshold) {
        // Mark as offline
        await admin.database()
          .ref(`stations/${stationId}/status/online`)
          .set(false);
        
        // Create offline alert
        await admin.firestore().collection('alerts').add({
          stationId,
          stationName: station.config?.stationName || stationId,
          type: 'DEVICE_OFFLINE',
          currentValue: null,
          threshold: null,
          timestamp: admin.firestore.FieldValue.serverTimestamp(),
          acknowledged: false,
          notifiedVia: ['email'],
          emailSentTo: []
        });
        
        // Send email notification
        // (same pattern as alert emails)
      }
    }
  });
```

---

### Function 6: `sendAlertEmail` — Email Service

**Implementation:** Use **SendGrid** (free tier: 100 emails/day) or **Nodemailer** with a Gmail service account.

```javascript
// functions/src/email.js

const sgMail = require('@sendgrid/mail');
sgMail.setApiKey(process.env.SENDGRID_API_KEY);

exports.sendAlertEmail = async (to, alert) => {
    const alertTypeLabels = {
        'HIGH_CURRENT': '⚠️ High Current Alert',
        'LOW_CURRENT': '⚠️ Low Current Alert',
        'NO_CURRENT': '🔴 No Current Detected',
        'DEVICE_OFFLINE': '🔴 Device Offline'
    };
    
    const subject = `${alertTypeLabels[alert.type]} — ${alert.stationName}`;
    
    const html = `
        <div style="font-family: Arial, sans-serif; max-width: 600px;">
            <h2 style="color: #ef4444;">${alertTypeLabels[alert.type]}</h2>
            <table style="width: 100%; border-collapse: collapse;">
                <tr>
                    <td style="padding: 8px; border-bottom: 1px solid #eee;"><strong>Station</strong></td>
                    <td style="padding: 8px; border-bottom: 1px solid #eee;">${alert.stationName} (${alert.stationId})</td>
                </tr>
                <tr>
                    <td style="padding: 8px; border-bottom: 1px solid #eee;"><strong>Current Reading</strong></td>
                    <td style="padding: 8px; border-bottom: 1px solid #eee;">${alert.currentValue !== null ? alert.currentValue + ' A' : 'N/A'}</td>
                </tr>
                <tr>
                    <td style="padding: 8px; border-bottom: 1px solid #eee;"><strong>Threshold</strong></td>
                    <td style="padding: 8px; border-bottom: 1px solid #eee;">${alert.threshold !== null ? alert.threshold + ' A' : 'N/A'}</td>
                </tr>
                <tr>
                    <td style="padding: 8px;"><strong>Time</strong></td>
                    <td style="padding: 8px;">${new Date().toLocaleString('de-AT')}</td>
                </tr>
            </table>
            <p style="margin-top: 20px;">
                <a href="https://pumping-station-iot.web.app/#/station/${alert.stationId}" 
                   style="background: #06b6d4; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px;">
                    View Station Dashboard →
                </a>
            </p>
        </div>
    `;
    
    await sgMail.send({
        to,
        from: 'alerts@pumping-station-iot.com', // Verified sender
        subject,
        html
    });
};
```

---

## Firebase Indexes

### Firestore Composite Indexes

| Collection | Fields | Order |
|---|---|---|
| `history/{stationId}/readings` | `timestamp` | ASC |
| `alerts` | `stationId`, `timestamp` | ASC, DESC |
| `alerts` | `acknowledged`, `timestamp` | ASC, DESC |
| `users` | `role` | ASC |

Create via `firestore.indexes.json`:
```json
{
  "indexes": [
    {
      "collectionGroup": "readings",
      "queryScope": "COLLECTION",
      "fields": [
        { "fieldPath": "timestamp", "order": "ASCENDING" }
      ]
    },
    {
      "collectionGroup": "alerts",
      "queryScope": "COLLECTION",
      "fields": [
        { "fieldPath": "stationId", "order": "ASCENDING" },
        { "fieldPath": "timestamp", "order": "DESCENDING" }
      ]
    },
    {
      "collectionGroup": "alerts",
      "queryScope": "COLLECTION",
      "fields": [
        { "fieldPath": "acknowledged", "order": "ASCENDING" },
        { "fieldPath": "timestamp", "order": "DESCENDING" }
      ]
    }
  ]
}
```

---

## Deployment

### Firebase CLI Commands

```bash
# Install Firebase CLI
npm install -g firebase-tools

# Login
firebase login

# Initialize project
firebase init
# Select: Functions, Hosting, Firestore, Database, Storage

# Deploy everything
firebase deploy

# Deploy only functions
firebase deploy --only functions

# Deploy only hosting (frontend)
firebase deploy --only hosting

# Deploy only rules
firebase deploy --only database
firebase deploy --only firestore:rules
firebase deploy --only firestore:indexes
```

### Environment Variables (Functions)

```bash
# Set SendGrid API key
firebase functions:secrets:set SENDGRID_API_KEY

# Set any other secrets
firebase functions:secrets:set CUSTOM_SECRET
```

---

## File Structure

```
backend/
├── plan.md                      ← This file
├── firebase.json                ← Firebase project config
├── .firebaserc                  ← Project alias
├── database.rules.json          ← Realtime DB security rules
├── firestore.rules              ← Firestore security rules
├── firestore.indexes.json       ← Firestore indexes
├── storage.rules                ← Storage security rules
└── functions/
    ├── package.json             ← Node.js dependencies
    ├── .env                     ← Environment variables (local)
    └── src/
        ├── index.js             ← Export all Cloud Functions
        ├── alerts.js            ← onLiveDataWrite — alert trigger
        ├── archive.js           ← archiveReading — save to Firestore
        ├── purge.js             ← purgeOldData — 30-day cleanup
        ├── provision.js         ← provisionDevice — device auth
        ├── offline.js           ← checkOfflineStations — heartbeat monitor
        └── email.js             ← sendAlertEmail — email service
```

---

## Testing Plan

| Test | Method |
|---|---|
| Security rules reject unauthorized access | Firebase emulator + rules test suite |
| Alert triggers on high current write | Write test data to RTDB, verify Firestore alert created |
| Alert email sent | Trigger alert, check SendGrid delivery logs |
| Alert cooldown works (5 min) | Trigger 2 alerts within 5 min, verify only 1 email |
| History archived correctly | Write to RTDB live, verify Firestore reading created |
| 30-day purge deletes old data | Insert data with old timestamps, run purge, verify deleted |
| Device provisioning creates station | Call provisionDevice, verify RTDB + Firestore entries |
| Custom token authentication works | Use generated token in REST API call, verify 200 response |
| Offline detection fires | Stop writing to station, wait 5 min, verify offline alert |
| Firestore indexes work | Run history query with date range, verify no index error |

### Firebase Emulator Suite

```bash
# Start local emulators for testing
firebase emulators:start

# Emulators available:
# - Auth: localhost:9099
# - RTDB: localhost:9000
# - Firestore: localhost:8080
# - Functions: localhost:5001
# - Hosting: localhost:5000

# Run function tests
cd functions && npm test
```
