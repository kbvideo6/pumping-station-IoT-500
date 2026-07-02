import { initializeApp } from 'firebase/app';
import { getAuth, connectAuthEmulator } from 'firebase/auth';
import { getDatabase, connectDatabaseEmulator } from 'firebase/database';
import { getFirestore, connectFirestoreEmulator } from 'firebase/firestore';
import { getFunctions, connectFunctionsEmulator } from 'firebase/functions';

// ── Firebase project configuration ───────────────────────────
// Replace placeholder values with your actual Firebase project credentials
// before deploying. Obtain them from: Firebase Console → Project Settings → Web App.
const firebaseConfig = {
  apiKey: "AIzaSyA-Mmub9KEdSiZqGvJ4-rgBndQVPNybAA8",
  authDomain: "pumping-station-iot.firebaseapp.com",
  databaseURL: "https://pumping-station-iot-default-rtdb.europe-west1.firebasedatabase.app",
  projectId: "pumping-station-iot",
  storageBucket: "pumping-station-iot.firebasestorage.app",
  messagingSenderId: "214766592563",
  appId: "1:214766592563:web:aab47f06dc5408269c3946",
  measurementId: "G-5STLHJR7VH",
};

const app = initializeApp(firebaseConfig);

export const auth = getAuth(app);
export const db = getDatabase(app);
export const firestore = getFirestore(app);
export const functions = getFunctions(app);

/*if (typeof window !== 'undefined' && (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1')) {
  console.log('Connecting to local Firebase emulators...');
  connectAuthEmulator(auth, 'http://127.0.0.1:9099');
  connectDatabaseEmulator(db, '127.0.0.1', 9000);
  connectFirestoreEmulator(firestore, '127.0.0.1', 8080);
  connectFunctionsEmulator(functions, '127.0.0.1', 5001);
}*/

