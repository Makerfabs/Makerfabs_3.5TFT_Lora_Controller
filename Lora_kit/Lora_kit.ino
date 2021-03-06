//#include <Wire.h>
#include <SPI.h>
#include <LovyanGFX.hpp>
#include <RadioLib.h>
#include <WiFi.h>
#include <time.h>

#include "makerfabs_pin.h"
#include "log_save.h"
#include "touch.h"
#include "Button.h"
#include "Lora.h"
#include "LoraNode.h"

#define SUCCESS 1
#define FALSE 0
#define ASK_CYCLE 5000
#define RECEIVE_LEN 15

#define SOIL_NUM_MAX 8
#define RELAY_NUM_MAX 2

#define SOIL_FILE "/soil_id.txt"
#define RELAY_FILE "/relay_id.txt"
#define WIFI_FILE "/wifi.txt"

#define COLOR_BACKGROUND 0xEF9E
#define COLOR_BUTTON TFT_WHITE
#define COLOR_TEXT 0x322B
#define COLOR_LINE TFT_BLACK
#define COLOR_SHADOW 0x4BAF

//Wifi and ntp
char ssid[40] = "ssid";
char pwd[40] = "121345678";
const char *ntpServer = "120.25.108.11";
const long gmtOffset_sec = (-5) * 60 * 60; //China+8
const int daylightOffset_sec = 0;
struct tm timeinfo;
int16_t timezone = 8;
int wifi_flag = FALSE;
String global_time = "NULL";

struct LGFX_Config
{
    static constexpr spi_host_device_t spi_host = VSPI_HOST;
    static constexpr int dma_channel = 1;
    static constexpr int spi_sclk = LCD_SCK;
    static constexpr int spi_mosi = LCD_MOSI;
    static constexpr int spi_miso = LCD_MISO;
};

static lgfx::LGFX_SPI<LGFX_Config> tft;
static LGFX_Sprite sprite(&tft);
// static lgfx::Panel_ILI9481 panel;
static lgfx::Panel_ILI9488 panel;

//硬件对象
SPIClass SPI_Lora = SPIClass(HSPI);
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, SPI_Lora, SPISettings());
Lora lora(&radio);

//全局传感器对象
//Soil
LoraNode soil_list[8];
int soil_num;
int soil_id_list[8];
String soil_data_list[8];
String soil_time_list[8];

//Relay
LoraNode relay_list[2];
int relay_num;
int relay_id_list[2];

//log
int log_index = 0;

