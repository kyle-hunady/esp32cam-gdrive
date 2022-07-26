#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "CameraPins.h"
#include "esp_camera.h"

//////////// CHANGE THESE VALUES AS NEEDED ////////////
#include "MySecrets.h" // this contains MY_SCRIPT macro (not included in repo for security). Delete if desired

String cameraName = "CAM0";
String script = MY_SCRIPT;
// e.g.: String script = "/macros/s/ASLKJDFxkJOIASFvDLSKJxLv.../exec"

framesize_t frameSize = FRAMESIZE_UXGA; // lower res is more stable
long captureDelay = 1000; // ms
#define DEBUG // comment to disable serial print
///////////////////////////////////////////////////////

////////////////////// FRAMESIZES /////////////////////
//FRAMESIZE_96X96    // 96x96
//FRAMESIZE_QQVGA    // 160x120
//FRAMESIZE_QCIF     // 176x144
//FRAMESIZE_HQVGA    // 240x176
//FRAMESIZE_240X240  // 240x240
//FRAMESIZE_QVGA     // 320x240
//FRAMESIZE_CIF      // 400x296
//FRAMESIZE_HVGA     // 480x320
//FRAMESIZE_VGA      // 640x480
//FRAMESIZE_SVGA     // 800x600
//FRAMESIZE_XGA      // 1024x768
//FRAMESIZE_HD       // 1280x720
//FRAMESIZE_SXGA     // 1280x1024
//FRAMESIZE_UXGA     // 1600x1200
///////////////////////////////////////////////////////

///////////////// CUSTOM GLOBAL VALUES ////////////////
long initDelay = 8000; // ms
int numDummyImages = 3; // works after init delay to remove poorly exposed images
#define RESERVED_IMAGEFILE_BYTES 100000 // works well for 3 batches
#define IMAGEFILE_BATCHES 3
///////////////////////////////////////////////////////

int captureCount = 0; // number of successful image uploads

// parameters to be sent to the GApps Script
String folderName = "&folderName="; // defaults to cameraName
String fileName = "&fileName=";     // defaults to cameraName (GApps Script adds timestamp)
String file = "&file=";             // will hold base64 encoded jpeg image

// macros allow you to remove print statements easily when debugging is no longer needed
#ifdef DEBUG    
  #define DEBUG_BEGIN(...)    Serial.begin(__VA_ARGS__)
  #define PRINT(...)          Serial.print(__VA_ARGS__)     
  #define PRINTLN(...)        Serial.println(__VA_ARGS__) 
  #define PRINTF(...)         Serial.printf(__VA_ARGS__) 
#else
  #define DEBUG_BEGIN(...)
  #define PRINT(...)   
  #define PRINTLN(...)  
  #define PRINTF(...)
#endif

///////////////////////////////////////////////////////
/////////////////// HELPER FUNCTIONS //////////////////
///////////////////////////////////////////////////////
void turnOnLight() { // unused
  ledcAttachPin(4, 3);
  ledcSetup(3, 5000, 8);
  ledcWrite(3,10);
}

void turnOffLight() { // unused
  ledcWrite(3,0);
  ledcDetachPin(3);
}

String urlEncode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield(); // not sure what this does, so removed
    }
    return encodedString;
}

// prints message through wifi client
void clientPrint(WiFiClientSecure& clientTCP, const String& msg) {
  // note use of &; these are references
  // they allow you to use the variable you input rather than create a new instance (therefore save memory)
  // but be careful: modifications to them will change the original variable
  // use const to ensure they won't be modified
  
  int msgLength = msg.length();
  for (int i = 0; i < msgLength; i = i+1000) {
    if (i+1000 > msgLength) {
      clientTCP.print(msg.substring(i, msgLength));
    } else {
      clientTCP.print(msg.substring(i, i+1000));
    }
  }
}

