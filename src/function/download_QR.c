/*
 * robust_image_downloader.c
 *
 * A robust implementation for downloading device-specific QR images
 * without failing, with built-in retry mechanism and error handling.
 *
 * Created on: April 29, 2025
 * Author: Claude
 */

#include <stdio.h>
#include <string.h>
#include "def.h"
#include <coredef.h>
#include <struct.h>
#include <poslib.h>
#include "httpDownload.h"

// Define currency codes to attempt downloading
#define MAX_CURRENCIES 2  // Maximum number of currency variants to try

typedef struct {
    char code[8];        // Currency code (e.g., "KHR", "USD")
    int downloaded;      // 1 if downloaded successfully, 0 if not
} CurrencyVariant;

/*
 * Downloads an image with retry mechanism and detailed error handling
 * Returns: 0 on success, negative value on failure
 */
int download_image_with_retry(const char* url, const char* savePath, int maxRetries) {
    int result;
    int retryCount = 0;
    int backoffDelay = 1000; // Start with 1 second delay between retries

    MAINLOG_L1("Attempting to download image from: %s", url);
    MAINLOG_L1("Will save to: %s", savePath);

    // Try downloading with increasing backoff delays
    do {
        if (retryCount > 0) {
            MAINLOG_L1("Retry attempt %d/%d after failure (code: %d)",
                       retryCount, maxRetries, result);

            // Delete any partial files before retrying
//            DelFile_Api(savePath);

            // Wait with exponential backoff
            Delay_Api(backoffDelay);
            backoffDelay = (backoffDelay * 3) / 2; // Increase delay by 50%
        }

        // Attempt download
        MAINLOG_L1("Downloading Image from: %s", url);
        MAINLOG_L1("Save at Image from: %s", savePath);
        result = httpDownload(url, METHOD_GET, savePath);

        // Check result
        if (result >= 0) {
            MAINLOG_L1("Image successfully downloaded, size: %d bytes", result);
            return 0; // Success
        }

        retryCount++;
    } while (retryCount <= maxRetries);

    MAINLOG_L1("All download attempts failed after %d retries", maxRetries);
    return -1; // Failure after all retries
}

/*
 * Tries to display a downloaded image on screen
 * Returns: 0 on success, -1 on failure
 */
int display_downloaded_image(const char* imagePath) {
    // Try to display the image
    ScrCls_Api();
    if (ScrDispImage_Api(imagePath, 0, 0) != 0) {
        MAINLOG_L1("Warning: Image downloaded but failed to display: %s", imagePath);
        ScrClsRam_Api();
        ScrDispRam_Api(LINE3, 0, "Image downloaded", CDISP);
        ScrDispRam_Api(LINE5, 0, "Display error", CDISP);
        ScrBrush_Api();
        return -1;
    }
    return 0;
}

/*
 * Shows a success message on screen with visual indicators
 */
void display_success_message(const char* message) {
    ScrClsRam_Api();
    ScrDispRam_Api(LINE2, 0, "====================", CDISP);
    ScrDispRam_Api(LINE3, 0, "DOWNLOAD SUCCESSFUL", CDISP);
    ScrDispRam_Api(LINE4, 0, "====================", CDISP);
    ScrDispRam_Api(LINE6, 0, message, CDISP);
    ScrBrush_Api();

    // Briefly flash the screen to indicate success
    Delay_Api(500);
    ScrClsRam_Api();
//    ScrBrush_Api();

}

/*
 * Downloads a single QR image for the specified currency
 * Returns: 0 on success, negative value on failure
 */
