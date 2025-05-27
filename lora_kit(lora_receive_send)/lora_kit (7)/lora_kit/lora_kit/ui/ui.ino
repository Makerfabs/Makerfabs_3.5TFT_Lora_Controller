#include <lvgl.h>
#include <ui.h>
#include "FT6236.h" // Include for touch
#include <Arduino_GFX_Library.h>

#include "WiFi.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RadioLib.h>
#include <SPI.h>
#include <stdio.h>

#include "config.h"
#include "log_save.h"
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
// #include <freertos/event_groups.h>

EventGroupHandle_t eventGroup;

// NTP server address
const char* ntpServer = "pool.ntp.org";

// Create a UDP instance
WiFiUDP ntpUDP;
// Create an NTP client instance without setting a time zone offset to get UTC time
NTPClient timeClient(ntpUDP);


uint8_t add_wifi_flag = 0;
uint8_t time_update_flag = 0;
char read_time[10];
char soil_time[75];
uint8_t lora_mode = 0;
// extern char soil_name_list[8][7];
uint8_t lora_send_data[20];
uint8_t lora_send_len = 0;

uint8_t receive_data[20];

char *mos0_on_str = "MOS0: ON";
char *mos1_on_str = "MOS1: ON";
char *mos2_on_str = "MOS2: ON";
char *mos3_on_str = "MOS3: ON";

char mos_id0[7];
uint16_t mos0_cnts = 0;
char mos_id1[7];
uint16_t mos1_cnts = 0;

uint8_t mos0_control_flag = 0;
uint8_t mos1_control_flag = 0;

extern uint8_t mos0_send_flag;
extern uint8_t mos1_send_flag;

uint32_t mos0_one_seconde_cnts = 0;
uint32_t mos1_one_seconde_cnts = 0;

uint8_t mos0_first_send_flag = 0;
uint8_t mos1_first_send_flag = 0;

uint8_t mos0_resend_cnts = 0;
uint8_t mos1_resend_cnts = 0;

char soil_id_analyse[80];

extern uint8_t add_soil_flag;
extern uint8_t add_soil_button;
// char soil_id[40];

// disable interrupt when it's not needed
volatile bool recv_interrupt_flag = true;
// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// save transmission state between loops
int transmissionState = ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;
// 定义两个 SPI 总线
SPIClass spi1(SPI); // 默认的 SPI 总线
// SPIClass spi2(1);   // 第二个 SPI 总线（SPI1）

// SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, SPI, SPISettings());
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, spi1, SPISettings());

#define SOIL0_FILE "/soil0_id.txt"
#define SOIL1_FILE "/soil1_id.txt"
#define SOIL2_FILE "/soil2_id.txt"
#define SOIL3_FILE "/soil3_id.txt"
#define SOIL4_FILE "/soil4_id.txt"
#define SOIL5_FILE "/soil5_id.txt"
#define SOIL6_FILE "/soil6_id.txt"
#define SOIL7_FILE "/soil7_id.txt"

char soil_id0_writer_cnts = 0;
char soil_id1_writer_cnts = 0;
char soil_id2_writer_cnts = 0;
char soil_id3_writer_cnts = 0;
char soil_id4_writer_cnts = 0;
char soil_id5_writer_cnts = 0;
char soil_id6_writer_cnts = 0;
char soil_id7_writer_cnts = 0;

uint8_t lora_start_recevie_flag = 0;
uint8_t loat_start_recevie_cnts = 0;

// Screen dimensions - these should match your display
static const uint16_t screenWidth = 480; // Example, adjust if needed
static const uint16_t screenHeight = 320; // Example, adjust if needed

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10]; // LVGL buffer

#define I2C_SCL 39 // Define I2C SCL pin
#define I2C_SDA 38 // Define I2C SDA pin

Arduino_DataBus *bus = new Arduino_ESP32SPI(21 /* DC */, 15 /* CS */, 14 /* SCK */, 13 /* MOSI */, 12 /* MISO */);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, GFX_NOT_DEFINED /* RST */, 1 /* rotation */, false /* IPS */);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}

/* Read the touchpad */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    int touchX = 0, touchY = 0;

    if (!get_pos(&touchX, &touchY)) { // Assuming get_pos is defined by FT6236
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        /*Set the coordinates -  adjust these based on your screen rotation if needed*/
        data->point.x = touchY; // Example rotation adjustment
        data->point.y = 320-touchX;     // Example rotation adjustment
        // Serial.print(data->point.x);
        // Serial.print(",");
        // Serial.println(data->point.y);
    }
}

