#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <EEPROM.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include "filesys_esp.h"
#include "uart_print.h"

#define COMMAND_LENGTH  32
#define EEPROM_SIZE     16
/******************EEPROM******************/
const uint8_t eep_ssid[EEPROM_SIZE] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
const uint8_t eep_pass[EEPROM_SIZE] = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
/******************EEPROM******************/
char ssid[EEPROM_SIZE];
char password[EEPROM_SIZE]; 
/******************CARMERA******************/
const char* websockets_server_host    = "192.168.1.15";
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
unsigned long pre_update  = 0UL;
uint16_t frame_per_second = 100;
/***************Variable*******************/
char    command_buf[COMMAND_LENGTH];
int8_t  command_num   = 0;
uint8_t path_depth    = 0;
String  path_current  = "/";
bool    wifi_able     = false;
bool    camera_onoff  = true;
bool    sd_card_mode  = true;
/***************Functions******************/
void wifi_config() {
  serial_wifi_config(&Serial,ssid,password);
}
/******************************************/
void WIFI_scan(bool wifi_state){
  wifi_able = wifi_state;
  WiFi.disconnect(true);
  Serial.println("WIFI Scanning…");
  uint8_t networks = WiFi.scanNetworks();
  if (networks == 0) {
    Serial.println("WIFI not found!");
  }else {
    Serial.print(networks);
    Serial.println(" networks found");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    String wifi_list ="";
    for (int index = 0; index < networks; ++index) {
      // Print SSID and RSSI for each network found
      Serial.printf("%2d",index + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(index).c_str());
      Serial.print(" | ");
      Serial.printf("%4d", WiFi.RSSI(index));
      Serial.print(" | ");
      Serial.printf("%2d", WiFi.channel(index));
      Serial.print(" | ");
      byte wifi_type = WiFi.encryptionType(index);
      String wifi_encryptionType;
      if(wifi_type == WIFI_AUTH_OPEN){wifi_encryptionType = "open";}
      else if(wifi_type == WIFI_AUTH_WEP){wifi_encryptionType = "WEP";}
      else if(wifi_type == WIFI_AUTH_WPA_PSK){wifi_encryptionType = "WPA";}
      else if(wifi_type == WIFI_AUTH_WPA2_PSK){wifi_encryptionType = "WPA2";}
      else if(wifi_type == WIFI_AUTH_WPA_WPA2_PSK){wifi_encryptionType = "WPA2";}
      else if(wifi_type == WIFI_AUTH_WPA2_ENTERPRISE){wifi_encryptionType = "WPA2-EAP";}
      else if(wifi_type == WIFI_AUTH_WPA3_PSK){wifi_encryptionType = "WPA3";}
      else if(wifi_type == WIFI_AUTH_WPA2_WPA3_PSK){wifi_encryptionType = "WPA2+WPA3";}
      else if(wifi_type == WIFI_AUTH_WAPI_PSK){wifi_encryptionType = "WAPI";}
      else{wifi_encryptionType = "unknown";}
      Serial.println(wifi_encryptionType);
    }
  }
  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
  if(wifi_able){
    wifi_connect();
  }
}
/******************************************/
void wifi_connect() {
  wifi_config();
  wifi_able = true;
  WiFi.disconnect(true);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long wifi_config_update  = millis();
  while (WiFi.status() != WL_CONNECTED) {
    unsigned long update_time = millis();
    if(update_time - wifi_config_update > 5000){
      wifi_able = false;
      break;
    }
  }
  Serial.println("WIFI connected");
  if(! wifi_able)Serial.println("fail");
}
/******************************************/
void command_service(){
  String cmd_text     = "";
  String temp_text    = "";
  bool   eep_change   = false;
  uint8_t check_index = 0;
  
  for(uint8_t index_check=0; index_check<COMMAND_LENGTH; index_check++){
    if(command_buf[index_check] == 0x20 || command_buf[index_check] == 0x00){
      check_index = index_check+1;
      break;
    }
    cmd_text += command_buf[index_check];
  }
  for(uint8_t index_check=check_index; index_check<COMMAND_LENGTH; index_check++){
    if(command_buf[index_check] == 0x20 || command_buf[index_check] == 0x00){
      check_index = index_check+1;
      break;
    }
    temp_text += command_buf[index_check];
  }
  String file_path = path_current+"/"+temp_text;
  /**********/
  Serial.print("cmd: ");
  Serial.println(cmd_text);

  if(cmd_text=="help"){
    serial_command_help(&Serial,sd_card_mode);
  }else if(cmd_text=="reboot"){
    ESP.restart();
  }else if(cmd_text=="ssid"){
    wifi_able = false;
    WiFi.disconnect(true);
    Serial.print("ssid: ");
    if(temp_text.length() > 0){
      for (int index = 0; index < EEPROM_SIZE; index++) {
        if(index < temp_text.length()){
          Serial.print(temp_text[index]);
          ssid[index] = temp_text[index];
          EEPROM.write(eep_ssid[index], byte(temp_text[index]));
        }else{
          ssid[index] = 0x00;
          EEPROM.write(eep_ssid[index], byte(0x00));
        }
      }
      eep_change = true;
    }
    Serial.println("");
  }else if(cmd_text=="pass"){
    wifi_able = false;
    WiFi.disconnect(true);
    Serial.print("pass: ");
    if(temp_text.length() > 0){
      for (int index = 0; index < EEPROM_SIZE; index++) {
        if(index < temp_text.length()){
          Serial.print(temp_text[index]);
          password[index] = temp_text[index];
          EEPROM.write(eep_pass[index], byte(temp_text[index]));
        }else{
          password[index] = 0x00;
          EEPROM.write(eep_pass[index], byte(0x00));
        }
      }
      eep_change = true;
    }
    Serial.println("");
  }else if(cmd_text=="wifi"){
    if(temp_text=="stop"){
      wifi_able = false;
      Serial.print("WIFI disconnect");
      WiFi.disconnect(true);
    }else if(temp_text=="scan"){
      WIFI_scan(WiFi.status() == WL_CONNECTED);
    }else{
      wifi_connect();
    }
  }else if(cmd_text=="fps"){
    uint8_t cmd_fps = temp_text.toInt();
    frame_per_second = 1000/cmd_fps;
  }else if(cmd_text=="cam"){
    if(temp_text=="on") camera_onoff = true;
    else camera_onoff = false;
  }else if(sd_card_mode){
    if(cmd_text=="ls"){
      dir_list(path_current+"/"+temp_text,true,true);
    }else if(cmd_text=="cd"){
      if(temp_text == "/"){
        path_depth   = 0;
        path_current = "/";
      }else if(temp_text == ".." && path_depth != 0){
        String temp_path = path_current;
        char *upper_path = const_cast<char*>(temp_path.c_str());
        String dir_path  = strtok(upper_path, "/");
        if(path_depth == 1) path_current = "/";
        else path_current = "";
        for(uint8_t index=1; index<path_depth; index++){
          path_current += "/" + dir_path;
          dir_path = strtok(0x00, "/");
        }
        path_depth -= 1;
      }else if(exisits_check(file_path)){
        path_depth += 1;
        if(path_current == "/") path_current += temp_text;
        else path_current += "/"+temp_text;
      }
      Serial.println(path_current);
    }else if(cmd_text=="md"){
      dir_make(file_path);
    }else if(cmd_text=="rd"){
      dir_remove(file_path);
    }else if(cmd_text=="op"){
      file_stream(file_path);
    }else if(cmd_text=="rf"){
      if(temp_text == "*") files_all_remove(path_current);
      else file_remove(file_path);
    }
  }else{ serial_err_msg(&Serial, command_buf); }
  if(eep_change){
    EEPROM.commit();
  }
}
void command_process(char ch) {
  if(ch=='\n'){
    command_buf[command_num] = 0x00;
    command_num = 0;
    command_service();
    memset(command_buf, 0x00, COMMAND_LENGTH);
  }else if(ch!='\r'){
    command_buf[command_num++] = ch;
    command_num %= COMMAND_LENGTH;
  }
}
/*******************************************/
uint8_t* camera_frame = nullptr;
size_t frame_length = 0;

void setup() {
  Serial.begin(115200);
  //SS=13,MOSI=15,SCK=14, MISO=2
  //SPI.begin(SCK, MISO, MOSI, SS);
  SPI.begin(14, 2, 15, 13);
  //chipSelect = SS
  sd_init(13,&sd_card_mode);
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

  // Connect to WiFi
  wifi_connect();
  
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
    if(wifi_able) client.connect(websockets_server_host, websockets_server_port, "/");
}

void loop() {
  unsigned long millisec = millis();
  client.poll();
  if(camera_onoff && millisec > pre_update + frame_per_second){
    pre_update = millisec;
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
        // 카메라 프레임 및 길이를 전역 변수에 저장
        camera_frame = fb->buf;
        frame_length = fb->len;

        // WebSocket을 통해 바이너리 데이터 전송
        client.sendBinary(reinterpret_cast<char*>(camera_frame), frame_length);
        esp_camera_fb_return(fb);
    }
  }
  if(Serial.available()) command_process(Serial.read());
}