int download_currency_qr_image(const char* serverHost, const char* serverPath,
                              const char* currencyCode, const char* saveDir, int maxRetries) {
    char saveLocation[256] = {0};
    char urlDownload[512] = {0};

    // Create full paths using device serial number and currency code
    snprintf(saveLocation, sizeof(saveLocation), "%s%s_%s.jpg",
             saveDir, G_sys_param.sn, currencyCode);

    snprintf(urlDownload, sizeof(urlDownload), "https://%s%s%s/%s_%s.jpg",
             serverHost, serverPath, G_sys_param.sn, G_sys_param.sn, currencyCode);

    MAINLOG_L1("Download QR %s", urlDownload);
    // Show download status to user
    ScrClrLineRam_Api(LINE5, LINE6);
    char statusMsg[64];
    snprintf(statusMsg, sizeof(statusMsg), "Downloading %s QR...", currencyCode);
    ScrDispRam_Api(LINE6, 0, statusMsg, CDISP);
    ScrBrush_Api();

    // Attempt the download with retries
    int result = download_image_with_retry(urlDownload, saveLocation, maxRetries);

    // Verify the download was successful by checking file size
    if (result == 0) {
        int fileSize = GetFileSize_Api(saveLocation);
        if (fileSize > 0) {
            MAINLOG_L1("Download verified for %s, image size: %d bytes", currencyCode, fileSize);

            // Update status display
            ScrClrLineRam_Api(LINE5, LINE6);
            snprintf(statusMsg, sizeof(statusMsg), "%s QR Downloaded", currencyCode);
            ScrDispRam_Api(LINE5, 0, statusMsg, CDISP);
            ScrBrush_Api();

            // Display success message for this currency
            char successMsg[64];
            snprintf(successMsg, sizeof(successMsg), "%s%s Downloaded", "QR ",currencyCode);
            display_success_message(successMsg);
            Delay_Api(1000);  // Show success message for 1 second

            return 0; // Success
        } else {
            MAINLOG_L1("Error: Zero-size file downloaded for %s", currencyCode);

            // Update status display
            ScrClrLineRam_Api(LINE5, LINE6);
            snprintf(statusMsg, sizeof(statusMsg), "%s QR: Empty file", currencyCode);
            ScrDispRam_Api(LINE5, 0, statusMsg, CDISP);
            ScrBrush_Api();

            // Remove empty file
            DelFile_Api(saveLocation);

            return -2; // Empty file
        }
    } else {
        // Update status display with failure
        ScrClrLineRam_Api(LINE5, LINE6);
        snprintf(statusMsg, sizeof(statusMsg), "%s QR: Empty", currencyCode);
        ScrDispRam_Api(LINE5, 0, statusMsg, CDISP);
        ScrBrush_Api();

        return -1; // Download failed
    }
}

/*
 * Downloads multiple QR images for different currencies
 * Returns: Number of successfully downloaded images
 */
int download_multiple_qr_images(const char* serverHost, const char* serverPath) {
    const char* SAVE_DIR = "/ext/tms/";
    const int MAX_RETRIES = 1;
    int successCount = 0;

    // Define currencies to try downloading
    CurrencyVariant currencies[MAX_CURRENCIES] = {
        {"KHR", 0},
        {"USD", 0}
    };

    // Ensure the directory exists
    int dirStatus = GetFileSize_Api(SAVE_DIR);
    if (dirStatus <= 0) {
        MAINLOG_L1("Creating directory: %s", SAVE_DIR);
        fileMkdir_lib(SAVE_DIR);
    }

    // Show download status to user
    ScrCls_Api();
    ScrClsRam_Api();
    ScrDispRam_Api(LINE2, 0, "Downloading QR Images", CDISP);
    ScrDispRam_Api(LINE3, 0, G_sys_param.sn, CDISP);
    ScrDispRam_Api(LINE4, 0, "------------------------", CDISP);
    ScrBrush_Api();

    // Try downloading each currency variant
    for (int i = 0; i < MAX_CURRENCIES; i++) {
        // Skip empty currency codes
        if (strlen(currencies[i].code) == 0) {
            continue;
        }

        int result = download_currency_qr_image(
            serverHost,
            serverPath,
            currencies[i].code,
            SAVE_DIR,
            MAX_RETRIES
        );

        if (result == 0) {
            currencies[i].downloaded = 1;
            successCount++;
        }

        // Short delay between downloads to avoid overwhelming the server
        Delay_Api(300);
    }

    // Show summary of download results
    ScrClsRam_Api();

    // Display overall success message if any downloads succeeded
    if (successCount > 0) {
        char summaryMsg[64];
        snprintf(summaryMsg, sizeof(summaryMsg), "%d of %d QRs downloaded",
                 successCount, MAX_CURRENCIES);

        // Build list of downloaded currencies
        char successList[64] = "";
        int listPos = 0;

        for (int i = 0; i < MAX_CURRENCIES; i++) {
            if (currencies[i].downloaded && strlen(currencies[i].code) > 0) {
                // Only add if there's room in the buffer
                if (listPos + strlen(currencies[i].code) + 2 < sizeof(successList)) {
                    if (listPos > 0) {
                        successList[listPos++] = ',';
                        successList[listPos++] = ' ';
                    }
                    strcpy(successList + listPos, currencies[i].code);
                    listPos += strlen(currencies[i].code);
                }
            }
        }


    } else {
        // Show failure message
        ScrClsRam_Api();
        ScrDispRam_Api(LINE3, 0, "Download Failed", CDISP);
        ScrDispRam_Api(LINE5, 0, "No QR downloaded", CDISP);
        Delay_Api(1000);
        ScrBrush_Api();
        ScrDispImage_Api(KHQRBMP, 0, 0);
    }

    return successCount;
}

