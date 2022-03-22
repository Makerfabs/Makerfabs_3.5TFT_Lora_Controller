#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <SPI.h>
#include <LovyanGFX.hpp>
#include <WiFi.h>
#include <time.h>

#include "makerfabs_pin.h"
#include "touch.h"
#include "Lora.h"
#include "LoraNode.h"
#include "Button.h"
#include "log_save.h"

#define SUCCESS 1
#define FALSE 0
#define ASK_CYCLE 5000

#define COLOR_BACKGROUND 0xEF9E
#define COLOR_BUTTON TFT_WHITE
#define COLOR_TEXT 0x322B
#define COLOR_LINE TFT_BLACK
#define COLOR_SHADOW 0x4BAF

#define SOIL_FILE "/soil_id.txt"
#define RELAY_FILE "/relay_id.txt"
#define MOS_FILE "/mos_id.txt"
#define WIFI_FILE "/wifi.txt"
#define SOIL_LOG_FILE "/lora_log_soil.txt"

#define SOIL_NUM_MAX 8
#define RELAY_NUM_MAX 2

//TFT
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
static lgfx::Panel_ILI9488 panel;

//Lora
SPIClass SPI_Lora = SPIClass(HSPI);
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, SPI_Lora, SPISettings());
Lora lora(&radio);

//Global value
String test_str = "";
char ssid[40] = "ssid";
char pwd[40] = "121345678";
int wifi_flag = FALSE;
const char *ntpServer = "120.25.108.11";
const long gmtOffset_sec = (-5) * 60 * 60; //China+8
const int daylightOffset_sec = 0;
struct tm timeinfo;
String global_time = "WIFI UNLINK";
int time_fresh_flag = FALSE;
int log_index = 0;
int page_fresh_flag = FALSE;

//Lora node
int lora_task_index = 0;
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

void setup()
{
    Serial.begin(115200);

    pinMode(LCD_CS, OUTPUT);
    // pinMode(SD_CS, OUTPUT);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI_Lora.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    SD_init();
    read_wifi_config(SD, WIFI_FILE, ssid, pwd);
    wifi_flag = wifi_init();

    //Init all soil data
    for (int i = 0; i < 8; i++)
    {
        soil_data_list[i] = "No Reply";
        soil_time_list[i] = "NULL";
    }

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 3, NULL, ARDUINO_RUNNING_CORE);
    xTaskCreatePinnedToCore(Task_Lora, "Task_Lora", 10240, NULL, 2, NULL, ARDUINO_RUNNING_CORE);

    if (wifi_flag == SUCCESS)
        xTaskCreatePinnedToCore(Task_Clock, "Task_Clock", 2048, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
}

void loop()
{
}

void Task_TFT(void *pvParameters) // This is a task.
{
    (void)pvParameters;

    panel.freq_write = 40000000;
    panel.freq_fill = 40000000;
    panel.freq_read = 16000000;

    panel.spi_cs = LCD_CS;
    panel.spi_dc = LCD_DC;
    panel.gpio_rst = LCD_RST;
    panel.gpio_bl = LCD_BL;

    tft.setPanel(&panel);
    tft.begin();
    tft.setRotation(SCRENN_ROTATION);
    tft.fillScreen(COLOR_BACKGROUND);

    touch_init();
    welcome_page();

    while (1) // A Task shall never return or exit.
    {
        // page_title(test_str + "  TestCount: " + my_count++);
        // vTaskDelay(500);

        // test_page();
        main_page();
    }
}

void Task_Clock(void *pvParameters)
{
    char time_str[20] = "";
    while (1)
    {
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("Failed to obtain time");
        }
        else
        {
            sprintf(time_str, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            global_time = (String)time_str;
            // Serial.println(global_time);
            time_fresh_flag = SUCCESS;
        }
        vTaskDelay(300);
    }
}

