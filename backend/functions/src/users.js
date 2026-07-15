const functions = require('firebase-functions');
const admin = require('firebase-admin');

exports.onUserWrite = functions.region('europe-west1').firestore
  .document('users/{userId}')
  .onWrite(async (change, context) => {
    const userId = context.params.userId;
    
    // If the user document was deleted
    if (!change.after.exists) {
        return;
    }
    
    const data = change.after.data();
    const role = data.role || 'viewer';
    
    try {
        const isAdmin = role === 'admin';
        await admin.auth().setCustomUserClaims(userId, { admin: isAdmin, role: role });
        console.log(`Successfully set custom claims for user ${userId}: admin=${isAdmin}, role=${role}`);
    } catch (error) {
        console.error(`Failed to set custom claims for user ${userId}`, error);
    }
  });
