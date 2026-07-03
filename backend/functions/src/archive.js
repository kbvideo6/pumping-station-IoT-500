const functions = require('firebase-functions');
const admin = require('firebase-admin');

exports.archiveReading = functions.database
  .ref('/stations/{stationId}/live')
  .onWrite(async (change, context) => {
    const stationId = context.params.stationId;
    const data = change.after.val();

    if (!data) {
      console.log(`No live data written (deleted) for station ${stationId}. Skipping archiving.`);
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
        timestamp:   admin.firestore.FieldValue.serverTimestamp(),
        stationId:   stationId
      };

      const docRef = await firestore
        .collection('history')
        .doc(stationId)
        .collection('readings')
        .add(reading);

      console.log(`Archived reading to history collection with ID: ${docRef.id} for station ${stationId}`);
      return docRef.id;
    } catch (err) {
      console.error(`Failed to archive reading for station ${stationId}:`, err);
      return null;
    }
  });