void setup() {
    Serial.begin(115200); /* prepare for possible serial debug */

    eventGroup = xEventGroupCreate(); // 创建事件组

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println(LVGL_Arduino);

    Wire.begin(I2C_SDA, I2C_SCL); // Initialize I2C

    // Initialize LCD (LGFX)
    gfx->begin();
    gfx->fillScreen(BLACK);
    
    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    pinMode(LORA_CS, OUTPUT);
    delay(100);

    spi1.begin(LORA_SCK, LORA_MISO, LORA_MOSI);
    lora_init();
    // SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    // // SPI.begin(42, 41, 2);
    // SPI_ON_SD;
    // spi2.begin(SD_SCK, SD_MISO, SD_MOSI);
    SD_init();
    Serial.println("Setup done");

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 40960, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(Lora_main, "Lora_main", 40960, NULL, 4, NULL, 1);
    xTaskCreatePinnedToCore(Task_wifi, "Task_wifi", 40960, NULL, 3, NULL, 1);
}

void loop() {
    // lv_timer_handler();
    // delay(10);
}

void Task_TFT(void *pvParameters) // screen refresh
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(10);
    }
}

void Lora_main(void *pvParameters) // Relay and alarm settings
{
    uint32_t ev_bits;
    while (1)
    {
        ev_bits = xEventGroupWaitBits(eventGroup,
            LORA_RECV_EV | LORA_SEND_EV | LORA_INIT_EV,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);
        if((ev_bits & LORA_RECV_EV) == LORA_RECV_EV)
        {
            Serial.println("(ev_bits & LORA_RECV_EV)");
            ev_bits &= ~(uint32_t)(LORA_RECV_EV); // 清空标志位
            receive_lora(); 
        }
        if((ev_bits & LORA_SEND_EV) == LORA_SEND_EV)
        {
            Serial.println("(ev_bits & LORA_SEND_EV)");
            ev_bits &= ~(uint32_t)(LORA_SEND_EV); // 清空标志位
            send_lora();
        }
        if((ev_bits & LORA_INIT_EV) == LORA_INIT_EV)
        {
            Serial.println("(ev_bits & LORA_INIT_EV)");
            ev_bits &= ~(uint32_t)(LORA_INIT_EV); // 清空标志位
            lora_init();
        }
        // lora_main();
        // vTaskDelay(100);
    }
}

 // Determine whether the alarm clock and the current time match, match the corresponding switch relays
void Task_wifi(void *pvParameters)
{
    while (1)
    {
        // lora_main();

        if(add_wifi_flag == 1)
        {
            radio.standby();
            radio.sleep();
            
            Serial.println("add_wifi_flag == 1");
            wifi_scan();
            add_wifi_flag = 0;
        }

        if(wifi_connect_flag == 1 && add_wifi_flag == 0)
        {
            Serial.println("add_wifi_flag == 1");
            wifi_connect();
            wifi_connect_flag = 0;
            start_receive();

        }

        if(time_update_flag == 1 && wifi_connect_flag == 0)
        {
            Serial.println("time_update_flag == 1");
            time_updata();          
        }

        if(mos0_send_flag == 1)
        {
            lv_label_set_text(ui_Label18, "Sending...");
            Serial.println("mos0_one_seconde_cnts");
            // Serial.println(mos0_one_seconde_cnts);
            if(mos0_one_seconde_cnts == 0 && mos0_first_send_flag == 0)
            {
                mos0_first_send_flag = 1;
                // Serial.println("11111111111");
                xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位   第一次发送的情况               
            }
            mos0_one_seconde_cnts++;
            if((mos0_one_seconde_cnts >= 6) && (mos0_send_flag == 1))
            {
                Serial.println("mos0_resend_cnts");
                Serial.println(mos0_resend_cnts);
                // lora_mode = Lora_Send;   //重发数据
                mos0_one_seconde_cnts = 0;
                mos0_resend_cnts++;
                if(mos0_resend_cnts <= 2)
                {
                    xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位
                }
                else
                {
                    lv_label_set_text(ui_Label18, "Send Error");
                    mos0_send_flag = 0;
                    mos0_one_seconde_cnts = 0;
                    mos0_resend_cnts = 0;
                    mos0_first_send_flag = 0;
                }
            }
        }
        else
        {
            mos0_one_seconde_cnts = 0;
            mos0_resend_cnts = 0;
            mos0_first_send_flag = 0;
        }

        if(mos1_send_flag == 1)
        {
            Serial.println("mos1_one_seconde_cnts");
            lv_label_set_text(ui_Label19, "Sending...");
            if(mos1_one_seconde_cnts == 0 && mos1_first_send_flag == 0)
            {
                xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位   第一次发送的情况               
            }
            mos1_one_seconde_cnts++;
            if((mos1_one_seconde_cnts >= 6) && (mos1_send_flag == 1))
            {
                Serial.println("mos1_resend_cnts");
                Serial.println(mos1_resend_cnts);
                // lora_mode = Lora_Send;   //重发数据
                mos1_one_seconde_cnts = 0;
                mos1_resend_cnts++;
                if(mos1_resend_cnts <= 2)
                {
                    xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位
                }
                else
                {
                    lv_label_set_text(ui_Label19, "Send Error");
                    mos1_send_flag = 0;
                    mos1_one_seconde_cnts = 0;
                    mos1_resend_cnts = 0;
                    mos1_first_send_flag = 0;
                }
            }
        }
        else
        {
            mos1_one_seconde_cnts = 0;
            mos1_resend_cnts = 0;
            mos1_first_send_flag = 0;
        }

        display_soil_id_history_data();
        static uint8_t test_cnts = 0;

        if(lora_start_recevie_flag == 1)
        {
            loat_start_recevie_cnts++;
            if(loat_start_recevie_cnts >= 5)
            {
                start_receive();
                loat_start_recevie_cnts = 0;
                test_cnts++;
                if(test_cnts >= 3)
                {
                    lora_init();                    
                }

            }
        }
        else
        {
            test_cnts = 0;
            loat_start_recevie_cnts = 0;
        }
        Serial.println("test_lines\n");
        vTaskDelay(1000);
    }
}

