// firebase-config.js
// This file contains the Firebase configuration and initialization.

import { initializeApp } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js";
import { getAuth } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-auth.js";
import { getFirestore } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-firestore.js";

// YOUR REAL FIREBASE KEYS GO HERE:
const firebaseConfig = {
  apiKey: "YOUR_API_KEY", // Replace with your Firebase API Key
  authDomain: "finilist-hq.firebaseapp.com",
  projectId: "finilist-hq",
  storageBucket: "finilist-hq.appspot.com",
  messagingSenderId: "YOUR_SENDER_ID",
  appId: "YOUR_APP_ID"
};

// Check if Firebase is actually configured
export const isFirebaseConfigured = firebaseConfig.apiKey !== "YOUR_API_KEY";

let app, auth, db;

if (isFirebaseConfigured) {
    app = initializeApp(firebaseConfig);
    auth = getAuth(app);
    db = getFirestore(app);
} else {
    console.warn("Firebase is NOT configured! Running in 'Local Demo Mode'. Your friend's data will NOT appear until you add your keys.");
}

export { auth, db };