#ifdef DEBUG
String getSummary(String msg) {
  return (msg.substring(0,50) + "..." + msg.substring(msg.length()-50,msg.length()));
}
#endif
///////////////////////////////////////////////////////
///////////////// END HELPER FUNCTIONS ////////////////
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
//////////////////// INIT FUNCTIONS ///////////////////
///////////////////////////////////////////////////////
void startWiFi() {
  PRINT("Starting WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setDebugOutput(false);
  // wm.resetSettings();
  char* networkName = &cameraName[0]; // gets pointer for cameraName
  PRINT("connect to \"");
  PRINT(networkName);
  PRINT("\" on your phone to enter WiFi credentials...");
  bool result = wm.autoConnect(networkName, ""); // second param is password (blank for none)

  if(!result) {
    PRINTLN("failed");
    ESP.restart();
  } 
  else {
    PRINTLN("success");
    PRINT("STA IP address: ");
    PRINTLN(WiFi.localIP()); 
  }
}

void wait(long duration) {
  long start = millis();
  while (millis() - start < duration) {}//do nothing
}

void startCamera() {
  camera_config_t cameraConfig;
  cameraConfig.ledc_channel = LEDC_CHANNEL_0;
  cameraConfig.ledc_timer = LEDC_TIMER_0;
  cameraConfig.pin_d0 = Y2_GPIO_NUM;
  cameraConfig.pin_d1 = Y3_GPIO_NUM;
  cameraConfig.pin_d2 = Y4_GPIO_NUM;
  cameraConfig.pin_d3 = Y5_GPIO_NUM;
  cameraConfig.pin_d4 = Y6_GPIO_NUM;
  cameraConfig.pin_d5 = Y7_GPIO_NUM;
  cameraConfig.pin_d6 = Y8_GPIO_NUM;
  cameraConfig.pin_d7 = Y9_GPIO_NUM;
  cameraConfig.pin_xclk = XCLK_GPIO_NUM;
  cameraConfig.pin_pclk = PCLK_GPIO_NUM;
  cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
  cameraConfig.pin_href = HREF_GPIO_NUM;
  cameraConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  cameraConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
  cameraConfig.pin_reset = RESET_GPIO_NUM;
  cameraConfig.xclk_freq_hz = 20000000;
  cameraConfig.pixel_format = PIXFORMAT_JPEG;
  cameraConfig.grab_mode = CAMERA_GRAB_LATEST; // important for ensuring correct image is pulled from buffer
  
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    PRINTLN("PSRAM found");
    cameraConfig.frame_size = FRAMESIZE_UXGA;
    cameraConfig.jpeg_quality = 10;  //0-63 lower number means higher quality
    cameraConfig.fb_count = 2;
  } else {
    PRINTLN("PSRAM not found");
    cameraConfig.frame_size = FRAMESIZE_SVGA;
    cameraConfig.jpeg_quality = 12;  //0-63 lower number means higher quality
    cameraConfig.fb_count = 1;
  }

  // initialize camera
  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    PRINTF("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  // set desired frame size
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, frameSize);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA 

  // wait before capturing to allow auto-exposure to adjust (discussed in https://github.com/espressif/esp32-camera/issues/314)
  // higher resolutions will take longer
  PRINT("Waiting " + String(initDelay) + "ms after init..."); // 8sec delay + 3 dummy captures seems to work on UXGA framesize
  wait(initDelay);
  PRINTLN("done");
  
  PRINT("Capturing " + String(numDummyImages) + " dummy images...");
  for (int i = 0; i < numDummyImages; i++) {
    camera_fb_t* fb = esp_camera_fb_get(); 
    esp_camera_fb_return(fb);
  }
  PRINTLN("done");
}
///////////////////////////////////////////////////////
////////////////// END INIT FUNCTIONS /////////////////
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
///////////// CAPTURE AND UPLOAD FUNCTIONS ////////////
///////////////////////////////////////////////////////
camera_fb_t* captureImage() {
  
  PRINT("Capturing image...");
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    PRINTLN("failed. Restarting");
    ESP.restart();
  } else {
    PRINTLN("success");
    return fb;
  }
}

int getImageFileLength(camera_fb_t* fb, WiFiClientSecure& clientTCP, const bool printToClient) {
  
  int totalLength = 0;

  const int base64EncodeBatchSize = 3; // not to be confused with an imageFile batchSize
  char *input = (char *)fb->buf; // pointer to image (frame) buffer
  char output[base64_enc_len(base64EncodeBatchSize)]; // char array initialized with a known size (when batchSize of data is encoded to base64)
  int len = fb->len; // length of frame buffer

  int batchSize = len/IMAGEFILE_BATCHES;
  while (batchSize % base64EncodeBatchSize != 0) {batchSize++;} // keep batchSize divisible by base64EncodeBatchSize

  // create array of indicies where one batch ends and another begins
  // for example, if the frame buffer is 100,000 long, and we have 3 batches, batchSize = 33333, indicies = [33333, 66666]
  int indicies[IMAGEFILE_BATCHES - 1];
  for (int i = 0; i < IMAGEFILE_BATCHES - 1; i++) {
    indicies[i] = batchSize * (i + 1);
  }

  // initialize imageFile string (it will contain image data, so it will be large!)
  String imageFile;
  imageFile.reserve(RESERVED_IMAGEFILE_BYTES); // set aside space for imageFile
  imageFile += "data:image/jpeg;base64,"; // prefix for jpeg

  int currentBatch = 0;
  
  // print frame buffer to wifi client
  for (int i=0; i <= len; i++) {
    base64_encode(output, (input++), base64EncodeBatchSize);

    // do url encoding on every base64 batch
    if (i % base64EncodeBatchSize == 0) {imageFile += urlEncode(String(output));}

    // if we've reached the end of a imageFile batch, print batch and reset imageFile to save memory
    if (i == indicies[currentBatch]) {
      if (printToClient) {clientPrint(clientTCP, imageFile);} // print batch
      totalLength += imageFile.length();
      imageFile = ""; // reset
      currentBatch++;
    }
  }

  if (printToClient) {clientPrint(clientTCP, imageFile);} // print last batch
  totalLength += imageFile.length();
  
  return totalLength;
}


