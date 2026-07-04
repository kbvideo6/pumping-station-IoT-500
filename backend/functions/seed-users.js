const admin = require('firebase-admin');

process.env.FIRESTORE_EMULATOR_HOST = '127.0.0.1:8080';
process.env.FIREBASE_AUTH_EMULATOR_HOST = '127.0.0.1:9099';
process.env.FIREBASE_DATABASE_EMULATOR_HOST = '127.0.0.1:9000';

admin.initializeApp({
  projectId: 'pumping-station-iot'
});

async function seed() {
  const firestore = admin.firestore();
  
  // Create a placeholder admin account.
  // Replace the email below with the Google account email of your first administrator.
  // After seeding, this user can log in and manage additional users from the dashboard.
  const adminEmail = process.env.ADMIN_EMAIL || 'admin@example.com';

  await firestore.collection('users').doc(adminEmail).set({
    email: adminEmail,
    role: 'admin',
    isPlaceholder: true,
    createdAt: new Date()
  });

  console.log(`Successfully seeded placeholder admin for: ${adminEmail}`);
  console.log('This user can now log in via Google and will be auto-promoted to admin.');
}

seed().catch(console.error).finally(() => process.exit(0));