void setup()
{
    Serial.begin(115200);

    pinMode(LCD_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI_Lora.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    //SD(SPI) init
    SD_init();

    //TFT(SPI) init
    set_tft();
    tft.begin();
    tft.setRotation(SCRENN_ROTATION);

    //Touch(I2C) init
    touch_init();

    //Lora init must after HSPI init
    lora.init();

    //Init all soil data
    for (int i = 0; i < 8; i++)
    {
        soil_data_list[i] = "No Reply";
        soil_time_list[i] = "NULL";
    }

    read_wifi_config(SD, WIFI_FILE, ssid, pwd);

    page_title("Initing......");
    wifi_flag = wifi_init();

    welcome_page();
}

void loop()
{
    main_page();
    delay(10);
}

//********************************page**************************
void welcome_page()
{
    // tft.setRotation(1);
    print_img(SD, "/logo.bmp", 480, 320);
    // tft.setRotation(SCRENN_ROTATION);
    delay(3000);
    tft.fillScreen(COLOR_BACKGROUND);
}

void main_page()
{
    Button b_soil[SOIL_NUM_MAX];
    Button b_add_soil;
    Button b_relay[RELAY_NUM_MAX];
    Button b_add_relay;
    Button b_set;
    Button b_clock;

    int pos[2] = {0, 0};
    long lora_runtime = 0;
    long clock_runtime = 0;

    //page init
    page_title("ESP32 TFT LORA");

    //从文件读取soil
    soil_num = read_nodes_id(SD, SOIL_FILE, soil_id_list, SOIL_NUM_MAX);

    for (int i = 0; i < soil_num; i++)
    {
        Serial.println(soil_id_list[i]);
    }

    //土壤传感器最多8个
    if (soil_num > SOIL_NUM_MAX)
        soil_num = SOIL_NUM_MAX;

    //Add a lora node
    for (int i = 0; i < soil_num; i++)
    {
        soil_list[i].setNode(soil_id_list[i], soil_id_list[i] / 10000);
        if (i < 4)
            b_soil[i].set(10, 60 + 50 * i, 140, 40, "NULL", UNABLE);
        else
            b_soil[i].set(170, 60 + 50 * (i - 4), 140, 40, "NULL", UNABLE);
        String show_txt = soil_list[i].getId_str() + " " + soil_list[i].getType_str();
        b_soil[i].setText(show_txt);
        //b_soil[i].setText2("No Reply");
        b_soil[i].setText2(soil_data_list[i]);
        b_soil[i].setText3(soil_time_list[i]);
        b_soil[i].setTextSize(1);
        drawButton_soil(b_soil[i]);
    }

    if (soil_num < SOIL_NUM_MAX)
    {
        b_add_soil.set(170, 210, 140, 40, "Add Soil", ENABLE);
        drawButton(b_add_soil);
    }

    //从文件读取relay
    relay_num = read_nodes_id(SD, RELAY_FILE, relay_id_list, RELAY_NUM_MAX);

    for (int i = 0; i < relay_num; i++)
    {
        Serial.println(relay_id_list[i]);
    }

    //土壤传感器最多2个
    if (relay_num > RELAY_NUM_MAX)
        relay_num = RELAY_NUM_MAX;

    for (int i = 0; i < relay_num; i++)
    {
        relay_list[i].setNode(relay_id_list[i], relay_id_list[i] / 10000);
        b_relay[i].set(330, 60 + 100 * i, 140, 80, "NULL", ENABLE);
        String show_txt = relay_list[i].getId_str() + " " + relay_list[i].getType_str();
        b_relay[i].setText(show_txt);
        b_relay[i].setText2("No Reply");
        b_relay[i].setTextSize(1);
        drawButton(b_relay[i]);
    }

    if (relay_num < RELAY_NUM_MAX)
    {
        b_add_relay.set(330, 210, 140, 40, "Add Relay", ENABLE);
        drawButton(b_add_relay);
    }

    b_set.set(330, 260, 140, 40, "SET", ENABLE);
    b_set.setTextSize(2);
    drawButton(b_set);

    b_clock.set(10, 260, 270, 40, "WIFI UNLINK", UNABLE);
    b_clock.setTextSize(4);
    drawButton_clock(b_clock);

    //延迟两秒防止误触
    delay(2000);

    //Page fresh loop
    while (1)
    {
        //button check
        if (get_touch(pos))
        {
            int button_value = UNABLE;

            //添加Soil
            if ((button_value = b_add_soil.checkTouch(pos[0], pos[1])) != UNABLE)
            {
                if (DEBUG)
                {
                    Serial.println("b_add_soil Button");
                }

                // null_type_page();

                if (add_soil_page() == SUCCESS)
                {
                    Serial.println("Add nodes success");
                }

                break;
            }

            //添加Relay
            if ((button_value = b_add_relay.checkTouch(pos[0], pos[1])) != UNABLE)
            {
                if (DEBUG)
                {
                    Serial.println("b_add_soil Button");
                }

                // null_type_page();

                if (add_relay_page() == SUCCESS)
                {
                    Serial.println("Add nodes success");
                }

                break;
            }

            //配置页面，清空所有节点
            if ((button_value = b_set.checkTouch(pos[0], pos[1])) != UNABLE)
            {
                if (DEBUG)
                {
                    Serial.println("SET Button");
                }

                if (set_page() == SUCCESS)
                {
                    Serial.println("SET clean Over");
                }
                break;
            }

            for (int i = 0; i < relay_num; i++)
            {

                if ((button_value = b_relay[i].checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.printf("Pos is :%d,%d\n", pos[0], pos[1]);
                        Serial.printf("Value is :%d\n", button_value);
                        Serial.printf("Text is :");
                        Serial.println(b_relay[i].getText());
                    }
                    relay4_page_v2(relay_list[i].getId_str());
                    //relay4_control_page(relay_list[i].getId_str());
                    return;
                    //break;
                }
            }
        }

        if ((lora_runtime - millis()) > 1000)
        {
            //Receive Lora
            String rec_str = "";
            rec_str = lora.receive();
            lora_runtime = millis();
            if (!rec_str.equals(""))
            {
                Serial.println(rec_str);
                soil_reply_analyse(rec_str);
                break;
            }
        }

        if (wifi_flag == SUCCESS)
        {
            if ((clock_runtime - millis()) > 1000)
            {
                clock_runtime = millis();

                //Receive Lora
                char time_str[20] = "";

                if (!getLocalTime(&timeinfo))
                {
                    Serial.println("Failed to obtain time");
                }
                else
                {
                    sprintf(time_str, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                    global_time = (String)time_str;
                    b_clock.setText((String)time_str);
                    drawButton_clock(b_clock);
                }
            }
        }

        delay(10);
    }
}

//TYPE03    RELAY4
void relay4_page(String Node_id)
{

    Serial.println("PAGE:lora_relay4_page");

    //Button init
    Button b_control(40, 220, 180, 80, "Control", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("LORA RELAY 4Channel");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_control);
        drawButton(b_back);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > ASK_CYCLE)
            {
                String cut_reply = typical_lora_check_request(Node_id);

                //show Relay status
                tft.fillRect(20, 100, 300, 50, COLOR_BACKGROUND);
                tft.setCursor(20, 100);
                tft.setTextSize(2);
                tft.println(cut_reply);

                runtime = millis();
            }

            //button check

            if (get_touch(pos))
            {

                int button_value = UNABLE;
                if ((button_value = b_control.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    relay4_control_page(Node_id);
                    delay(400);
                    break;
                }
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return;
                }
            }
        }
    }
}