void wifi_scan()
{
    Serial.println("Scan start");
    char wifi_name[32];
    String wifi_string = "";
    // Set WiFi to station mode and disconnect from an AP if it was previously connected.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(1000);

    // WiFi.scanNetworks will return the number of networks found.
    int n = WiFi.scanNetworks();

    Serial.println("n");
    Serial.println(n);

    if (n == 0) 
    {
        Serial.println("no networks found");
        lv_dropdown_set_options(ui_Dropdown1, "no networks found...");
    } 
    else if(n < 0)
    {
        Serial.println("WiFi scan failed");
        lv_dropdown_set_options(ui_Dropdown1, "WiFi scan failed...");
        return;
    }
    else 
    {
        for (int i = 0; i < n; ++i) 
        {
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            if(WiFi.SSID(i).length() < 32)
            {
                memset(wifi_name, 0, sizeof(wifi_name));
                memcpy(wifi_name, WiFi.SSID(i).c_str(), WiFi.SSID(i).length());  
                wifi_name[WiFi.SSID(i).length()] = '\0';
                String name = wifi_name;
                wifi_string += name;
                wifi_string += "\n";  
                name = "";          
            }

            delay(10);
        }
        // for(int i = 0; i < n; i++)
        // {
        //     wifi_string += wifi_name[i];
        //     wifi_string += "\n";
        
        // }

    }

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();

    lv_dropdown_set_options(ui_Dropdown1, wifi_string.c_str());
    Serial.println("scan end");

}

void wifi_connect(void)
{
    uint8_t attempts_count = 0;
    char ssid[32];
    char password_name[32];
    lv_dropdown_get_selected_str(ui_Dropdown1, ssid, sizeof(ssid));
    const char *password = lv_textarea_get_text(ui_TextArea3);
    sprintf(password_name, "%s", password);

    lv_label_set_text(ui_Label59, "Connecting...");

    Serial.println("Connecting to WiFi");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);
    Serial.print("password_name: ");
    Serial.println(password_name);

    if(WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(ssid, password_name);        
    }

    while (WiFi.status() != WL_CONNECTED) 
    {
        attempts_count++;
        delay(500);
        Serial.print(".");
        if(attempts_count > 5)
        {
            //WiFi.disconnect(); // 断开 WiFi 连接
            Serial.println("\nFailed to connect to WiFi");
            lv_label_set_text(ui_Label59, "Error");
            lv_label_set_text(ui_Label63, "-- -- --");
            time_update_flag = 0;
            break;
        }
    }
    if(attempts_count != 0 && WiFi.status() != WL_CONNECTED)
    {
      attempts_count = 0;
      Serial.println("\n return ");
      return;
    }
    attempts_count = 0;
    Serial.println("\nWiFi connected");
    lv_label_set_text(ui_Label59, "Connected");

    // Start the NTP client
    if(WiFi.status() == WL_CONNECTED)
    {
        timeClient.begin();
    }
    else
    {
        Serial.println("\nWiFi unconnected  return \n");
        return;
    }
    
    while (!timeClient.update()) 
    {
        attempts_count++;
        delay(500);
        Serial.print(".");
        if(attempts_count > 5)
        {
            Serial.println("\nFailed to update NTP time");
            lv_label_set_text(ui_Label63, "-- -- --");
            break;
        }
    }

    if(attempts_count != 0 && (!timeClient.update()))
    {
        attempts_count = 0;
        Serial.println("\n time return ");
        return;
    }
    Serial.println("\nNTP time synced");

    time_update_flag = 1;

}