bool uploadImage(camera_fb_t* fb) {  
  const char* scriptDomain = "script.google.com";
  
  PRINT("Connecting to " + String(scriptDomain) + "...");
  WiFiClientSecure clientTCP;
  clientTCP.setInsecure(); // need insecure; won't connect to script.google.com otherwise

  // 443 is standard port for transmission control protocol (TCP)
  if (clientTCP.connect(scriptDomain, 443)) {
    PRINTLN("success");
    
    bool printToClient = false;
    int imageFileLength = getImageFileLength(fb, clientTCP, printToClient);
    PRINT(F("File length: "));
    PRINTLN(imageFileLength);
    
    String params = folderName + fileName + file;
    PRINT("Uploading");
    clientTCP.println("POST " + script + " HTTP/1.1");
    clientTCP.println("Host: " + String(scriptDomain));
    clientTCP.println("Content-Length: " + String(params.length()+imageFileLength));
    clientTCP.println("Content-Type: application/x-www-form-urlencoded");
    clientTCP.println("Connection: keep-alive");
    clientTCP.println();
    
    clientTCP.print(params); // sends completed folderName and fileName; file will be added in loop:

    printToClient = true;
    getImageFileLength(fb, clientTCP, printToClient);
        
    esp_camera_fb_return(fb); // clear image from camera buffer
    
    int waitTime = 10000; // timeout 10 seconds
    long startTime = millis();
    boolean firstNewLineFound = false;
    String body = "";
    
    // wait for response
    while ((startTime + waitTime) > millis()) {
      PRINT(".");
      wait(100);     
      while (clientTCP.available()) {
          char c = clientTCP.read();
          // if document has started, save stream
          if (firstNewLineFound==true) {body += String(c);}
          // if we find the first newline string, begin saving future characters
          if (c == '\n' && body.length() == 0) {firstNewLineFound=true;}
       }
       if (body.length()>0) break; // break if we have grabbed info
    }
    clientTCP.stop();

    if (body.length()>0) {
      // check if exception exists in response
      int exceptionStart = body.indexOf("Exception:");
      int exceptionEnd = body.indexOf("</div>", exceptionStart);
      if (exceptionStart > 0) {
        PRINTLN("error");
        PRINTLN(body.substring(exceptionStart, exceptionEnd)); // print exception
        return false;
      } else {
        captureCount++;
        PRINT("success (");
        PRINT(captureCount);
        PRINTLN(")");
        return true;
      }
    } else {
      PRINT("failed (timeout)");
      return false; // timeout
    }
  }
  else {
    PRINTLN("failed");
    return false;
  }  
}
///////////////////////////////////////////////////////
/////////// END CAPTURE AND UPLOAD FUNCTIONS //////////
///////////////////////////////////////////////////////

///////////////////////////////////////////////////////
//////////////////// SETUP AND LOOP ///////////////////
///////////////////////////////////////////////////////
void setup() {
  // disable brown-out detection (WiFi and camera can cause voltage drops)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  DEBUG_BEGIN(115200);
  wait(10);

  folderName += cameraName;
  fileName += cameraName + ".jpg";

  startCamera();
  startWiFi();
}

void loop() {
  uploadImage(captureImage());
  PRINT("Waiting " + String(captureDelay) + "ms...");
  wait(captureDelay);
  PRINTLN("done");
}
///////////////////////////////////////////////////////
////////////////// END SETUP AND LOOP /////////////////
///////////////////////////////////////////////////////
