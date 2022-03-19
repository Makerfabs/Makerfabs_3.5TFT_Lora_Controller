#include "log_save.h"

void SD_init()
{

    if (!SD.begin(SD_CS, SPI))
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

    Serial.println("SD init over.");
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