void time_updata(void)
{
    // Update NTP time
  timeClient.update();

  // Get the current timestamp
  unsigned long epochTime = timeClient.getEpochTime();
  Serial.print("UTC Timestamp: ");
  Serial.println(epochTime);

  // Convert unsigned long to time_t
  time_t timeinfo = static_cast<time_t>(epochTime);
  struct tm* tm_info = gmtime(&timeinfo);

  // Get year, month, day, hour, minute, and second
  Serial.print("UTC Time: ");
  Serial.print(tm_info->tm_year + 1900); // Year
  Serial.print("-");
  Serial.print(tm_info->tm_mon + 1); // Month
  Serial.print("-");
  Serial.print(tm_info->tm_mday); // Day
  Serial.print(" ");
  Serial.print(tm_info->tm_hour); // Hour
  Serial.print(":");
  Serial.print(tm_info->tm_min); // Minute
  Serial.print(":");
  Serial.println(tm_info->tm_sec); // Second
  sprintf(read_time, "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  sprintf(soil_time, "%d/%d/%d %02d:%02d:%02d", tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
  soil_time[20] = '\0';
  read_time[9] = '\0';
  lv_label_set_text(ui_Label63, read_time);

}

void lora_init(void)
{
    pinMode(LORA_CS, OUTPUT);
    delay(100);

    spi1.begin(LORA_SCK, LORA_MISO, LORA_MOSI);

    // initialize SX1276 with default settings
    Serial.print(F("[SX1276] Initializing ... "));

    int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
    // int state = radio.begin();
    if (state == ERR_NONE)
    {
        Serial.println(F("success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        // while (true);
    }

    // set the function that will be called
    // when new packet is received
    radio.setDio0Action(setFlag);

    // start listening for LoRa packets
    Serial.print(F("[SX1278] Starting to listen ... "));
    state = radio.startReceive();
    if (state == ERR_NONE) 
    {
        Serial.println(F("success!"));
    } 
    else 
    {
        Serial.print(F("radio.startReceive(), failed, code "));
        Serial.println(state);
        // while (true);
    }

}

void start_receive(void)
{
    int state = radio.startReceive();
    if (state == ERR_NONE) 
    {
        Serial.println(F("start_receive success!"));
        lora_start_recevie_flag = 0;
    } 
    else 
    {
        Serial.print(F("radio.startReceive(), failed, code "));
        Serial.println(state);
        lora_start_recevie_flag = 1;
        //while (true);
    }
}

void setFlag(void) {
    // check if the interrupt is enabled
    if(!recv_interrupt_flag) {
      return;
    }
  
    // // we got a packet, set the flag
    // receivedFlag = true;
    // if(mos0_send_flag == 0 && mos1_send_flag == 0)
    // {
        // receivedFlag = true;
        // xEventGroupSetBits(eventGroup, LORA_RECV_EV); // 设置接收事件位
    // }

    // // we sent a packet, set the flag
    // transmittedFlag = true;

    // if (transmissionState == ERR_NONE) {
    //   // packet was successfully sent
    //   Serial.println(F("transmission finished!"));

    //   // NOTE: when using interrupt-driven transmit method,
    //   //       it is not possible to automatically measure
    //   //       transmission data rate using getDataRate()

    // } 
    // else 
    // {
    //   Serial.print(F("failed, code "));
    //   Serial.println(transmissionState);

    // }

    // if(transmittedFlag == true)
    // {
    //     transmittedFlag == false;
        // if (transmissionState == ERR_NONE) {
        //     // packet was successfully sent
        //     Serial.println(F("transmission finished!"));

        //     // NOTE: when using interrupt-driven transmit method,
        //     //       it is not possible to automatically measure
        //     //       transmission data rate using getDataRate()

        // } 
        // else 
        // {
        //     Serial.print(F("failed, code "));
        //     Serial.println(transmissionState);
        // }
        // // put module back to listen mode
        // // Serial.print("change to receive!!!\n");
        // // radio.standby();
        // // radio.startReceive();
    // }
    // else
    // {
        receivedFlag = true;
        xEventGroupSetBits(eventGroup, LORA_RECV_EV); // 设置接收事件位
        Serial.println("send LORA_RECV_EV!!!");
    // }
}

void send_lora()
{
    recv_interrupt_flag = false;

    mos_send_data();

    for(int i = 0; i < lora_send_len; i++)
    {
      Serial.println(lora_send_data[i], HEX);
      Serial.println("--");
    }
    radio.transmit(lora_send_data, lora_send_len);

    // transmissionState = radio.startTransmit(lora_send_data, lora_send_len);

    // lora_mode = Lora_Recv;

    Serial.print("change to receive!!!\n");
    // radio.standby();
    // put module back to listen mode
    // radio.startReceive();
    start_receive();
    // transmittedFlag = true;

    memset(receive_data, 0, sizeof(receive_data));

    recv_interrupt_flag = true;
}

void receive_lora()
{
    if(receivedFlag) 
    {
        // disable the interrupt service routine while
        // processing the data
        recv_interrupt_flag = false;

        // reset flag
        receivedFlag = false;

        // print the received data
        // int state = radio.receive(receive_data,20);
        int state = radio.readData(receive_data, 20);

        if (state == ERR_NONE)
        {
            // packet was successfully received
            Serial.println(F("received success!"));

            // print the RSSI (Received Signal Strength Indicator)
            // of the last received packet
            Serial.print(F("[SX1276] RSSI:\t\t\t"));
            Serial.print(radio.getRSSI());
            Serial.println(F(" dBm"));

            // print the SNR (Signal-to-Noise Ratio)
            // of the last received packet
            Serial.print(F("[SX1276] SNR:\t\t\t"));
            Serial.print(radio.getSNR());
            Serial.println(F(" dB"));

            // print frequency error
            // of the last received packet
            Serial.print(F("[SX1276] Frequency error:\t"));
            Serial.print(radio.getFrequencyError());
            Serial.println(F(" Hz"));

            Serial.print(F("[SX1276] Data:\t\t\t"));
            for(int i = 0; i < 20; i++)
            {
            Serial.println(receive_data[i], HEX);
            }
            analyse_lora_data(receive_data);
        }
        else if (state == ERR_RX_TIMEOUT)
        {
            // timeout occurred while waiting for a packet
    //        Serial.println(F("timeout!"));
        }
        else if (state == ERR_CRC_MISMATCH)
        {
            // packet was received, but is malformed
            Serial.println(F("CRC error!"));
        }
        else
        {
            // some other error occurred
            Serial.print(F("failed, code "));
            Serial.println(state);
        }   

        recv_interrupt_flag = true;

    }


    // delay(1000);
}

void lora_main()
{
    switch(lora_mode)
    {
        case Lora_Init:
        {
            // lora_init();
            lora_mode = Lora_Recv;
            break;
        }
        case Lora_Send:
        {
            send_lora();
            break;
        }
        case Lora_Recv:
        {
            receive_lora();
            break;
        }
        default:
            break;
        // case Lora_Sleep:
        // {
        //     lora_sleep();
        // }
        // break;
    }
}

void analyse_lora_data(uint8_t *data)
{
    uint16_t soil_adc = 0;
    uint16_t air_temp = 0;
    uint16_t air_humi = 0;
    float air_temp_dis = 0.0;
    float air_humi_dis = 0.0;
    uint8_t lab_i = 0xff;
    memset(soil_id_analyse, 0, sizeof(soil_id_analyse));
    if(data[8] == 0x01)
    {
        Serial.print("soil data type");
        uint16_t analyse_crc = CRC16_CCITT(data, data[16]);
        Serial.print("analyse_crc:");
        Serial.println(analyse_crc);
        uint16_t crc = (data[17] << 8) | data[18];
        Serial.print("crc:");
        Serial.println(crc);
        if(analyse_crc == crc)
        {
            Serial.print("CRC check pass");
            memcpy(soil_id_analyse, data, 6);
            air_humi = (data[9] << 8) | data[10];
            // Serial.print("air_humi:");
            // Serial.println(air_humi);
            air_humi_dis = air_humi/10.0;
            air_temp = (data[11] << 8) | data[12];
            // Serial.print("air_temp:");
            // Serial.println(air_temp);
            air_temp_dis = air_temp/10.0;
            soil_adc = (data[13] << 8) | data[14];
            for(int i = 0; i < 8; i++)
            {
                if(memcmp(soil_id_analyse, soil_name_list[i], 6) == 0)
                {
                    lab_i = i;
                    break;
                }
            }
            switch(lab_i)
            {
                case 0:
                    Serial.print("soil1");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label6, soil_id_analyse);
                    char node_c[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL0_FILE, node_c);
                    soil_id0_writer_cnts++;
                    break;
                case 1: 
                    Serial.print("soil2");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label2, soil_id_analyse);
                    char node_c1[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c1, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL1_FILE, node_c1);
                    soil_id1_writer_cnts++;
                    break;
                case 2: 
                    Serial.print("soil3");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label4, soil_id_analyse);
                    char node_c2[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c2, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL2_FILE, node_c2);
                    soil_id2_writer_cnts++;
                    break;
                case 3: 
                    Serial.print("soil4");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label8, soil_id_analyse);
                    char node_c3[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c3, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL3_FILE, node_c3);
                    soil_id3_writer_cnts++;
                    break;
                case 4: 
                    Serial.print("soil5");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label10, soil_id_analyse);
                    char node_c4[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c4, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL4_FILE, node_c4);
                    soil_id4_writer_cnts++;
                    break;
                case 5: 
                    Serial.print("soil6");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label12, soil_id_analyse);
                    char node_c5[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c5, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL5_FILE, node_c5);
                    soil_id5_writer_cnts++;
                    break;
                case 6: 
                    Serial.print("soil7");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label14, soil_id_analyse);
                    char node_c6[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c6, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL6_FILE, node_c6);
                    soil_id6_writer_cnts++;
                    break;
                case 7: 
                    Serial.print("soil8");
                    sprintf(soil_id_analyse, "ID%.7s\nSoil AD: %d\nAir T/H: %.1f/%.1f%%\n%.20s", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
				    lv_label_set_text(ui_Label17, soil_id_analyse);
                    char node_c7[70];
                    // sprintf(node_c, "ID%s Soil AD: %d Air T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, soil_time);
                    sprintf(node_c7, "%s AD:%d T/H: %.1f/%.1f%% %s\n", soil_name_list[lab_i], soil_adc, air_temp_dis, air_humi_dis, read_time);
                    appendFile(SD, SOIL7_FILE, node_c7);
                    soil_id7_writer_cnts++;
                    break;
                default:
                    Serial.print("unknown soil");
                    break;
            }

            lora_send_len = 0;
            memset(lora_send_data, 0, sizeof(lora_send_data));
            memcpy(lora_send_data, data, 8);
            lora_send_len += 8;
            lora_send_data[lora_send_len++] = 0x01;
            lora_send_data[lora_send_len++] = 0x01;
            uint8_t len = lora_send_len;
            lora_send_data[lora_send_len++] = len;
            uint16_t crc_send = CRC16_CCITT(lora_send_data, len);
            lora_send_data[lora_send_len++] = crc_send >> 8;
            lora_send_data[lora_send_len++] = crc_send & 0xff;

            // lora_mode = Lora_Send;
            // radio.standby();
            // radio.sleep();
            // radio.standby();
            xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位  发送土壤接受成功响应
            // transmissionState = radio.startTransmit(lora_send_data, lora_send_len);
            // radio.startReceive();
        }
        else
        {
            // put module back to listen mode
            // radio.startReceive();
            start_receive();
        }
        memset(receive_data, 0, sizeof(receive_data));  
    }
    else if(data[8] == 0x02)
    {
        Serial.print("MOS data type");
        uint16_t mos_analyse_crc = CRC16_CCITT(data, data[10]);
        Serial.print("mos_analyse_crc:");
        Serial.println(mos_analyse_crc, HEX);
        uint16_t mos_crc = (data[11] << 8) | data[12];
        Serial.print("mos_crc:");
        Serial.println(mos_crc, HEX);
        if(mos_analyse_crc == mos_crc)
        {
            Serial.print("mos CRC check pass\n");
            if(mos0_send_flag == 1)
            {
                if((memcmp(data, mos_id0, 6) == 0) && mos0_control_flag == data[9])
                {
                    Serial.print("mos_id0 control ok!\n");
                    lv_label_set_text(ui_Label18, "Send OK");
                    mos0_send_flag = 0;
                    // lora_mode = Lora_Recv;

                    // put module back to listen mode
                    // radio.startReceive();
                    start_receive();
                    memset(receive_data, 0, sizeof(receive_data)); 
                }
                else
                {
                    Serial.print("mos_id0 control error! send again\n");
                    // lora_mode = Lora_Send;   //重发数据
                    xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位
                }                
            }
            else
            {
                // put module back to listen mode
                // radio.startReceive();
                start_receive();
            }

            if(mos1_send_flag == 1)
            {
                if((memcmp(data, mos_id1, 6) == 0) && mos1_control_flag == data[9])
                {
                    Serial.print("mos_id1 control ok!\n");
                    lv_label_set_text(ui_Label19, "Send OK");
                    mos1_send_flag = 0;
                    // lora_mode = Lora_Recv;

                    // put module back to listen mode
                    // radio.startReceive();
                    start_receive();
                    memset(receive_data, 0, sizeof(receive_data)); 
                }
                else
                {
                    Serial.print("mos_id1 control error! send again\n");
                    // lora_mode = Lora_Send;   //重发数据
                    xEventGroupSetBits(eventGroup, LORA_SEND_EV); // 设置发送事件位
                }                
            }
            else
            {
                // put module back to listen mode
                // radio.startReceive();
                start_receive();
            }

        }
        else
        {
            // put module back to listen mode
            // radio.startReceive();
            start_receive();
        }
    }
    else
    {
        // lora_mode = Lora_Recv;

        // put module back to listen mode
        // radio.startReceive();
        start_receive();
        memset(receive_data, 0, sizeof(receive_data));        
        Serial.print("unknown data type");
        // xEventGroupSetBits(eventGroup, LORA_INIT_EV); // 设置发送事件位
    }


}

