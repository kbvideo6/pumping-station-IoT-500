const functions = require('firebase-functions');
const admin = require('firebase-admin');

exports.provisionDevice = functions.https.onCall(async (data, context) => {
  // 1. Verify Authentication
  if (!context.auth) {
    throw new functions.https.HttpsError(
      'unauthenticated',
      'The function must be called while authenticated.'
    );
  }

  const firestore = admin.firestore();
  const db = admin.database();
  const callerUid = context.auth.uid;

  try {
    // 2. Verify admin permissions
    const userDoc = await firestore.collection('users').doc(callerUid).get();
    if (!userDoc.exists || userDoc.data().role !== 'admin') {
      throw new functions.https.HttpsError(
        'permission-denied',
        'Only administrators can provision new devices.'
      );
    }

    const { stationId, stationName, pumpPowerKW, lat, lng } = data;

    // 3. Validation
    if (!stationId || typeof stationId !== 'string') {
      throw new functions.https.HttpsError(
        'invalid-argument',
        'stationId must be a valid non-empty string.'
      );
    }

    if (!stationName || typeof stationName !== 'string') {
      throw new functions.https.HttpsError(
        'invalid-argument',
        'stationName must be a valid non-empty string.'
      );
    }

    const parsedStationId = stationId.trim().toUpperCase();

    // 4. Check if station already exists
    const stationExistSnap = await db.ref(`/stations/${parsedStationId}`).once('value');
    if (stationExistSnap.exists()) {
      throw new functions.https.HttpsError(
        'already-exists',
        `A station with ID ${parsedStationId} already exists.`
      );
    }

    // 5. Generate Custom Token
    // We set the UID of the Custom Token to parsedStationId, so when the ESP32 authenticates,
    // auth.uid will equal parsedStationId, matching our RTDB & Firestore security rules.
    const customToken = await admin.auth().createCustomToken(parsedStationId, {
      deviceType: 'esp32',
      stationId: parsedStationId
    });

    // 6. Create station node in Realtime DB
    await db.ref(`/stations/${parsedStationId}`).set({
      live: {
        current: 0.0,
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
        stationName: stationName.trim(),
        pumpPowerKW: pumpPowerKW !== undefined ? parseFloat(pumpPowerKW) : 1.5,
        lat: lat !== undefined ? parseFloat(lat) : 47.0707, // Default coordinates in Austria
        lng: lng !== undefined ? parseFloat(lng) : 15.4395,
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

    // 7. Store provisioning log in Firestore
    await firestore.collection('devices').doc(parsedStationId).set({
      stationId: parsedStationId,
      stationName: stationName.trim(),
      provisionedAt: admin.firestore.FieldValue.serverTimestamp(),
      provisionedBy: callerUid,
      active: true
    });

    // 8. Increment station counter
    await db.ref('metadata/totalStations').set(admin.database.ServerValue.increment(1));

    console.log(`Successfully provisioned station ${parsedStationId} by admin ${callerUid}`);

    return {
      success: true,
      stationId: parsedStationId,
      customToken,
      message: `Station ${parsedStationId} provisioned. Configure the firmware with the returned token.`
    };
  } catch (error) {
    console.error('Error during device provisioning:', error);
    if (error instanceof functions.https.HttpsError) {
      throw error;
    }
    throw new functions.https.HttpsError(
      'internal',
      error.message || 'An unexpected error occurred during provisioning.'
    );
  }
});