void Task_Lora(void *pvParameters)
{
    (void)pvParameters;

    vTaskDelay(10000);
    lora.init();

    while (1)
    {
        if (lora_task_index)
        {
            //Receive Lora
            String rec_str = "";
            rec_str = lora.receive();
            // rec_str = lora.receive_intr();
            // Serial.println(rec_str);
            if (!rec_str.equals(""))
            {
                switch (lora_task_index)
                {
                case 0:
                    break;
                case 1:
                    soil_reply_analyse(rec_str);
                    break;
                default:
                    break;
                }
            }
        }

        vTaskDelay(100);
    }
}

//UI function
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

//page

void welcome_page()
{
    // tft.setRotation(1);
    print_img(SD, "/logo.bmp", 480, 320);
    // tft.setRotation(SCRENN_ROTATION);
    delay(3000);
    tft.fillScreen(COLOR_BACKGROUND);
}

void test_page()
{
    Button b_a;
    Button b_b;

    int count_a = 0;
    int count_b = 0;

    b_a.set(10, 100, 150, 100, "a", ENABLE);
    b_a.setTextSize(2);

    b_b.set(200, 100, 150, 100, "b", ENABLE);
    b_b.setTextSize(2);

    int pos[2] = {0, 0};

    page_title("Test Page1");

    while (1)
    {
        drawButton(b_a);
        drawButton(b_b);

        int count = 0;
        while (1)
        {
            if (count < 5)
            {
                count++;
            }
            else
            {
                count = 0;
                tft.fillRect(20, 40, 200, 40, COLOR_BACKGROUND);
                tft.setCursor(20, 40);
                tft.setTextColor(COLOR_TEXT);
                tft.setTextSize(2);
                tft.println(test_str);
            }
            if (get_touch(pos))
            {
                int button_value = UNABLE;

                if ((button_value = b_a.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("a Button");
                    }
                    b_a.setText((String) "a" + count_a++);
                    break;
                }

                if ((button_value = b_b.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("b Button");
                    }
                    b_b.setText((String) "b" + count_b++);
                    break;
                }
            }
            vTaskDelay(100);
        }
    }
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

    lora_task_index = 1;

    //Read soil from file
    soil_num = read_nodes_id(SD, SOIL_FILE, soil_id_list, SOIL_NUM_MAX);

    for (int i = 0; i < soil_num; i++)
    {
        Serial.println(soil_id_list[i]);
    }

    if (soil_num > SOIL_NUM_MAX)
        soil_num = SOIL_NUM_MAX;

    //Read relay from file
    relay_num = read_nodes_id(SD, MOS_FILE, relay_id_list, RELAY_NUM_MAX);

    for (int i = 0; i < relay_num; i++)
    {
        Serial.println(relay_id_list[i]);
    }

    if (relay_num > RELAY_NUM_MAX)
        relay_num = RELAY_NUM_MAX;

    //page init
    page_title("ESP32 TFT LORA");

    //Set lora node and draw buttons
    for (int i = 0; i < soil_num; i++)
    {
        soil_list[i].setNode(soil_id_list[i], soil_id_list[i] / 10000);
        if (i < 4)
            b_soil[i].set(10, 60 + 50 * i, 140, 40, "NULL", UNABLE);
        else
            b_soil[i].set(170, 60 + 50 * (i - 4), 140, 40, "NULL", UNABLE);
        String show_txt = soil_list[i].getId_str() + " " + soil_list[i].getType_str();
        b_soil[i].setText(show_txt);
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

    // b_clock.set(10, 260, 270, 40, "WIFI UNLINK", UNABLE);
    b_clock.set(10, 260, 270, 40, global_time, UNABLE);

    b_clock.setTextSize(4);
    drawButton_clock(b_clock);

    vTaskDelay(2000);
    while (1)
    {
        //button check
        if (get_touch(pos))
        {
            int button_value = UNABLE;

            //Add Soil
            if ((button_value = b_add_soil.checkTouch(pos[0], pos[1])) != UNABLE)
            {
                if (DEBUG)
                {
                    Serial.println("b_add_soil Button");
                }

                if (add_soil_page() == SUCCESS)
                {
                    Serial.println("Add nodes success");
                }

                break;
            }

            //Add Relay
            if ((button_value = b_add_relay.checkTouch(pos[0], pos[1])) != UNABLE)
            {
                if (DEBUG)
                {
                    Serial.println("b_add_soil Button");
                }

                if (add_relay_page() == SUCCESS)
                {
                    Serial.println("Add nodes success");
                }

                break;
            }

            //Set page
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
                    mos_page(relay_list[i].getId_str());
                    //relay4_control_page(relay_list[i].getId_str());
                    return;
                    //break;
                }
            }
        }

        if (time_fresh_flag == SUCCESS)
        {
            time_fresh_flag = 0;
            b_clock.setText(global_time);
            drawButton_clock(b_clock);
        }

        if (page_fresh_flag == SUCCESS)
        {
            page_fresh_flag = FALSE;
            break;
        }

        vTaskDelay(100);
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
                    if (b_id.getValue() / 10000 == 6)
                    {
                        char node_c[20];
                        sprintf(node_c, "%d;\n", b_id.getValue());
                        appendFile(SD, MOS_FILE, node_c);

                        tft.setCursor(100, 160);
                        tft.println("Over and Back.");
                        delay(500);
                        return SUCCESS;
                    }
                    else
                    {
                        tft.setCursor(100, 160);
                        tft.println("MOS4 ID must be 06XXXX.");
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
                    writeFile(SD, MOS_FILE, "");

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

void mos_page(String Node_id)
{
    Serial.println("PAGE:lora_mos4_page");

    lora_task_index = 0;

    //Button init
    Button b_send(40, 250, 180, 50, "Send", UNABLE);
    Button b_back(240, 250, 180, 50, "Back", 2);

    //Four button
    Button b_mos_switch[4];
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            b_mos_switch[2 * i + j].set(40 + 200 * j, 120 + 60 * i, 160, 50, "NULL", UNABLE);
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
        page_title("LORA MOSFET4");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_send);
        drawButton(b_back);

        for (int i = 0; i < 4; i++)
            drawButton(b_mos_switch[i]);

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

                    if (cut_reply.indexOf("MOS4") != -1)
                    {
                        int status = mos4_reply_analyse(cut_reply);
                        for (int i = 0; i < 4; i++)
                        {
                            int temp_value = status % 10;
                            temp_str = (String) "MOS" + i + (temp_value != 0 ? ":ON" : ":OFF");
                            b_mos_switch[i].setText(temp_str);
                            b_mos_switch[i].setValue(temp_value);

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
                        message_param += b_mos_switch[i].getValue();
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
                    if ((button_value = b_mos_switch[i].checkTouch(pos[0], pos[1])) != UNABLE)
                    {
                        if (DEBUG)
                        {
                            Serial.println("Relay Button");
                        }

                        int temp = b_mos_switch[i].getValue();
                        if (temp != 0)
                            temp = 0;
                        else
                            temp = 8;
                        // temp = !temp;

                        b_mos_switch[i].setValue(temp);
                        temp_str = (String) "MOS" + i + (temp != 0 ? ":ON" : ":OFF");
                        b_mos_switch[i].setText(temp_str);

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

//init
int wifi_init()
{
    Serial.println("Prepare Link Wifi");
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

    configTime(28800, daylightOffset_sec, ntpServer);
    // configTime((const long)(timezone * 3600), daylightOffset_sec, ntpServer);
    Serial.println(F("Alread get npt time."));

    return SUCCESS;
}

//Lora
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
            sprintf(rec_cstr, "INDEX%d ID%d %s %s", log_index++, id, soil_data_list[i], global_time.c_str());
            Serial.println(rec_cstr);
            appendFile(SD, SOIL_LOG_FILE, rec_cstr);

            page_fresh_flag = SUCCESS;
        }
    }
}

int mos4_reply_analyse(String data)
{
    int status = 0;

    status = data.substring(5, 9).toInt();

    return status;
}

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