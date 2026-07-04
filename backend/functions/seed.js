const admin = require('firebase-admin');

process.env.FIRESTORE_EMULATOR_HOST = "127.0.0.1:8080";
process.env.FIREBASE_AUTH_EMULATOR_HOST = "127.0.0.1:9099";
process.env.FIREBASE_DATABASE_EMULATOR_HOST = "127.0.0.1:9000";

admin.initializeApp({
  projectId: "pumping-station-iot"
});

async function seed() {
  const db = admin.firestore();
  
  // Create a placeholder admin account for local development.
  const email = process.env.ADMIN_EMAIL || 'admin@example.com';
  
  await db.collection('users').doc(email).set({
    email: email,
    role: 'admin',
    createdAt: new Date(),
    isPlaceholder: true
  });

  console.log(`Successfully created placeholder admin for: ${email}`);
}

seed().catch(console.error).finally(() => process.exit(0));