void relay4_control_page(String Node_id)
{
    Serial.println("PAGE:relay_control_page");

    Button b_send(40, 220, 180, 80, "Send", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);

    Button b_relay_switch[4];
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            String relay_name = (String) "Relay" + (2 * i + j) + ":OFF";
            b_relay_switch[2 * i + j].set(40 + 200 * j, 80 + 70 * i, 160, 60, relay_name, 0);
        }
    }

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("Relay CONTROL");

        drawButton(b_send);
        drawButton(b_back);
        for (int i = 0; i < 4; i++)
            drawButton(b_relay_switch[i]);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //button check
            if (get_touch(pos))
            {

                int button_value = UNABLE;
                //Button:Send
                if ((button_value = b_send.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Send Lora Message");
                    }

                    page_alert("Sending....");

                    delay(1000);

                    //lora.send("ID000123ACT114PARAM000000");
                    int message_param = 0;
                    for (int i = 3; i >= 0; i--)
                    {
                        message_param *= 10;
                        message_param += b_relay_switch[i].getValue();
                    }
                    String lora_str = lora.command_format(Node_id, 2, message_param);
                    lora.send(lora_str);

                    tft.setCursor(100, 160);
                    tft.println("Over and Back.");
                    delay(500);
                    return;
                }
                //Button:Back
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return;
                }
                //Button:Relay
                int refresh_flag = 0;
                for (int i = 0; i < 4; i++)
                {
                    if ((button_value = b_relay_switch[i].checkTouch(pos[0], pos[1])) != UNABLE)
                    {
                        if (DEBUG)
                        {
                            Serial.println("Relay Button");
                        }

                        int temp = b_relay_switch[i].getValue();
                        temp = !temp;

                        b_relay_switch[i].setValue(temp);
                        temp_str = (String) "Relay" + i + (temp != 0 ? ":ON" : ":OFF");
                        b_relay_switch[i].setText(temp_str);

                        if (DEBUG)
                        {
                            Serial.print("Button value:");
                            Serial.println(temp);
                            Serial.print("Button text:");
                            Serial.println(temp_str);
                        }
                        refresh_flag = 1;
                        runtime = millis();
                        break;
                    }
                }
                if (refresh_flag == 1)
                {
                    runtime = millis();
                    break;
                }
            }
        }
    }
}

