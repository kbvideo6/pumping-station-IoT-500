const admin = require('firebase-admin');

// Initialize with application default credentials
admin.initializeApp({
  credential: admin.credential.applicationDefault()
});

async function fixClaims() {
  console.log('Fetching all users from Firestore...');
  const db = admin.firestore();
  
  const snapshot = await db.collection('users').where('role', '==', 'admin').get();
  
  if (snapshot.empty) {
    console.log('No admin users found.');
    return;
  }
  
  for (const doc of snapshot.docs) {
    const uid = doc.id;
    console.log(Setting admin: true for UID: );
    try {
      await admin.auth().setCustomUserClaims(uid, { admin: true });
      console.log(Successfully updated claims for );
      // Touch the document to ensure consistency
      await db.collection('users').doc(uid).update({ updatedAt: admin.firestore.FieldValue.serverTimestamp() });
    } catch (err) {
      console.error(Failed to set claims for :, err);
    }
  }
  console.log('Done.');
}

fixClaims().catch(console.error);
