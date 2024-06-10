#include <Arduino.h>

void nextion_print(HardwareSerial *uart, String cmd);
void nextion_display(String IDs, uint16_t values, HardwareSerial *uart);

void serial_err_msg(HardwareSerial *uart, char *msg);
void serial_command_help(HardwareSerial *uart);
void serial_wifi_config(HardwareSerial *uart, char *ssid, char *pass);