void relay4_page_v2(String Node_id)
{
    Serial.println("PAGE:lora_relay4_page");

    //Button init
    Button b_send(40, 250, 180, 50, "Send", UNABLE);
    Button b_back(240, 250, 180, 50, "Back", 2);

    //Four button
    Button b_relay_switch[4];
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            b_relay_switch[2 * i + j].set(40 + 200 * j, 120 + 60 * i, 160, 50, "NULL", UNABLE);
        }
    }

    int reply_flag = 0;
    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("LORA RELAY 4Channel");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_send);
        drawButton(b_back);

        for (int i = 0; i < 4; i++)
            drawButton(b_relay_switch[i]);

        if (reply_flag == 0)
        {
            tft.setCursor(20, 90);
            tft.setTextSize(2);
            tft.println("Searching device...");
        }

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            if (reply_flag == 0)
            {
                //No reply
                if (millis() - runtime > ASK_CYCLE)
                {
                    String cut_reply = typical_lora_check_request(Node_id);

                    if (cut_reply.indexOf("RELAY4") != -1)
                    {
                        int status = relay4_reply_analyse(cut_reply);
                        for (int i = 0; i < 4; i++)
                        {
                            int temp_value = status % 10;
                            temp_str = (String) "Relay" + i + (temp_value != 0 ? ":ON" : ":OFF");
                            b_relay_switch[i].setText(temp_str);
                            b_relay_switch[i].setValue(temp_value);

                            status /= 10;
                        }
                        reply_flag = 1;
                        b_send.setValue(1);
                        break;
                    }

                    //show Relay status
                    tft.fillRect(20, 90, 300, 20, COLOR_BACKGROUND);
                    tft.setCursor(20, 90);
                    tft.setTextSize(2);
                    tft.println(cut_reply);

                    runtime = millis();
                }
            }

            //button check

            if (get_touch(pos))
            {

                int button_value = UNABLE;
                if ((button_value = b_send.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Send Lora Message");
                    }

                    page_alert("Sending....");

                    delay(1000);

                    //lora.send("ID000123ACT114PARAM000000");
                    int message_param = 0;
                    for (int i = 3; i >= 0; i--)
                    {
                        message_param *= 10;
                        message_param += b_relay_switch[i].getValue();
                    }
                    String lora_str = lora.command_format(Node_id, 2, message_param);
                    lora.send(lora_str);

                    tft.setCursor(100, 160);
                    tft.println("Over and Back.");
                    delay(500);

                    reply_flag = 0;
                    break;
                }
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return;
                }

                //Button:Relay
                int refresh_flag = 0;
                for (int i = 0; i < 4; i++)
                {
                    if ((button_value = b_relay_switch[i].checkTouch(pos[0], pos[1])) != UNABLE)
                    {
                        if (DEBUG)
                        {
                            Serial.println("Relay Button");
                        }

                        int temp = b_relay_switch[i].getValue();
                        temp = !temp;

                        b_relay_switch[i].setValue(temp);
                        temp_str = (String) "Relay" + i + (temp != 0 ? ":ON" : ":OFF");
                        b_relay_switch[i].setText(temp_str);

                        if (DEBUG)
                        {
                            Serial.print("Button value:");
                            Serial.println(temp);
                            Serial.print("Button text:");
                            Serial.println(temp_str);
                        }
                        refresh_flag = 1;
                        runtime = millis();
                        break;
                    }
                }
                if (refresh_flag == 1)
                {
                    runtime = millis();
                    break;
                }
            }
        }
    }
}

//Function page
void null_type_page()
{
    Serial.println("PAGE:null_type_page");

    //Button init
    Button b_back(240, 220, 180, 80, "Back", 2);

    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("NULL TYPE");

        drawButton(b_back);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //button check

            if (get_touch(pos))
            {

                int button_value = UNABLE;
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return;
                }
            }
        }
    }
}

