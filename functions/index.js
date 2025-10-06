/**
 ** Firebase Cloud Function to delete images from Firebase Storage
 ** and corresponding entries in Realtime Database
 ** that are older than 6 a.m. of the day before today.
 */

const functions = require("firebase-functions");
const admin = require("firebase-admin");
const {Storage} = require("@google-cloud/storage");
const path = require("path");

// Initialize Firebase Admin SDK
// This allows the function to interact with Firebase services.
admin.initializeApp();

// Initialize Google Cloud Storage client
const storage = new Storage();

// --- IMPORTANT: Retrieve configuration from environment variables ---
let BUCKET_NAME;
let FOLDER_PATH;

// Check if running in a local emulator environment
if (process.env.FUNCTIONS_EMULATOR === "true") {
  // Load local .env file
  const fs = require("fs");
  const dotenv = require("dotenv");
  const envConfig = dotenv.parse(fs.readFileSync(
      path.resolve(__dirname, ".env")));

  BUCKET_NAME = envConfig.BUCKET_NAME;
  FOLDER_PATH = envConfig.FOLDER_PATH;

  if (!BUCKET_NAME) {
    console.error("BUCKET_NAME is not set in the local .env file.");
    throw new Error("Missing BUCKET_NAME configuration in .env.");
  }
} else {
  // Use Firebase Functions config for deployed environment
  BUCKET_NAME = functions.config().app.bucket_name;
  FOLDER_PATH = functions.config().app.folder_path;

  if (!BUCKET_NAME) {
    console.error("BUCKET_NAME is not set in Cloud Functions");
    throw new Error("Missing BUCKET_NAME configuration.");
  }
}

// FOLDER_PATH is optional; a warning is logged if it's missing.
if (!FOLDER_PATH) {
  console.warn(
      "FOLDER_PATH is not set in Cloud Functions.",
  );
}

/**
 * Scheduled function to delete old spider images and database entries.
 * This function runs daily at 6 AM (America/New_York timezone).
 */
exports.deleteEverythingEveryTwoDays = functions.pubsub
    .schedule("0 6 */2 * *")
    .timeZone("America/New_York")
    .onRun(async (context) => {
      await performDeletion();
    });

/**
 * Helper function to delete everything
 */
async function performDeletion() {
  console.log("Starting destructive cleanup (every 2 days)...");
  console.log(`Bucket: ${BUCKET_NAME}, Folder: ${FOLDER_PATH || "<all>"}`);

  const bucket = storage.bucket(BUCKET_NAME);

  try {
    // --- Delete all files in storage ---
    const prefix = FOLDER_PATH || "";
    console.log("Listing files to delete (prefix):", prefix);
    const [files] = await bucket.getFiles({prefix});
    if (!files || files.length === 0) {
      console.log("No files found to delete.");
    } else {
      console.log(`Deleting ${files.length} files...`);
      await Promise.all(files.map((file) => file.delete()));
      console.log("Storage cleanup complete.");
    }

    // --- Delete Realtime Database data ---
    if (FOLDER_PATH) {
      console.log("Removing Realtime Database path:", FOLDER_PATH);
      await admin.database().ref(FOLDER_PATH).remove();
      console.log("Realtime Database path removed:", FOLDER_PATH);
    } else {
      console.log("Removing entire Realtime Database root (BE CAREFUL).");
      await admin.database().ref().remove();
      console.log("Realtime Database root removed.");
    }

    console.log("Destructive cleanup finished successfully.");
  } catch (err) {
    console.error("Error during destructive cleanup:", err);
    throw err;
  }
}

/**
 * HTTP trigger to run deletion on-demand.
 */
exports.deleteOldSpiderImagesHttp = functions.https.
    onRequest(async (req, res) => {
      console.log("HTTP trigger received for deletion.");
      try {
        await performDeletion();
        res.status(200).send("Deletion completed.");
      } catch (err) {
        console.error("Error in HTTP deletion:", err);
        res.status(500).send("Deletion failed. Check logs.");
      }
    });
