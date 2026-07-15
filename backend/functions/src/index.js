const admin = require('firebase-admin');

// Initialize Firebase Admin SDK
admin.initializeApp();

// Export Cloud Functions
const { onLiveDataWrite } = require('./alerts');
const { archiveReading } = require('./archive');
const { purgeOldData } = require('./purge');
const { provisionDevice, getDeviceCustomToken } = require('./provision');
const { checkOfflineStations } = require('./offline');
const { onUserWrite } = require('./users');

exports.onLiveDataWrite = onLiveDataWrite;
exports.archiveReading = archiveReading;
exports.purgeOldData = purgeOldData;
exports.provisionDevice = provisionDevice;
exports.getDeviceCustomToken = getDeviceCustomToken;
exports.checkOfflineStations = checkOfflineStations;
exports.onUserWrite = onUserWrite;
