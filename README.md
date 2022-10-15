# esp32cam-gdrive
Periodically upload photos from an ESP32CAM to Google Drive. This code is implements a fix over other repos to upload the highest resolution (UXGA).

## Steps:
### 1) Set up the Google Apps Script to receive images
1) Upload the "upload.gs" Google Apps Script to your Google Drive
2) Open > adjust baseFolder to your liking (starting from Google Drive root)
3) Deploy > allow all access > authorize for your account

### 2) Add Google Apps Script to Arduino program
1) In Google Apps deploy, copy script macro string > place into "esp32cam-gdrive.ino"

### 3) Upload and connect your ESP32 to WiFi through your phone
1) On first upload, connect to your ESP32's hotspot (default name: CAM0)
2) Enter WiFi network name and password - you only need to do this once, it will save onto hardware

## Note:
This is not meant for speedy uploads - encoding the image to a string and uploading to Drive takes several seconds.

Your images will appear in your GDrive as /**baseFolder**/_imgFolder_/**timeStamp**-_imgName_.jpg
- **baseFolder** and **timeStamp** are defined in the GApps Script
- imgFolder and imgName are defined in Arduino program

The GApps Script logs data into a Google Doc to help debug issues with upload
- The Doc is named 'log' and is placed in baseFolder
- It can be disabled in the GApps Script
