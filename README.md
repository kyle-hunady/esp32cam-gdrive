# esp32cam-gdrive
Periodically upload photos from an ESP32CAM to Google Drive.

## Steps:

### 1) Set up the Google Apps Script to receive images
1) Upload the "upload.gs" Google Apps Script to your Google Drive
2) Open > adjust baseFolder to your liking (starting from Google Drive root)
3) Deploy > allow all access > authorize for your account

### 2) Add Google Apps Script to Arduino program
1) Copy script macro string > place into "esp32cam-gdrive.ino"

### 3) Upload and connect your ESP32 to WiFi through your phone
1) On first upload, connect to your ESP32's hotspot (default name: CAM0)
2) Enter WiFi network name and password - you only need to do this once, it will save onto hardware

## Note:
Your images will appear in your GDrive as /baseFolder/imgFolder/timeStamp-imgName.jpg
- **baseFolder** and **timeStamp** are defined in the GApps Script
- **imgFolder** and **imgName** are defined in Arduino program

The GApps Script logs data into a Google Doc to help debug issues with upload
- The Doc is named 'log' and is placed in baseFolder
- It can be disabled in the GApps Script

## Known issue:
First few images (typically 3) either fail to upload to show green tint. I'm not sure what's happening with that or how to fix it. Think you could help?
