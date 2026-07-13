const functions = require('firebase-functions');
const admin = require('firebase-admin');
const { sendAlertEmail } = require('./email');

// Runs every 5 minutes
exports.checkOfflineStations = functions.region('europe-west1').pubsub
  .schedule('*/5 * * * *')
  .onRun(async (context) => {
    const db = admin.database();
    const firestore = admin.firestore();
    
    // Threshold: 5 minutes ago
    const offlineThresholdMs = 5 * 60 * 1000;
    const cutoffTime = Date.now() - offlineThresholdMs;

    try {
      const stationsSnap = await db.ref('/stations').once('value');
      const stations = stationsSnap.val() || {};

      for (const [stationId, station] of Object.entries(stations)) {
        const lastSeen = station.status?.lastSeen || 0;
        const wasOnline = station.status?.online || false;

        // If the station was considered online but hasn't updated in 5 minutes
        if (wasOnline && lastSeen < cutoffTime) {
          console.log(`Station ${stationId} has missed heartbeat. Setting status.online to false.`);

          // 1. Update Realtime DB status to offline
          await db.ref(`/stations/${stationId}/status/online`).set(false);

          // 2. Log an offline alert in Firestore
          const alertDoc = {
            stationId,
            stationName: station.config?.stationName || stationId,
            type: 'DEVICE_OFFLINE',
            currentValue: null,
            threshold: null,
            timestamp: new Date(),
            acknowledged: false,
            acknowledgedBy: null,
            acknowledgedAt: null,
            notifiedVia: ['email'],
            emailSentTo: []
          };

          const alertRef = await firestore.collection('alerts').add(alertDoc);
          console.log(`Logged DEVICE_OFFLINE alert for station ${stationId} with ID: ${alertRef.id}`);

          // 3. Find administrators to email
          const adminsSnap = await firestore.collection('users')
            .where('role', '==', 'admin')
            .get();

          const emails = [];
          adminsSnap.forEach(doc => {
            const u = doc.data();
            if (u.email) {
              emails.push(u.email);
            }
          });

          // 4. Send emails
          const sentEmails = [];
          for (const email of emails) {
            const success = await sendAlertEmail(email, {
              ...alertDoc,
              timestamp: Date.now()
            });
            if (success) {
              sentEmails.push(email);
            }
          }

          if (sentEmails.length > 0) {
            await alertRef.update({
              emailSentTo: sentEmails
            });
          }
        }
      }
      return { success: true };
    } catch (error) {
      console.error('Error occurred checking offline stations:', error);
      return { success: false, error: error.message };
    }
  });
