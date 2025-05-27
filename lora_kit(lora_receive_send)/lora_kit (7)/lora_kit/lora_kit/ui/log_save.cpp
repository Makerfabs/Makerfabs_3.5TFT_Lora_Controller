#include "log_save.h"

SPIClass spi2(1);   // 第二个 SPI 总线（SPI1）

extern char soil_id0_writer_cnts;
extern char soil_id1_writer_cnts;
extern char soil_id2_writer_cnts;
extern char soil_id3_writer_cnts;
extern char soil_id4_writer_cnts;
extern char soil_id5_writer_cnts;
extern char soil_id6_writer_cnts;
extern char soil_id7_writer_cnts;
void SD_init()
{
    spi2.begin(SD_SCK, SD_MISO, SD_MOSI);
    // if (!SD.begin(SD_CS, SPI))
    if(!SD.begin(SD_CS, spi2, 80000000))
    {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Serial.println("SDHC");
    }
    else
    {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    listDir(SD, "/", 2);
    deleteFile(SD, "/soil0_id.txt");
    deleteFile(SD, "/soil1_id.txt");
    deleteFile(SD, "/soil2_id.txt");
    deleteFile(SD, "/soil3_id.txt");

    deleteFile(SD, "/soil4_id.txt");
    deleteFile(SD, "/soil5_id.txt");
    deleteFile(SD, "/soil6_id.txt");
    deleteFile(SD, "/soil7_id.txt");

    Serial.println("SD init over.");
}
void deleteFile(fs::FS &fs, const char *path) 
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("File written");
    }
    else
    {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("Failed to open file for appending");
        return;
    }
     // 将文件指针移动到文件开头
    file.seek(0);
    if (file.print(message))
    {
        Serial.println("Message appended");
    }
    else
    {
        Serial.println("Append failed");
    }
    file.close();
}

int read_wifi_config(fs::FS &fs, const char *path, char *ssid, char *pwd)
{
    //如果文件不存在
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return 0;
    }

    //遍历文件
    String msg = "";
    int null_flag = 0;
    char c;
    String data[2];
    int data_index = 0;
    Serial.println("Read from file: ");
    while (file.available())
    {

        c = file.read();
        if (c == '\r' || c == '\n')
        {
            if (null_flag == 1)
            {
                Serial.println(msg);
                data[data_index++] = msg;
                msg = "";
                null_flag = 0;

                if (data_index == 2)
                {
                    strcpy(ssid, data[0].c_str());
                    strcpy(pwd, data[1].c_str());
                    break;
                }
            }
        }
        else
        {
            msg += c;
            null_flag = 1;
        }
    }
    file.close();

    Serial.printf("SSID:%s\n", ssid);
    Serial.printf("PWD:%s\n", pwd);

    return 1;
}

int read_nodes_id(fs::FS &fs, const char *path, int *node_id_list, int list_len)
{
    int node_num = 0;

    //如果文件不存在
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return 0;
    }

    //遍历文件
    String msg = "";
    char c;
    Serial.println("Read from file: ");
    while (file.available())
    {

        c = file.read();
        if (c == ';')
        {
            Serial.println(msg);
            node_id_list[node_num++] = msg.toInt();
            msg = "";

            if (node_num > list_len)
                break;
        }
        else if (c == '\r' || c == '\n')
            ;
        else
        {
            msg += c;
        }
    }
    file.close();

    return node_num;
}

void read_soil_file(fs::FS &fs, const char *path, char *data)
{
        //如果文件不存在
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return ;
    }

    // 将文件指针移动到文件开头
    if(strcmp(path, "/soil0_id.txt") == 0)
    {
        if(soil_id0_writer_cnts > 10)
        {
            file.seek((soil_id0_writer_cnts - 10) * 39);
        }
    }
    else if(strcmp(path, "/soil1_id.txt") == 0)
    {
        if(soil_id1_writer_cnts > 10)
        {
            file.seek((soil_id1_writer_cnts - 10) * 39);
        }
    }    
    else if(strcmp(path, "/soil2_id.txt") == 0)
    {
        if(soil_id2_writer_cnts > 10)
        {
            file.seek((soil_id2_writer_cnts - 10) * 39);
        }
    }    
    else if(strcmp(path, "/soil3_id.txt") == 0)
    {
        if(soil_id3_writer_cnts > 10)
        {
            file.seek((soil_id3_writer_cnts - 10) * 39);
        }
    }    
    else if(strcmp(path, "/soil4_id.txt") == 0)
    {
        if(soil_id4_writer_cnts > 10)
        {
            file.seek((soil_id4_writer_cnts - 10) * 39);
        }
    }    
    else if(strcmp(path, "/soil5_id.txt") == 0)
    {
        if(soil_id5_writer_cnts > 10)
        {
            file.seek((soil_id5_writer_cnts - 10) * 39);
        }
    }    
    else if(strcmp(path, "/soil6_id.txt") == 0)
    {
        if(soil_id6_writer_cnts > 10)
        {
            file.seek((soil_id6_writer_cnts - 10) * 39);
        }
    }    
    else if(strcmp(path, "/soil7_id.txt") == 0)
    {
        if(soil_id7_writer_cnts > 10)
        {
            file.seek((soil_id7_writer_cnts - 10) * 39);
        }
    }
    
    //遍历文件
    String msg = "";
    char c;
    int data_index = 0;
    Serial.println("Read from file: ");
    while (file.available())
    {
        c = file.read();
        msg += c;
        if (c == '\n')
        {
            data_index++;
            if(data_index == 10)
            {
                break;
            }
        }
    }

    strcpy(data, msg.c_str());
    file.close();

}
