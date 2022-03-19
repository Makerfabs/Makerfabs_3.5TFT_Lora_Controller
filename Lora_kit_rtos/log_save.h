#ifndef _Log_Save_H_
#define _Log_Save_H_

#include <SD.h>
#include <FS.h>

#include "makerfabs_pin.h"

void SD_init();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
int read_wifi_config(fs::FS &fs, const char *path, char *ssid, char *pwd);
int read_nodes_id(fs::FS &fs, const char *path, int *node_id_list, int list_len);

#endif
