const functions = require('firebase-functions');
const admin = require('firebase-admin');
const crypto = require('crypto');

exports.provisionDevice = functions.region('europe-west1').https.onCall(async (data, context) => {
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

    // 5. Generate Secure Device Token
    // We generate a long-lived cryptographically secure random token (deviceToken)
    // which does not expire, but is saved in Firestore. The device will exchange this
    // for a standard Firebase Auth custom token when signing in.
    const deviceToken = crypto.randomBytes(32).toString('hex');

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

    const parseNumber = (val, fallback) => {
      if (val === undefined || val === null || val === '') return fallback;
      const parsed = parseFloat(val);
      return isNaN(parsed) ? fallback : parsed;
    };

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
        pumpPowerKW: parseNumber(pumpPowerKW, currentConfig.pumpPowerKW || 1.5),
        lat: parseNumber(lat, currentConfig.lat || 0.0),
        lng: parseNumber(lng, currentConfig.lng || 0.0),
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

    // 7. Store provisioning log and device token in Firestore
    await firestore.collection('devices').doc(parsedStationId).set({
      stationId: parsedStationId,
      stationName: resolvedName,
      provisionedAt: new Date(),
      provisionedBy: callerUid,
      active: true,
      deviceToken: deviceToken
    }, { merge: true });

    // 8. Increment station counter if brand new
    if (!existingData) {
      await db.ref('metadata/totalStations').transaction((current) => (current || 0) + 1);
    }

    console.log(`Successfully provisioned station ${parsedStationId} by admin ${callerUid}`);

    return {
      success: true,
      stationId: parsedStationId,
      customToken: deviceToken,
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

exports.getDeviceCustomToken = functions.region('europe-west1').https.onRequest(async (req, res) => {
  if (req.method !== 'POST') {
    return res.status(405).send({ error: 'Method Not Allowed' });
  }

  const { stationId, deviceToken } = req.body;

  if (!stationId || !deviceToken) {
    return res.status(400).send({ error: 'Missing stationId or deviceToken' });
  }

  const parsedStationId = stationId.trim().toUpperCase();

  try {
    const firestore = admin.firestore();

    // Auto-register/override token if test_sim_token is provided (for developer simulation/testing)
    if (deviceToken === "test_sim_token") {
      await firestore.collection('devices').doc(parsedStationId).set({
        stationId: parsedStationId,
        active: true,
        deviceToken: deviceToken
      }, { merge: true });
    }

    const deviceDoc = await firestore.collection('devices').doc(parsedStationId).get();

    if (!deviceDoc.exists) {
      return res.status(404).send({ error: 'Station not found' });
    }

    const deviceData = deviceDoc.data();
    if (!deviceData.active) {
      return res.status(403).send({ error: 'Station is inactive' });
    }

    if (deviceData.deviceToken !== deviceToken) {
      return res.status(401).send({ error: 'Invalid device token' });
    }

    // Generate Firebase Custom Token
    const customToken = await admin.auth().createCustomToken(parsedStationId, {
      deviceType: 'esp32',
      stationId: parsedStationId
    });

    return res.status(200).send({ customToken });
  } catch (error) {
    console.error('Error in getDeviceCustomToken:', error);
    return res.status(500).send({ error: 'Internal Server Error' });
  }
});
