/**
 * Firebase Cloud Function to delete images from Firebase Storage
 * and corresponding entries in Realtime Database
 * that are older than 6 a.m. of the day before today.
 */

const functions = require('firebase-functions');
const admin = require('firebase-admin');
const { Storage } = require('@google-cloud/storage');

// Initialize Firebase Admin SDK
admin.initializeApp();

// Initialize Google Cloud Storage client
const storage = new Storage();

// Retrieve configuration from environment variables
const BUCKET_NAME = functions.config().app.bucket_name;
const FOLDER_PATH = functions.config().app.folder_path;

if (!BUCKET_NAME) {
    console.error('BUCKET_NAME is not set in Cloud Functions environment configuration.');
    throw new Error('Missing BUCKET_NAME configuration.');
}
if (!FOLDER_PATH) {
    console.warn('FOLDER_PATH is not set in Cloud Functions environment configuration.');
}

// Calculate the cutoff time (6 a.m. of the day before today)
function getCutoffTime() {
    const now = new Date();
    now.setDate(now.getDate() - 1); // Go back one day
    now.setHours(6, 0, 0, 0); // Set time to 6 a.m.
    return now.getTime();
}

exports.deleteOldSpiderImages = functions.pubsub.schedule('every day 06:00')
    .timeZone('America/New_York')
    .onRun(async (context) => {
        console.log('Starting scheduled deletion process...');
        console.log(`Configured Bucket: ${BUCKET_NAME}, Folder: ${FOLDER_PATH}`);

        const bucket = storage.bucket(BUCKET_NAME);
        const cutoffTime = getCutoffTime();

        try {
            // Delete files from Firebase Storage
            const [files] = await bucket.getFiles({ prefix: FOLDER_PATH });

            if (files.length === 0) {
                console.log('No files found in the specified folder.');
            } else {
                const filesToDelete = files.filter(file => {
                    const creationTime = new Date(file.metadata.timeCreated).getTime();
                    return creationTime < cutoffTime;
                });

                console.log(`Found ${filesToDelete.length} files to delete.`);
                await Promise.all(filesToDelete.map(file => file.delete()));
                console.log('Successfully deleted old files from Firebase Storage.');
            }

            // Delete entries from Realtime Database
            const dbRef = admin.database().ref(FOLDER_PATH);
            const snapshot = await dbRef.once('value');
            const updates = {};

            snapshot.forEach(childSnapshot => {
                const creationTime = new Date(childSnapshot.val().timeCreated).getTime();
                if (creationTime < cutoffTime) {
                    updates[childSnapshot.key] = null; // Mark for deletion
                }
            });

            if (Object.keys(updates).length > 0) {
                await dbRef.update(updates);
                console.log('Successfully deleted old entries from Realtime Database.');
            } else {
                console.log('No old entries found in Realtime Database.');
            }

            console.log('Deletion process completed.');
        } catch (error) {
            console.error('Error during deletion process:', error);
            throw new Error('Failed to delete old images and database entries.');
        }
    });