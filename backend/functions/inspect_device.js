const admin = require('firebase-admin');
admin.initializeApp({
  projectId: 'argus360-c0496'
});
const db = admin.firestore();
db.collection('devices').doc('STATION_002').get().then(doc => {
  if (doc.exists) {
    console.log('STATION_002:', JSON.stringify(doc.data(), null, 2));
  } else {
    console.log('STATION_002 does not exist in Firestore!');
  }
  process.exit(0);
}).catch(err => {
  console.error(err);
  process.exit(1);
});
