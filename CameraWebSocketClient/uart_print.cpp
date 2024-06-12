#include "uart_print.h"

void nextion_print(HardwareSerial *uart, String cmd) {
  uart->print(cmd);
  uart->write(0xFF);
  uart->write(0xFF);
  uart->write(0xFF);
}

void nextion_display(String IDs, uint16_t values, HardwareSerial *uart) {
  String cmd;
  char buf[8] = { 0 };
  sprintf(buf, "%d", values);
  cmd = IDs + ".val=";
  cmd += buf;
  nextion_print(uart, cmd);
}

void serial_err_msg(HardwareSerial *uart, char *msg) {
  uart->print("wrong cmd: ");
  uart->println(msg);
}
void serial_command_help(HardwareSerial *uart, bool sdcard) {
  uart->println("************* help *************");
  uart->println("help    * this text");
  uart->println("reboot  * system reboot");
  uart->println("ssid    * ex)ssid your ssid");
  uart->println("pass    * ex)pass your password");
  uart->println("wifi    * WIFI connet");
  uart->println("   scan * WIFI scan");
  uart->println("   stop * WIFI disconnet");
  uart->println("led     * LED light");
  uart->println("fps     * frame per second");
  uart->println("size    * cam pixel size");
  uart->println("cam     * ");
  uart->println("   on   * cam on");
  uart->println("   off  * cam off");
  if (sdcard) {
    uart->println("ls      * show list");
    uart->println("cd      * move path");
    uart->println("cd/     * move root");
    uart->println("md      * make dir");
    uart->println("rd      * remove dir");
    uart->println("op      * open file");
    uart->println("rf      * remove file");
  }
  uart->println("************* help *************");
}
void serial_wifi_config(HardwareSerial *uart, char *ssid, char *pass) {
  uart->println("********* wifi config *********");
  uart->print("your ssid: ");
  uart->println(ssid);
  uart->print("your pass: ");
  uart->println(pass);
  uart->println("********* wifi config *********");
}