int add_soil_page()
{
    Serial.println("PAGE:add_soil_page");

    Button b_add(40, 220, 180, 80, "Add", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);
    Button b_id(40, 60, 300, 50, "ID000000", 0);

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("ADD SOIL MOISTURE");

        drawButton(b_add);
        drawButton(b_back);
        drawButton(b_id);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //button check
            if (get_touch(pos))
            {
                int button_value = UNABLE;
                //Button:Add
                if ((button_value = b_add.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Add Lora Node");
                    }

                    page_alert("Add....");
                    delay(1000);

                    //Add a lora node
                    if (b_id.getValue() / 10000 == 1)
                    {
                        char node_c[20];
                        sprintf(node_c, "%d;\n", b_id.getValue());
                        appendFile(SD, SOIL_FILE, node_c);

                        tft.setCursor(100, 160);
                        tft.println("Over and Back.");
                        delay(500);
                        return SUCCESS;
                    }
                    else
                    {
                        tft.setCursor(100, 160);
                        tft.println("Soil ID must be 01XXXX.");
                        delay(500);
                        break;
                    }
                }
                //Button:Back
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return FALSE;
                }
                //Button:ID
                if ((button_value = b_id.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }

                    int temp = num_pad(b_id.getValue());
                    if (temp > 999999)
                        temp = 999999;
                    b_id.setValue(temp);
                    char id_cstr[20];
                    sprintf(id_cstr, "ID%06d", temp);
                    b_id.setText((String)id_cstr);

                    if (DEBUG)
                    {
                        Serial.print("Button value:");
                        Serial.println(temp);
                        Serial.print("Button text:");
                        Serial.println(id_cstr);
                    }
                    break;
                }
            }
        }
    }
}

int add_relay_page()
{
    Serial.println("PAGE:add_relay_page");

    Button b_add(40, 220, 180, 80, "Add", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);
    Button b_id(40, 60, 300, 50, "ID000000", 0);

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("ADD RELAY4 DEVICE");

        drawButton(b_add);
        drawButton(b_back);
        drawButton(b_id);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //button check
            if (get_touch(pos))
            {
                int button_value = UNABLE;
                //Button:Add
                if ((button_value = b_add.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Add Lora Node");
                    }

                    page_alert("Add....");
                    delay(1000);

                    //Add a lora node
                    if (b_id.getValue() / 10000 == 3)
                    {
                        char node_c[20];
                        sprintf(node_c, "%d;\n", b_id.getValue());
                        appendFile(SD, RELAY_FILE, node_c);

                        tft.setCursor(100, 160);
                        tft.println("Over and Back.");
                        delay(500);
                        return SUCCESS;
                    }
                    else
                    {
                        tft.setCursor(100, 160);
                        tft.println("Relay4 ID must be 03XXXX.");
                        delay(500);
                        break;
                    }
                }
                //Button:Back
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return FALSE;
                }
                //Button:ID
                if ((button_value = b_id.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }

                    int temp = num_pad(b_id.getValue());
                    if (temp > 999999)
                        temp = 999999;
                    b_id.setValue(temp);
                    char id_cstr[20];
                    sprintf(id_cstr, "ID%06d", temp);
                    b_id.setText((String)id_cstr);

                    if (DEBUG)
                    {
                        Serial.print("Button value:");
                        Serial.println(temp);
                        Serial.print("Button text:");
                        Serial.println(id_cstr);
                    }
                    break;
                }
            }
        }
    }
}

int set_page()
{
    Serial.println("PAGE:set_page");

    Button b_clean(40, 220, 180, 80, "Clean", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("SET PAGE");

        drawButton(b_clean);
        drawButton(b_back);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //button check
            if (get_touch(pos))
            {
                int button_value = UNABLE;
                //Button:Clean
                if ((button_value = b_clean.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Clean All Lora Node");
                    }

                    //Clean lora node
                    writeFile(SD, SOIL_FILE, "");
                    writeFile(SD, RELAY_FILE, "");

                    //Clean all soil data
                    for (int i = 0; i < 8; i++)
                    {
                        soil_data_list[i] = "No Reply";
                        soil_time_list[i] = "NULL";
                    }

                    page_alert("Clean....");
                    delay(1000);
                    tft.setCursor(100, 160);
                    tft.println("Over and Back.");
                    delay(500);

                    return SUCCESS;
                }
                //Button:Back
                if ((button_value = b_back.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }
                    return FALSE;
                }
            }
        }
    }
}

void page_title(String title)
{
    tft.fillScreen(COLOR_BACKGROUND);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(20, 10);
    tft.setTextSize(4);
    tft.println(title);
}

void page_alert(String msg)
{
    tft.fillRect(80, 80, 320, 160, COLOR_BUTTON);
    tft.drawRect(80, 80, 320, 160, COLOR_LINE);
    tft.setCursor(100, 120);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.println(msg);
}
//****************************num_pad*************************

