/**
 * Firebase Cloud Function to delete images from Firebase Storage
 * that are older than a specified duration (e.g., 24 hours).
 * This function is scheduled to run daily at 6 AM.
 */

// Import necessary Firebase modules
const functions = require('firebase-functions');
const admin = require('firebase-admin');
const { Storage } = require('@google-cloud/storage');

// Initialize Firebase Admin SDK
// This allows the function to interact with Firebase services
// without explicit authentication (it uses the service account).
admin.initializeApp();

// Initialize Google Cloud Storage client
// This is used to interact with the Storage bucket directly.
const storage = new Storage();

// --- IMPORTANT: Retrieve configuration from environment variables ---
// These variables are set using `firebase functions:config:set`
// and are securely injected into the function's runtime environment.
const BUCKET_NAME = functions.config().app.bucket_name;
const FOLDER_PATH = functions.config().app.folder_path;

// Basic validation to ensure configuration variables are set
if (!BUCKET_NAME) {
    console.error('BUCKET_NAME is not set in Cloud Functions environment configuration. Please run `firebase functions:config:set app.bucket_name="..."`.');
    throw new Error('Missing BUCKET_NAME configuration.');
}
if (!FOLDER_PATH) {
    console.warn('FOLDER_PATH is not set in Cloud Functions environment configuration. Assuming root folder or check your `firebase functions:config:set app.folder_path="..."`.');
    // If FOLDER_PATH is optional or can be empty, handle it here.
    // For now, we'll proceed, but a warning is logged.
}

// Define the age threshold for deletion in milliseconds (24 hours)
const AGE_THRESHOLD_MS = 24 * 60 * 60 * 1000; // 24 hours * 60 min/hr * 60 sec/min * 1000 ms/sec

/**
 * Scheduled Cloud Function to delete old images from Firebase Storage.
 * This function runs daily at 6 AM (America/New_York timezone).
 *
 * The 'every day 06:00' cron job string schedules the function.
 * The 'America/New_York' timezone is chosen for consistency; adjust if needed.
 * For other timezones, refer to: https://cloud.google.com/functions/docs/locations#timezones
 */
exports.deleteOldSpiderImages = functions.pubsub.schedule('every day 06:00')
    .timeZone('America/New_York') // Set your desired timezone
    .onRun(async (context) => {
        console.log('Starting scheduled image deletion process...');
        console.log(`Configured Bucket: ${BUCKET_NAME}, Folder: ${FOLDER_PATH}`);

        const bucket = storage.bucket(BUCKET_NAME);
        const cutoffTime = Date.now() - AGE_THRESHOLD_MS; // Calculate the time 24 hours ago

        try {
            // List all files in the specified folder path
            // The `prefix` option filters files by a common prefix (your folder path).
            const [files] = await bucket.getFiles({ prefix: FOLDER_PATH });

            if (files.length === 0) {
                console.log('No files found in the specified folder.');
                return null; // Exit if no files are found
            }

            const filesToDelete = [];
            console.log(`Found ${files.length} files. Checking for old images...`);

            // Iterate through the files and check their creation time
            for (const file of files) {
                // `file.metadata.timeCreated` is the creation time of the file in ISO 8601 format.
                const creationTime = new Date(file.metadata.timeCreated).getTime();

                // If the file's creation time is older than the cutoff time, add it to the deletion list
                if (creationTime < cutoffTime) {
                    filesToDelete.push(file);
                    console.log(`File to delete: ${file.name} (Created: ${new Date(creationTime).toLocaleString()})`);
                }
            }

            if (filesToDelete.length === 0) {
                console.log('No images found older than 24 hours.');
                return null; // Exit if no old images are found
            }

            console.log(`Attempting to delete ${filesToDelete.length} old images...`);

            // Delete the identified old files
            const deletePromises = filesToDelete.map(async (file) => {
                try {
                    await file.delete();
                    console.log(`Successfully deleted: ${file.name}`);
                } catch (error) {
                    console.error(`Failed to delete ${file.name}:`, error);
                    // Continue with other deletions even if one fails
                }
            });

            // Wait for all deletion promises to resolve
            await Promise.all(deletePromises);

            console.log('Image deletion process completed.');
            return null; // Cloud Functions should return null or a Promise
        } catch (error) {
            console.error('Error during image deletion process:', error);
            // Re-throw the error to indicate function failure
            throw new Error('Failed to delete old images.');
        }
    });