void mos_send_data(void)
{
    Serial.print("mos_send_data\n");
    if(mos0_send_flag == 1)
    {
        Serial.print("mos0_send_flag == 1\n");
        mos0_control_flag = 0;
        char *mos0 = lv_label_get_text(ui_Label45);
        char *mos1 = lv_label_get_text(ui_Label47);
        char *mos2 = lv_label_get_text(ui_Label46);
        char *mos3 = lv_label_get_text(ui_Label48);
        // Serial.print(mos0);
        // Serial.print("\n");
        // Serial.print(mos0_on_str);
        // Serial.print("\n");
        // Serial.print(mos1);
        // Serial.print("\n");
        // Serial.print(mos2);
        // Serial.print("\n");
        // Serial.print(mos3);
        // Serial.print("\n");
        if(strcmp(mos0, mos0_on_str) == 0)
        {
            // Serial.print("00\n");
            mos0_control_flag |= 0x01;
        }
        if(strcmp(mos1, mos1_on_str) == 0)
        {
            // Serial.print("11\n");
            mos0_control_flag |= 0x02;
        }
        if(strcmp(mos2, mos2_on_str) == 0)
        {
            // Serial.print("22\n");
            mos0_control_flag |= 0x04;
        }
        if(strcmp(mos3, mos3_on_str) == 0)
        {
            // Serial.print("33\n");
            mos0_control_flag |= 0x08;
        }

        lora_send_len = 0;    
        memset(lora_send_data, 0, sizeof(lora_send_data));

        memcpy(lora_send_data, mos_id0, 6);
        lora_send_len += 6;
        mos0_cnts++;
        lora_send_data[lora_send_len++] = mos0_cnts >> 8;
        lora_send_data[lora_send_len++] = mos0_cnts & 0xff;
        lora_send_data[lora_send_len++] = 0x02;
        lora_send_data[lora_send_len++] = mos0_control_flag;
        uint8_t len = lora_send_len;
        lora_send_data[lora_send_len++] = len;
        uint16_t crc_mos0_send = CRC16_CCITT(lora_send_data, len);
        lora_send_data[lora_send_len++] = crc_mos0_send >> 8;
        lora_send_data[lora_send_len++] = crc_mos0_send & 0xff;
    }
    else if(mos1_send_flag == 1)
    {
        Serial.print("mos1_send_flag == 1\n");
        mos1_control_flag = 0;
        char * mos0 = lv_label_get_text(ui_Label53);
        char * mos1 = lv_label_get_text(ui_Label55);
        char * mos2 = lv_label_get_text(ui_Label54);
        char * mos3 = lv_label_get_text(ui_Label56);
        // Serial.print(mos0);
        // Serial.print("\n");
        // Serial.print(mos1);
        // Serial.print("\n");
        // Serial.print(mos2);
        // Serial.print("\n");
        // Serial.print(mos3);
        // Serial.print("\n");
        if(strcmp(mos0, mos0_on_str) == 0)
        {
            mos1_control_flag |= 0x01;
        }
        if(strcmp(mos1, mos1_on_str) == 0)
        {
            mos1_control_flag |= 0x02;
        }
        if(strcmp(mos2, mos2_on_str) == 0)
        {
            mos1_control_flag |= 0x04;
        }
        if(strcmp(mos3, mos3_on_str) == 0)
        {
            mos1_control_flag |= 0x08;
        }

        lora_send_len = 0;    
        memset(lora_send_data, 0, sizeof(lora_send_data));

        memcpy(lora_send_data, mos_id1, 6);
        lora_send_len += 6;
        mos1_cnts++;
        lora_send_data[lora_send_len++] = mos1_cnts >> 8;
        lora_send_data[lora_send_len++] = mos1_cnts & 0xff;
        lora_send_data[lora_send_len++] = 0x02;
        lora_send_data[lora_send_len++] = mos1_control_flag;
        uint8_t len = lora_send_len;
        lora_send_data[lora_send_len++] = len;
        uint16_t crc_mos1_send = CRC16_CCITT(lora_send_data, len);
        lora_send_data[lora_send_len++] = crc_mos1_send >> 8;
        lora_send_data[lora_send_len++] = crc_mos1_send & 0xff;
    }
}