int num_pad(int last_num)
{
    if (DEBUG)
    {
        Serial.println("PAGE:num_pad");
    }
    //input window init
    int output_value = last_num;

    tft.fillScreen(COLOR_BACKGROUND);
    tft.drawRect(20, 20, 440, 80, COLOR_LINE);
    tft.setCursor(40, 40);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(4);
    tft.println(String(output_value));

    //Button init
    int b_value_list[12] = {1, 2, 3, 10, 4, 5, 6, 0, 7, 8, 9, 11};
    String b_txt_list[12] = {"1", "2", "3", "Back", "4", "5", "6", "0", "7", "8", "9", "Enter"};
    Button b_list[12];
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int index = i * 4 + j;
            b_list[index].set(20 + 110 * j, 100 + 70 * i, 110, 70, b_txt_list[index], b_value_list[index]);
            drawButton(b_list[index]);
        }
    }

    //Prevent accidental touch.
    delay(1000);

    //Input loop
    int pos[2] = {0, 0};
    while (1)
    {

        //Check which button was touched
        if (get_touch(pos))
        {

            int button_value = UNABLE;
            for (int i = 0; i < 12; i++)
            {
                if ((button_value = b_list[i].checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.printf("Pos is :%d,%d\n", pos[0], pos[1]);
                        Serial.printf("Value is :%d\n", button_value);
                        Serial.printf("Text is :");
                        Serial.println(b_list[i].getText());
                    }

                    if (button_value == 11)
                    {
                        if (DEBUG)
                        {
                            Serial.printf("Num pad return:%d\n", output_value);
                        }
                        return output_value;
                    }
                    else
                    {
                        //Fresh input window
                        if (button_value != 10)
                        {
                            //Eight digits maximum
                            if (output_value < 10000000)
                                output_value = output_value * 10 + button_value;
                            else
                                break;
                        }
                        else
                            output_value /= 10;

                        tft.fillRect(40, 40, 300, 50, COLOR_BACKGROUND);
                        tft.setCursor(40, 40);
                        tft.setTextSize(4);
                        tft.setTextColor(COLOR_TEXT);
                        tft.println(String(output_value));

                        delay(400);
                        break;
                    }
                }
            }
        }
    }

    return -1;
}

//****************tft************************************

void set_tft()
{
    panel.freq_write = 40000000;
    panel.freq_fill = 40000000;
    panel.freq_read = 16000000;

    panel.spi_cs = LCD_CS;
    panel.spi_dc = LCD_DC;
    panel.gpio_rst = LCD_RST;
    panel.gpio_bl = LCD_BL;

    tft.setPanel(&panel);
}

void drawButton(Button b)
{
    int b_x;
    int b_y;
    int b_w;
    int b_h;
    int shadow_len = 4;
    String text;
    int textSize;

    b.getFoDraw(&b_x, &b_y, &b_w, &b_h, &text, &textSize);

    tft.fillRect(b_x, b_y, b_w, b_h, COLOR_BUTTON);
    tft.drawRect(b_x, b_y, b_w, b_h, COLOR_LINE);
    tft.setCursor(b_x + 20, b_y + 20);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(textSize);
    tft.println(text);

    //Add button shadow
    if (b.getValue() != UNABLE)
    {
        tft.fillRect(b_x + shadow_len, b_y + b_h, b_w, shadow_len, COLOR_SHADOW);
        tft.fillRect(b_x + b_w, b_y + shadow_len, shadow_len, b_h, COLOR_SHADOW);
    }
}

void drawButton_soil(Button b)
{
    int b_x;
    int b_y;
    int b_w;
    int b_h;
    String text;
    String text2;
    String text3;
    int textSize;

    b.getFoDraw(&b_x, &b_y, &b_w, &b_h, &text, &textSize);
    text2 = b.getText2();
    text3 = b.getText3();

    tft.fillRect(b_x, b_y, b_w, b_h, TFT_WHITE);
    tft.drawRect(b_x, b_y, b_w, b_h, COLOR_LINE);
    tft.setCursor(b_x + 20, b_y + 5);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(textSize);
    tft.println(text);
    tft.setCursor(b_x + 20, b_y + 15);
    tft.println(text2);
    tft.setCursor(b_x + 20, b_y + 25);
    tft.println(text3);
}

