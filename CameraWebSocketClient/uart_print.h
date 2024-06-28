#include <Arduino.h>

void serial_err_msg(HardwareSerial *uart, char *msg);
void serial_command_help(HardwareSerial *uart, bool sdcard);
void serial_wifi_config(HardwareSerial *uart, char *ssid, char *pass);