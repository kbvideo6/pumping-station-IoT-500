const admin = require('firebase-admin');

// Initialize Firebase Admin SDK
admin.initializeApp();

// Export Cloud Functions
const { onLiveDataWrite } = require('./alerts');
const { archiveReading } = require('./archive');
const { purgeOldData } = require('./purge');
const { provisionDevice } = require('./provision');
const { checkOfflineStations } = require('./offline');

exports.onLiveDataWrite = onLiveDataWrite;
exports.archiveReading = archiveReading;
exports.purgeOldData = purgeOldData;
exports.provisionDevice = provisionDevice;
exports.checkOfflineStations = checkOfflineStations;
