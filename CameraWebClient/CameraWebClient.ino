#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include "esp_camera.h"

#include "EEPROM.h"
#define SERIAL_MAX  32
#define EEPROM_SIZE 16
/******************EEPROM******************/
const uint8_t eep_ssid[EEPROM_SIZE] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
const uint8_t eep_pass[EEPROM_SIZE] = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
/******************EEPROM******************/
char ssid[EEPROM_SIZE];
char password[EEPROM_SIZE]; 
/******************CARMERA******************/
const char* websockets_server_host = "192.168.1.15";
const uint16_t websockets_server_port = 3000;

using namespace websockets;
WebsocketsClient client;

/******************CARMERA******************/
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
/******************PIN SET******************/
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
// 4 for flash led or 33 for normal led
#define LED_GPIO_NUM       4
/*******************************************/
/*******************************************/
// ===========================
// Enter your WiFi credentials
// ===========================
char    Serial_buf[SERIAL_MAX];
int8_t  Serial_num;
void wifi_config_change() {
  String at_cmd     = strtok(Serial_buf, "=");
  String ssid_value = strtok(NULL, "=");
  String pass_value = strtok(NULL, ";");
  if(at_cmd=="AT+WIFI"){
    Serial.print("ssid_value=");
    for (int index = 0; index < EEPROM_SIZE; index++) {
      if(index < ssid_value.length()){
        Serial.print(ssid_value[index]);
        EEPROM.write(eep_ssid[index], byte(ssid_value[index]));
      }else{
        EEPROM.write(eep_ssid[index], byte(0x00));
      }      
    }
    Serial.println("");
    Serial.print("pass_value=");
    for (int index = 0; index < EEPROM_SIZE; index++) {
      if(index < pass_value.length()){
        Serial.print(pass_value[index]);
        EEPROM.write(eep_pass[index], byte(pass_value[index]));
      }else{
        EEPROM.write(eep_pass[index], byte(0x00));
      }    
    }
    Serial.println("");
    EEPROM.commit();
    ESP.restart();
  }else{
    Serial.println(Serial_buf);
  }
}//Command_service() END
void Serial_process() {
  char ch;
  ch = Serial.read();
  switch ( ch ) {
    case ';':
      Serial_buf[Serial_num] = 0x00;
      wifi_config_change();
      Serial_num = 0;
      break;
    default :
      Serial_buf[ Serial_num ++ ] = ch;
      Serial_num %= SERIAL_MAX;
      break;
  }
}
/*******************************************/
uint8_t* camera_frame = nullptr;
size_t frame_length = 0;

void setup() {
  Serial.begin(115200);
  if (!EEPROM.begin(EEPROM_SIZE*2)){
    Serial.println("Failed to initialise eeprom");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  for (int index = 0; index < EEPROM_SIZE; index++) {
    ssid[index]     = EEPROM.read(eep_ssid[index]);
    password[index] = EEPROM.read(eep_pass[index]);
  }
  Serial.println("------- wifi config -------");
  Serial.println("AT+WIFI=SSID=PASS;");
  Serial.print("ssid: "); Serial.println(ssid);
  Serial.print("pass: "); Serial.println(password);
  Serial.println("---------------------------");
  // Connect to WiFi
  WiFi.disconnect(true);
  WiFi.begin(ssid, password);
  unsigned long wifi_config_update = 0UL;
  while (WiFi.status() != WL_CONNECTED) {
    if (Serial.available()) Serial_process();
    unsigned long update_time = millis();
    if(update_time - wifi_config_update > 3000){
      wifi_config_update = update_time;
      Serial.println("Connecting to WiFi..");
    }
  }
  
  WiFi.setSleep(false);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("Connected to WiFi");

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 4;
  config.fb_count = 1;

    // Initialize the camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    // Connect to WebSocket server
    client.connect(websockets_server_host, websockets_server_port, "/");
}

void loop() {
    client.poll();

    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
        // 카메라 프레임 및 길이를 전역 변수에 저장
        camera_frame = fb->buf;
        frame_length = fb->len;

        // WebSocket을 통해 바이너리 데이터 전송
        client.sendBinary(reinterpret_cast<char*>(camera_frame), frame_length);
        esp_camera_fb_return(fb);
    }
    delay(100);
}