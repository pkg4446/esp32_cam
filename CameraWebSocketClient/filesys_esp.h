#pragma once
#include <arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>

const PROGMEM char check_sdcard1[] = "SD check...";
const PROGMEM char check_sdcard2[] = "시스템을 시작합니다.";
const PROGMEM char check_sdcard3[] = "SD Card 가 없네요! 카드를 삽입 해주세요!";
const PROGMEM char check_sdcard4[] = "SD Card 없이 ";


#define CMD_UNIT_SIZE 1436

void sd_init(uint8_t chipSelect, bool *card_insert);
bool exisits_check(String path);

void dir_make(String path);
void dir_remove(String path);
uint16_t dir_list(String path, bool type, bool show);
String dir_index(String path, bool type, uint16_t dir_index);

String file_read(String path);
void file_write(String path, String contents);
void file_append(String path, uint8_t *contents, uint16_t f_index);
void file_remove(String path);
void files_all_remove(String root_path);

void file_stream(String path);
size_t file_size(String path);
