import {
  onAuthStateChanged,
  signInWithPopup,
  signOut,
  GoogleAuthProvider,
  type User as FirebaseUser,
} from 'firebase/auth';
import {
  doc,
  getDoc,
  setDoc,
  updateDoc,
  deleteDoc,
  serverTimestamp,
} from 'firebase/firestore';
import { auth, firestore } from './firebase';
import { Utils } from './utils';
import { i18n } from './i18n';

type AuthCallback = (
  user: FirebaseUser | null,
  errorType?: 'ACCESS_DENIED' | 'ERROR'
) => void;

interface UserData {
  role: 'admin' | 'viewer';
  email?: string;
  createdAt?: unknown;
  isPlaceholder?: boolean;
}

export const AuthService = {
  currentUser: null as FirebaseUser | null,
  userRole: null as 'admin' | 'viewer' | null,

  init(onStateChangedCallback: AuthCallback): void {
    onAuthStateChanged(auth, async (user) => {
      if (user) {
        this.currentUser = user;
        try {
          const userRef = doc(firestore, 'users', user.uid);
          let userDoc = await getDoc(userRef);
          let userData: UserData | null = null;

          if (userDoc.exists()) {
            userData = userDoc.data() as UserData;
          } else if (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1') {
            // Auto-create admin for localhost dev only
            userData = { email: user.email!, role: 'admin', isPlaceholder: false };
            await setDoc(userRef, userData);
          } else {
            // Check for email-based placeholder created by admin before first login
            const userEmail = user.email!.toLowerCase();
            const placeholderRef = doc(firestore, 'users', userEmail);
            const placeholderDoc = await getDoc(placeholderRef);

            if (placeholderDoc.exists()) {
              const pd = placeholderDoc.data() as UserData;
              // Migrate placeholder → UID document
              await setDoc(userRef, {
                email: userEmail,
                role: pd.role || 'viewer',
                createdAt: pd.createdAt || serverTimestamp(),
                isPlaceholder: false,
              });
              await deleteDoc(placeholderRef);
              userDoc = await getDoc(userRef);
              userData = userDoc.data() as UserData;
              console.log(`Auto-associated placeholder for ${userEmail} → UID: ${user.uid}`);
            }
          }


          if (userData) {
            this.userRole = userData.role || 'viewer';
            await updateDoc(doc(firestore, 'users', user.uid), {
              lastSeenAt: serverTimestamp(),
              displayName: user.displayName ?? '',
              photoURL: user.photoURL ?? '',
            });
            onStateChangedCallback(user);
          } else {
            console.warn(`Unregistered access attempt by UID: ${user.uid}`);
            this.userRole = null;
            this.currentUser = null;
            await signOut(auth);
            onStateChangedCallback(null, 'ACCESS_DENIED');
          }
        } catch (error) {
          console.error('Authentication check failed:', error);
          onStateChangedCallback(null, 'ERROR');
        }
      } else {
        this.currentUser = null;
        this.userRole = null;
        onStateChangedCallback(null);
      }
    });
  },

  async loginWithGoogle(): Promise<void> {
    const provider = new GoogleAuthProvider();
    try {
      await signInWithPopup(auth, provider);
    } catch (error: unknown) {
      const msg = error instanceof Error ? error.message : i18n.t('unknown_error');
      console.error('Google sign in failed:', error);
      Utils.showToast(i18n.t('login_failed') + msg, 'error');
    }
  },

  async logout(): Promise<void> {
    try {
      await signOut(auth);
      window.location.hash = '#/';
      Utils.showToast(i18n.t('logout_success'), 'success');
    } catch (error) {
      console.error('Sign out failed:', error);
    }
  },

  isAdmin(): boolean {
    return this.userRole === 'admin';
  },

  isAuthenticated(): boolean {
    return this.currentUser !== null;
  },
};
