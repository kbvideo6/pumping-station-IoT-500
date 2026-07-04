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
    const resolvedName = (stationName || data.name || stationId || '').trim();

    // 3. Validation
    if (!stationId || typeof stationId !== 'string') {
      throw new functions.https.HttpsError(
        'invalid-argument',
        'stationId must be a valid non-empty string.'
      );
    }

    if (!resolvedName) {
      throw new functions.https.HttpsError(
        'invalid-argument',
        'stationName or name must be a valid non-empty string.'
      );
    }

    const parsedStationId = stationId.trim().toUpperCase();

    // 4. Check if station already exists
    const stationExistSnap = await db.ref(`/stations/${parsedStationId}`).once('value');
    const existingData = stationExistSnap.exists() ? stationExistSnap.val() : null;

    // 5. Generate Custom Token
    // We set the UID of the Custom Token to parsedStationId, so when the ESP32 authenticates,
    // auth.uid will equal parsedStationId, matching our RTDB & Firestore security rules.
    const customToken = await admin.auth().createCustomToken(parsedStationId, {
      deviceType: 'esp32',
      stationId: parsedStationId
    });

    // 6. Create or update station node in Realtime DB
    const currentLive = (existingData && existingData.live) ? existingData.live : {
      current: 0.0,
      voltage: 230.0,
      power: 0.0,
      energy: 0.0,
      frequency: 50.0,
      powerFactor: 1.0,
      alert: false,
      alertType: null,
      rssi: 0,
      timestamp: Date.now(),
      firmwareVersion: "0.0.0",
      uptimeSeconds: 0
    };

    const currentConfig = (existingData && existingData.config) ? existingData.config : {};

    await db.ref(`/stations/${parsedStationId}`).set({
      live: currentLive,
      config: {
        highThreshold: currentConfig.highThreshold || 18.0,
        lowThreshold: currentConfig.lowThreshold || 2.0,
        highVoltageThreshold: currentConfig.highVoltageThreshold || 250.0,
        lowVoltageThreshold: currentConfig.lowVoltageThreshold || 200.0,
        reportIntervalSec: currentConfig.reportIntervalSec || 30,
        configPollIntervalSec: currentConfig.configPollIntervalSec || 300,
        stationName: resolvedName,
        pumpPowerKW: pumpPowerKW !== undefined ? parseFloat(pumpPowerKW) : (currentConfig.pumpPowerKW || 1.5),
        lat: lat !== undefined ? parseFloat(lat) : (currentConfig.lat || 47.0707),
        lng: lng !== undefined ? parseFloat(lng) : (currentConfig.lng || 15.4395),
        calibration: currentConfig.calibration || 20.0,
        latestFirmware: currentConfig.latestFirmware || {
          version: "1.0.0",
          url: "",
          checksum: ""
        }
      },
      status: {
        online: (existingData && existingData.status && existingData.status.online) || false,
        lastSeen: (existingData && existingData.status && existingData.status.lastSeen) || Date.now(),
        provisionedAt: Date.now()
      }
    });

    // 7. Store provisioning log in Firestore
    await firestore.collection('devices').doc(parsedStationId).set({
      stationId: parsedStationId,
      stationName: resolvedName,
      provisionedAt: new Date(),
      provisionedBy: callerUid,
      active: true
    }, { merge: true });

    // 8. Increment station counter if brand new
    if (!existingData) {
      await db.ref('metadata/totalStations').transaction((current) => (current || 0) + 1);
    }

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