/*
 * Main function to handle QR image download
 * This is the function you should call from your application
 */
void download_qr_image_main() {
    // Use this configuration for testing with localhost
    const char* SERVER_HOST = "dgdiot.acledabank.com.kh";
    const char* SERVER_PATH = "/upload/";

    // Display download starting message
    ScrCls_Api();
    ScrClsRam_Api();
    ScrDispRam_Api(LINE3, 0, "QR Image Download", CDISP);
    ScrDispRam_Api(LINE5, 0, "Initializing...", CDISP);
    ScrBrush_Api();
    Delay_Api(1000);

    // Log the serial number being used
    MAINLOG_L1("Starting QR image downloads for device: %s", G_sys_param.sn);

    // Execute the downloads
    int successCount = download_multiple_qr_images(SERVER_HOST, SERVER_PATH);

    if (successCount > 0) {
        MAINLOG_L1("Successfully downloaded %d QR images", successCount);

        // Optionally display one of the downloaded images (e.g., KHR)
        char imagePath[256];
        snprintf(imagePath, sizeof(imagePath), "/ext/tms/%s_KHR.jpg", G_sys_param.sn);
        if (GetFileSize_Api(imagePath) > 0) {
            Delay_Api(1000);
            display_downloaded_image(imagePath);
        }
    } else {
        MAINLOG_L1("Failed to download any QR images");
    }
}

/*
 * Function to handle QR image download from a production server
 */
void download_qr_image_production(const char* serverIp) {
    const char* SERVER_PATH = "/tms/Vanstone/QR/";
    download_multiple_qr_images(serverIp, SERVER_PATH);
}

/*
 * Function to check if specific currency QR files exist and download if missing
 * Returns: Number of files that needed downloading (0 if all existed)
 */
int check_and_download_missing_qr_images(const char* serverHost, const char* serverPath) {
    const char* SAVE_DIR = "/ext/tms/";
    const char* requiredCurrencies[] = {"KHR", "USD"};  // Most important currencies
    const int requiredCount = 2;  // Number of required currencies
    int missingCount = 0;

    // Check for each required currency QR image
    for (int i = 0; i < requiredCount; i++) {
        char imagePath[256];
        snprintf(imagePath, sizeof(imagePath), "%s%s_%s.jpg",
                 SAVE_DIR, G_sys_param.sn, requiredCurrencies[i]);

        // Check if file exists and has content
        int fileSize = GetFileSize_Api(imagePath);
        if (fileSize <= 0) {
            MAINLOG_L1("%s QR image missing or empty, will download", requiredCurrencies[i]);
            missingCount++;
        }
    }

    // If any images are missing, download all currencies
    if (missingCount > 0) {
        MAINLOG_L1("Found %d missing QR images, starting download", missingCount);
        download_multiple_qr_images(serverHost, serverPath);
    } else {
        MAINLOG_L1("All required QR images already present");

        // Display message indicating all QRs are already present
        ScrCls_Api();
        ScrClsRam_Api();
        ScrDispRam_Api(LINE3, 0, "QR Status Check", CDISP);
        ScrDispRam_Api(LINE5, 0, "All QR images are present", CDISP);
        ScrDispRam_Api(LINE7, 0, "No download needed", CDISP);
        ScrBrush_Api();
        Delay_Api(1000);
    }

    return missingCount;
}
