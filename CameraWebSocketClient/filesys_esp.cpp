#include "filesys_esp.h"

void sd_init(uint8_t chipSelect, bool *card_insert) {
  // Open serial communications and wait for port to open:
  bool sd_flage = SD.begin(chipSelect);

  for (uint16_t index = 0; index < strlen_P(check_sdcard1); index++) {
    Serial.print(char(pgm_read_byte_near(check_sdcard1 + index)));
  }
  Serial.println();
  if (sd_flage) {
    for (uint16_t index = 0; index < strlen_P(check_sdcard2); index++) {
      Serial.print(char(pgm_read_byte_near(check_sdcard2 + index)));
    }
  }
  Serial.println();
  bool sdcard_check = false;
  while (!SD.begin(chipSelect)) {
    if (sdcard_check) {
      for (uint16_t index = 0; index < strlen_P(check_sdcard4); index++) {
        Serial.print(char(pgm_read_byte_near(check_sdcard4 + index)));
      }
      for (uint16_t index = 0; index < strlen_P(check_sdcard2); index++) {
        Serial.print(char(pgm_read_byte_near(check_sdcard2 + index)));
      }
    } else {
      for (uint16_t index = 0; index < strlen_P(check_sdcard3); index++) {
        Serial.print(char(pgm_read_byte_near(check_sdcard3 + index)));
        delay(20);
      }
    }
    Serial.println();
    if (sdcard_check > 0) {
      Serial.println();
      *card_insert = false;
      return;
    } else {
      sdcard_check = true;
    }
  }
  *card_insert = true;
}

bool exisits_check(String path) {
  fs::FS &fs = SD;
  return fs.exists(path);
}

void dir_make(String path) {
  if (!exisits_check(path)) {
    fs::FS &fs = SD;
    char *path_root = const_cast<char *>(path.c_str());
    String make_dir = "";
    String dir_path = strtok(path_root, "/");
    while (dir_path != "") {
      make_dir += "/" + dir_path;
      dir_path = strtok(0x00, "/");
      fs.mkdir(make_dir);
    }
  }
}

void dir_remove(String path) {
  if (exisits_check(path)) {
    fs::FS &fs = SD;
    uint8_t dir_last = dir_list(path, true, false);
    if (dir_last > 0) {
      for (uint8_t index_d = dir_last; index_d > 0; index_d--) {
        dir_remove(path + "/" + dir_index(path, true, index_d));
      }
    }
    files_all_remove(path);
    fs.rmdir(path);
    Serial.println(path);
    Serial.println(" removed");
  }
}

uint16_t dir_list(String path, bool type, bool show) {
  uint16_t type_index = 0;
  File root = SD.open(path);
  if (!root || !root.isDirectory()) {
    Serial.print(path);
    Serial.println(" not exist");
    root.close();
    return 0;
  }
  File file = root.openNextFile();
  if (show) Serial.println(path);

  while (file) {
    if (show) Serial.print("\t");
    if (file.isDirectory()) {
      if (type) type_index++;
      if (show) {
        Serial.print(file.name());
        Serial.println("/");
      }
    } else {
      if (!type) type_index++;
      if (show) {
        Serial.print(file.name());
        Serial.print("\t");
        Serial.println(file.size());
      }
    }
    file.close();
    file = root.openNextFile();
  }
  file.close();
  root.close();
  return type_index;
}

String dir_index(String path, bool type, uint16_t dir_index) {
  String response = "";
  if (dir_index != 0) {
    File root = SD.open(path);
    File file = root.openNextFile();
    while (file) {
      if (file.isDirectory()) {
        if (type) dir_index--;
      } else {
        if (!type) dir_index--;
      }
      if (dir_index < 1) {
        response = file.name();
        break;
      }
      file.close();
      file = root.openNextFile();
    }
    file.close();
    root.close();
  }
  return response;
}

/******************************************************************/

String file_read(String path) {
  String response = "";
  if (exisits_check(path)) {
    File file;
    fs::FS &fs = SD;
    file = fs.open(path);
    while (file.available()) {
      response += char(file.read());
    }
    file.close();
  }
  return response;
}

void file_write(String path, String contents) {
  File file;
  fs::FS &fs = SD;
  file = fs.open(path, FILE_WRITE);
  file.print(contents);
  file.close();
}

void file_append(String path, uint8_t *contents, uint16_t f_index) {
  File file;
  fs::FS &fs = SD;
  file = fs.open(path, FILE_APPEND);
  for (uint16_t index = 0; index < f_index; index++) {
    file.write(*(contents + index));
  }
  file.close();
}

void file_remove(String path) {
  fs::FS &fs = SD;
  fs.remove(path);
}

void files_all_remove(String path) {
  fs::FS &fs = SD;
  File root = SD.open(path);
  if (!root || !root.isDirectory()) {
    Serial.print(path);
    Serial.println(" not exist");
    root.close();
    return;
  }
  File file = root.openNextFile();
  while (file) {
    String file_name = file.name();
    bool isFile = false;
    if (!file.isDirectory()) isFile = true;
    file.close();
    if (isFile) fs.remove(path + "/" + file_name);
    file = root.openNextFile();
  }
  file.close();
  root.close();
}

/******************************************************************/
void file_stream(String path) {
  File file;
  fs::FS &fs = SD;
  file = fs.open(path);
  Serial.print(path);
  Serial.print(" , size : ");
  Serial.println(file.size());

  while (file.available()) {
    Serial.print(char(file.read()));
  }
  file.close();
}

size_t file_size(String path) {
  size_t response = 0;
  File file;
  fs::FS &fs = SD;
  file = fs.open(path);
  response = file.size();
  return response;
}