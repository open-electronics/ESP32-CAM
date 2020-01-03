
#include <ESPAsyncWebServer.h> 
#include <ArduinoWebsockets.h>  
#include <WiFi.h>  
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

#include "soc/soc.h"      
#include "soc/rtc_cntl_reg.h"

#include "index.html.h" 
#include "error404.html.h"


 
 
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char*  mDNS_NAME = "esp32-cam";


#define LED                4
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


 
camera_fb_t * fb = NULL;
  
using namespace websockets;
WebsocketsServer WSserver;
AsyncWebServer webserver(80);

int n_attempts;




// Arduino like analogWrite
// value has to be between 0 and valueMax
void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 180) {
  // calculate duty, 8191 from 2 ^ 13 - 1
  uint32_t duty = (8191 / valueMax) * min(value, valueMax);
  ledcWrite(channel, duty);
}

 

void setup() {
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  
 
  // Ai-Thinker: pins 2 and 12
  ledcSetup(2, 50, 16); //channel, freq, resolution
  ledcAttachPin(2, 2); // pin, channel
  
  ledcSetup(4, 50, 16);
  ledcAttachPin(12, 4);
   
   
  camera_config_t config;
  
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

   
  // camera init
  esp_err_t err = esp_camera_init(&config);

  
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(500);
    ESP.restart();
    //return;
  }

 
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);



  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    n_attempts++;

    if (n_attempts > 5) {
      Serial.println("Restarting");
      ESP.restart();
    } 
  }

  
  Serial.println("");
  Serial.print("Connected to: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("  or  ");

  if (!MDNS.begin(mDNS_NAME)) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
  }
  Serial.print("http://");
  Serial.print(mDNS_NAME);
  Serial.println(".local");



  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, sizeof(index_html_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  webserver.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
   AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", error404_html_gz, sizeof(error404_html_gz));
   response->addHeader("Content-Encoding", "gzip");
   request->send(response);   
 });

  webserver.begin(); 
  WSserver.listen(82);


  MDNS.addService("http", "tcp", 80);  
  

}





void loop() {  
  auto client = WSserver.accept();
  client.onMessage(handle_message);
  
  while (client.available()) {
    client.poll();
    fb = esp_camera_fb_get();
    client.sendBinary((const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    fb = NULL;
  }  
}



 

void handle_message(WebsocketsMessage msg) {
  String PinName;
  int PinValue;
  int panValue ;
  int tiltValue;
  
  String message= msg.data();



  StaticJsonDocument<200> doc;   
                 
  DeserializationError error = deserializeJson(doc, message);


  Serial.print("Raw Message from Browser:");
  Serial.println(message);


  if (error) {
    Serial.println("deserialization Failed");
    return;
  } 


  Serial.println("deserialization OK");
    
  if(message.startsWith("{xy:")){
    
    Serial.println("XY");
   
    panValue = doc["xy"][0];
    tiltValue = doc["xy"][1]; 

    Serial.print("pan:");
    Serial.println(panValue);

    Serial.print("tilt:");
    Serial.println(tiltValue);

    panValue = map(panValue, -90, 90, 0, 180); // 0-180
    tiltValue = map(tiltValue, -90, 90, 180, 0); // 0-180 reversed
 
    ledcAnalogWrite(2, panValue); 
    ledcAnalogWrite(4, tiltValue);
    
  }



  if(message.startsWith("{pin:")){
    
    Serial.println("PIN");
    
    const char* pin = doc["pin"];
    PinValue =  doc["value"];

    Serial.print("PinName:");
    Serial.println(pin);
    
    Serial.print("PinValue:");
    Serial.println(PinValue);
  
    digitalWrite(LED, PinValue);   
     
  }


  
}