void drawButton_clock(Button b)
{
    int b_x;
    int b_y;
    int b_w;
    int b_h;
    String text;
    int textSize;

    b.getFoDraw(&b_x, &b_y, &b_w, &b_h, &text, &textSize);

    tft.fillRect(b_x, b_y, b_w, b_h, COLOR_BACKGROUND);
    //tft.drawRect(b_x, b_y, b_w, b_h, COLOR_LINE);
    tft.setCursor(b_x + 5, b_y + 5);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(4);
    tft.println(text);
}

//Display image from file
int print_img(fs::FS &fs, String filename, int x, int y)
{
    File f = fs.open(filename, "r");
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        f.close();
        return 0;
    }

    f.seek(54);
    int X = x;
    int Y = y;
    uint8_t RGB[3 * X];
    for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 3 * X * row);
        f.read(RGB, 3 * X);
        tft.pushImage(0, Y - row, X, 1, (lgfx::rgb888_t *)RGB);
    }

    f.close();
    return 0;
}

//********************Lora Transform and Log Save******************

//A standard, generic program that sends query commands, parses and stores them on an SD card through the Lora protocol.
String typical_lora_check_request(String Node_id)
{
    //send a lora message to a usual lora node
    String lora_str = lora.command_format(Node_id, 114, 0);
    //lora.send("ID000123ACT114PARAM000000");
    lora.send(lora_str);

    //if not reply, show error
    String rec_str = "";
    rec_str = lora.receive();

    //Cut reply don't have node id, it's for screen display.
    String cut_reply = "";

    if (rec_str.equals(""))
    {
        rec_str = "NULL";
        cut_reply = "NO REPLY";
    }
    else if (!rec_str.startsWith(Node_id))
    {
        rec_str = "NULL";
        cut_reply = "NO REPLY";
    }
    else
    {
        char rec_cstr[100];
        rec_str.toCharArray(rec_cstr, 100);
        appendFile(SD, "/lora_log.txt", rec_cstr);
        appendFile(SD, "/lora_log.txt", "\n");
        readFile(SD, "/lora_log.txt");

        cut_reply = lora.reply_analyse(rec_str);
    }
    return cut_reply;
}

//data example:
//ID010003 REPLY : SOIL INEDX:0 H:48.85 T:30.50 ADC:896 BAT:1016
void soil_reply_analyse(String data)
{
    int id = -1;
    int add_adc = data.indexOf("ADC");
    int add_bat = data.indexOf("BAT") - 1;

    id = data.substring(2, 8).toInt();
    Serial.println("Data ID:");
    Serial.println(id);

    for (int i = 0; i < soil_num; i++)
    {
        if (id == soil_id_list[i])
        {
            soil_data_list[i] = data.substring(add_adc, add_bat);
            Serial.println("Data Str:");
            Serial.println(soil_data_list[i]);
            if (wifi_flag == SUCCESS)
            {
                soil_time_list[i] = (String) "Time:" + global_time;
                Serial.println("Time Str:");
                Serial.println(soil_time_list[i]);
            }

            char rec_cstr[100];
            sprintf(rec_cstr, "INDEX%d ID%d %s %s", log_index++, id, soil_data_list[i], global_time);
            Serial.println(rec_cstr);
            appendFile(SD, "/lora_log_soil.txt", rec_cstr);
        }
    }
}

int relay4_reply_analyse(String data)
{
    int status = 0;

    status = data.substring(7, 11).toInt();

    return status;
}

//Load nodes from sd card
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
}

int wifi_init()
{
    Serial.println("Prepare Link Wifi");
    // WiFi.begin(ssid.c_str(), pwd.c_str());
    WiFi.begin(ssid, pwd);

    int connect_count = 0;

    //10S未连接上自动跳过并返回0
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        connect_count++;
        if (connect_count > 20)
        {
            Serial.println("Wifi time out");
            return FALSE;
        }
    }

    // 成功连接上WIFI则输出IP并返回1
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    configTime((const long)(timezone * 3600), daylightOffset_sec, ntpServer);
    Serial.println(F("Alread get npt time."));

    return SUCCESS;
}