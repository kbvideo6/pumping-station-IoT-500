const functions = require('firebase-functions');
const admin = require('firebase-admin');
const { sendAlertEmail } = require('./email');

exports.onLiveDataWrite = functions.database
  .ref('/stations/{stationId}/live')
  .onWrite(async (change, context) => {
    const stationId = context.params.stationId;
    const data = change.after.val();

    if (!data) {
      console.log(`Data deleted or null for station ${stationId}. Skipping alerts.`);
      return null;
    }

    // Check if alert is active
    if (!data.alert) {
      return null;
    }

    const db = admin.database();
    const firestore = admin.firestore();

    try {
      // 1. Fetch station config to retrieve threshold values and human name
      const configSnap = await db.ref(`/stations/${stationId}/config`).once('value');
      const config = configSnap.val() || {};

      // 2. Alert Cooldown Check (5 minutes = 300,000 ms)
      const fiveMinutesAgo = new Date(Date.now() - 5 * 60 * 1000);
      const recentAlertsSnap = await firestore.collection('alerts')
        .where('stationId', '==', stationId)
        .where('type', '==', data.alertType)
        .where('timestamp', '>', fiveMinutesAgo)
        .limit(1)
        .get();

      if (!recentAlertsSnap.empty) {
        console.log(`Alert of type ${data.alertType} for station ${stationId} is in cooldown. Skipping notification.`);
        return null;
      }

      // Determine threshold value
      let thresholdValue = null;
      if (data.alertType === 'HIGH_CURRENT') {
        thresholdValue = config.highThreshold !== undefined ? config.highThreshold : 18.0;
      } else if (data.alertType === 'LOW_CURRENT') {
        thresholdValue = config.lowThreshold !== undefined ? config.lowThreshold : 2.0;
      }

      const alertDoc = {
        stationId,
        stationName: config.stationName || stationId,
        type: data.alertType,
        currentValue: data.current !== undefined ? data.current : null,
        threshold: thresholdValue,
        timestamp: admin.firestore.FieldValue.serverTimestamp(),
        acknowledged: false,
        acknowledgedBy: null,
        acknowledgedAt: null,
        notifiedVia: ['email'],
        emailSentTo: []
      };

      // 3. Retrieve admin users to email
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

      if (emails.length === 0) {
        console.warn('No administrator emails found in Firestore /users collection.');
      }

      // Save alert to database
      const alertRef = await firestore.collection('alerts').add(alertDoc);
      console.log(`Alert logged in Firestore with ID: ${alertRef.id}`);

      // Send emails
      const sentEmails = [];
      for (const email of emails) {
        const success = await sendAlertEmail(email, {
          ...alertDoc,
          timestamp: Date.now() // Use current time for display in email
        });
        if (success) {
          sentEmails.push(email);
        }
      }

      // Update the alert document with the list of successfully sent emails
      if (sentEmails.length > 0) {
        await alertRef.update({
          emailSentTo: sentEmails
        });
      }

      return alertRef.id;
    } catch (err) {
      console.error(`Error in onLiveDataWrite function for station ${stationId}:`, err);
      return null;
    }
  });
