/*
ESP32 Borad Version 1.0.6
RadioLib Version 4.6.0
LovyanGFX V0.4.6


*/
#include <SPI.h>
#include <LovyanGFX.hpp>
#include <RadioLib.h>
#include <WiFi.h>

#include "makerfabs_pin.h"
#include "Lora.h"

#define RECEIVE_LEN 20

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

SPIClass SPI_Lora = SPIClass(HSPI);
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, SPI_Lora, SPISettings());
Lora lora(&radio);

void setup()
{
    Serial.begin(115200);

    pinMode(LCD_CS, OUTPUT);
    pinMode(SD_CS, OUTPUT);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SPI_Lora.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    set_tft();
    tft.begin();
    tft.setRotation(SCRENN_ROTATION);
    // Lora init must after HSPI init
    lora.init();

    page_title("WiFi Connecting...");
}

void loop()
{

    receive_page();
}

//********************************page**************************

void receive_page()
{
    long runtime = millis();
    int pos[2] = {0, 0};

    String rec_list[RECEIVE_LEN];
    int rec_index = 0;
    int rec_num = 0;

    // Page refresh loop
    while (1)
    {
        // page init

        page_title("RECEIVE PAGE");

        tft.drawRect(9, 44, 462, 252, TFT_WHITE);

        // Prevent accidental touch.
        delay(1000);

        // Working loop
        while (1)
        {
            String rec_str = "";
            rec_str = lora.receive();

            // delay(5000);
            // rec_str = "ID:P2,SLEEEP:10,H:1.01,T:2.86,PH:3.03,N:4,P:5,K:6";
            if (!rec_str.equals(""))
            {

                rec_list[rec_index] = rec_str;
                int temp_index = rec_index;

                if (rec_index == RECEIVE_LEN - 1)
                    rec_index = 0;
                else
                    rec_index++;

                if (rec_num < RECEIVE_LEN)
                    rec_num++;

                tft.fillRect(10, 45, 460, 200, TFT_BLACK);
                tft.setTextColor(TFT_WHITE);
                tft.setTextSize(1);

                for (int i = 0; i < rec_num; i++)
                {
                    tft.setCursor(12, 50 + i * 10);
                    tft.println(rec_list[temp_index--]);
                    if (temp_index < 0)
                        temp_index = rec_num - 1;
                }
            }
        }
    }
    return;
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

void page_title(String title)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 10);
    tft.setTextSize(4);
    tft.println(title);
}
