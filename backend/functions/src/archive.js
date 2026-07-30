const functions = require('firebase-functions');
const admin = require('firebase-admin');

exports.archiveReading = functions.region('europe-west1').database
  .ref('/stations/{stationId}/live')
  .onWrite(async (change, context) => {
    const stationId = context.params.stationId;
    const data = change.after.val();

    if (!data) {
      console.log(`No live data written (deleted) for station ${stationId}. Skipping archiving.`);
      return null;
    }

    const previousData = change.before.exists() ? change.before.val() : {};
    const isNewAlert = Boolean(data.alert) && (!previousData.alert || previousData.alertType !== data.alertType);

    let shouldArchive = false;
    let reason = '';

    if (isNewAlert) {
      // Archive immediately when a new alert state occurs or alert type changes
      shouldArchive = true;
      reason = 'alert';
    } else {
      // Check for periodic archiving
      const db = admin.database();
      const [configSnap, statusSnap] = await Promise.all([
        db.ref(`/stations/${stationId}/config`).once('value'),
        db.ref(`/stations/${stationId}/status`).once('value')
      ]);
      
      const config = configSnap.val() || {};
      const status = statusSnap.val() || {};
      
      const intervalMin = config.historyIntervalMin || 15;
      const intervalMs = intervalMin * 60 * 1000;
      const now = Date.now();
      const lastHistory = status.lastHistory || 0;
      
      if (now - lastHistory >= intervalMs) {
        shouldArchive = true;
        reason = 'periodic';
        // Update lastHistory immediately to prevent race conditions
        await db.ref(`/stations/${stationId}/status/lastHistory`).set(admin.database.ServerValue.TIMESTAMP);
      }
    }

    if (!shouldArchive) {
      return null;
    }

    try {
      const firestore = admin.firestore();
      
      const reading = {
        current:     data.current     !== undefined ? data.current     : null,
        voltage:     data.voltage     !== undefined ? data.voltage     : null,
        power:       data.power       !== undefined ? data.power       : null,
        energy:      data.energy      !== undefined ? data.energy      : null,
        frequency:   data.frequency   !== undefined ? data.frequency   : null,
        powerFactor: data.powerFactor !== undefined ? data.powerFactor : null,
        alert:       data.alert       !== undefined ? data.alert       : false,
        alertType:   data.alertType   || null,
        rssi:        data.rssi        !== undefined ? data.rssi        : null,
        timestamp:   new Date(),
        stationId:   stationId
      };

      const docRef = await firestore
        .collection('history')
        .add(reading);

      console.log(`Archived ${reason} reading to history collection with ID: ${docRef.id} for station ${stationId}`);
      return docRef.id;
    } catch (err) {
      console.error(`Failed to archive reading for station ${stationId}:`, err);
      return null;
    }
  });
