const functions = require('firebase-functions');
const admin = require('firebase-admin');

// Runs daily at 02:00 UTC
exports.purgeOldData = functions.pubsub
  .schedule('0 2 * * *')
  .timeZone('UTC')
  .onRun(async (context) => {
    const firestore = admin.firestore();
    const db = admin.database();
    
    // Retention period: 30 days
    const retentionMs = 30 * 24 * 60 * 60 * 1000;
    const cutoffDate = new Date(Date.now() - retentionMs);
    
    console.log(`Starting data purge. Cutoff date: ${cutoffDate.toISOString()}`);

    try {
      // 1. Fetch all station IDs from Realtime Database
      const stationsSnap = await db.ref('stations').once('value');
      const stations = stationsSnap.val() || {};
      const stationIds = Object.keys(stations);

      console.log(`Found ${stationIds.length} stations to check for historical data purge.`);

      for (const stationId of stationIds) {
        let deletedCount = 0;
        let hasMore = true;

        // Delete in batches of 400 to avoid memory limit / transaction timeout
        while (hasMore) {
          const oldReadingsSnap = await firestore.collection('history')
            .doc(stationId)
            .collection('readings')
            .where('timestamp', '<', cutoffDate)
            .limit(400)
            .get();

          if (oldReadingsSnap.empty) {
            hasMore = false;
            break;
          }

          const batch = firestore.batch();
          oldReadingsSnap.forEach(doc => {
            batch.delete(doc.ref);
          });

          await batch.commit();
          deletedCount += oldReadingsSnap.size;
          console.log(`Purged batch of ${oldReadingsSnap.size} readings for station ${stationId}`);
          
          if (oldReadingsSnap.size < 400) {
            hasMore = false;
          }
        }
        
        if (deletedCount > 0) {
          console.log(`Successfully purged ${deletedCount} readings total for station ${stationId}`);
        }
      }

      // 2. Also purge old acknowledged alerts (>30 days)
      let alertsDeleted = 0;
      let alertsHasMore = true;

      while (alertsHasMore) {
        const oldAlertsSnap = await firestore.collection('alerts')
          .where('acknowledged', '==', true)
          .where('timestamp', '<', cutoffDate)
          .limit(400)
          .get();

        if (oldAlertsSnap.empty) {
          alertsHasMore = false;
          break;
        }

        const batch = firestore.batch();
        oldAlertsSnap.forEach(doc => {
          batch.delete(doc.ref);
        });

        await batch.commit();
        alertsDeleted += oldAlertsSnap.size;
        console.log(`Purged batch of ${oldAlertsSnap.size} acknowledged alerts`);

        if (oldAlertsSnap.size < 400) {
          alertsHasMore = false;
        }
      }

      console.log(`Successfully purged ${alertsDeleted} acknowledged alerts total.`);
      return { success: true, stationPurgesCount: stationIds.length, alertsDeletedCount: alertsDeleted };
    } catch (error) {
      console.error('Error occurred during scheduled data purge:', error);
      return { success: false, error: error.message };
    }
  });