void display_soil_id_history_data(void)
{
    char node_c[700];
    if(ui_label6_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label6_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL0_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label2_flag == 1) 
    {
        lv_label_set_text(ui_Label16, "");
        ui_label2_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL1_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label4_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label4_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL2_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label8_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label8_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL3_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label10_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label10_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL4_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label12_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label12_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL5_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label14_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label14_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL6_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }
    else if(ui_label17_flag == 1)
    {
        lv_label_set_text(ui_Label16, "");
        ui_label17_flag = 0;
        memset(node_c, 0, sizeof(node_c));
        read_soil_file(SD, SOIL7_FILE, node_c);
        lv_label_set_text(ui_Label16, node_c);
    }

}


/****************************Info********************************************** 
 * Name:    CRC-16/CCITT        x16+x12+x5+1 
 * Width:	16
 * Poly:    0x1021 
 * Init:    0x0000 
 * Refin:   True 
 * Refout:  True 
 * Xorout:  0x0000 
 * Alias:   CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT 
 *****************************************************************************/ 
unsigned short CRC16_CCITT(unsigned char *data, unsigned int datalen)
{
	unsigned short wCRCin = 0x0000;
	unsigned short wCPoly = 0x1021;
	unsigned char wChar = 0;
	
	while (datalen--) 	
	{
		wChar = *(data++);
		InvertUint8(&wChar,&wChar);
		wCRCin ^= (wChar << 8);
		for(int i = 0;i < 8;i++)
		{
			if(wCRCin & 0x8000)
				wCRCin = (wCRCin << 1) ^ wCPoly;
			else
				wCRCin = wCRCin << 1;
		}
	}
	InvertUint16(&wCRCin,&wCRCin);
	return (wCRCin);

}


/****************************Info********************************************** 
 * Name:    InvertUint8 
 * Note: 	把字节颠倒过来，如0x12变成0x48
			0x12: 0001 0010
			0x48: 0100 1000
 *****************************************************************************/
void InvertUint8(unsigned char *dBuf,unsigned char *srcBuf)
{
	int i;
	unsigned char tmp[4]={0};
 
	for(i=0;i< 8;i++)
	{
		if(srcBuf[0]& (1 << i))
		tmp[0]|=1<<(7-i);
	}
	dBuf[0] = tmp[0];
	
}
void InvertUint16(unsigned short *dBuf,unsigned short *srcBuf)
{
	int i;
	unsigned short tmp[4]={0};
 
	for(i=0;i< 16;i++)
	{
		if(srcBuf[0]& (1 << i))
		tmp[0]|=1<<(15 - i);
	}
	dBuf[0] = tmp[0];

}
