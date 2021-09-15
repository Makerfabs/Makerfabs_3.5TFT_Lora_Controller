//#include <Wire.h>
#include <SPI.h>
#include <LovyanGFX.hpp>
#include <RadioLib.h>

#include "makerfabs_pin.h"
#include "log_save.h"
#include "touch.h"
#include "Button.h"
#include "Lora.h"
#include "LoraNode.h"

#define SUCCESS 1
#define FALSE 0
#define ASK_CYCLE 15000
#define RECEIVE_LEN 15

//#define DEBUG_ID "IDXDEBUG"
#define DEBUG_ID "ID000004"

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
SX1278 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1, SPI_Lora, SPISettings());
Lora lora(&radio);

//
LoraNode node_list[5];
int lora_num = 0;

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

    //Touch(I2C) init
    touch_init();

    //Lora init must after HSPI init
    lora.init();

    writeFile(SD, "/lora_log.txt", "Lora_log");

    welcome_page();
}

void loop()
{
    //page_test();
    //int n = 100;
    //num_pad(n);
    //ac_dimmer_page(DEBUG_ID);
    main_page();
}

//********************************page**************************
void welcome_page()
{
    tft.setRotation(1);
    print_img(SD, "/logo.bmp", 480, 320);
    tft.setRotation(SCRENN_ROTATION);
    delay(3000);
}
void main_page()
{
    Serial.println("PAGE:main_page");

    //Button init
    // Button b_add(380, 60, 89, 80, "Add", 0);
    // b_add.setText2("Node");
    // Button b_control(380, 160, 89, 80, "Con", 0);
    // b_control.setText2("trol");

    Button b_add(380, 60, 89, 60, "ADD", 0);
    Button b_del(380, 130, 89, 60, "DEL", 0);
    Button b_rec(380, 200, 89, 60, "REC", 0);

    //Button init
    Button b_node[5];
    for (int i = 0; i < 5; i++)
    {
        b_node[i].set(20, 60 + 50 * i, 300, 40, "NULL", UNABLE);
    }

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("ESP32 TFT LORA");

        for (int i = 0; i < 5; i++)
        {
            drawButton(b_node[i]);
            if (b_node[i].getValue() == ENABLE)
                icon_switch(node_list[i].getType(), i);
            //print_img_small(SD, "/s00.bmp", 330, 64, 32, 32);
        }

        drawButton(b_add);
        drawButton(b_del);
        drawButton(b_rec);

        // drawButton_2L(b_add);
        // drawButton_2L(b_del);

        //tft.fillRect(310, 64, 32, 32, TFT_RED);
        //print_img(SD, "/s00.bmp", 32, 32);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > ASK_CYCLE)
            {
                //Check lora message and refresh button str
                for (int i = 0; i < 5; i++)
                {
                    if (b_node[i].getValue() == ENABLE)
                    {
                        //send a lora message to ac dimmer
                        String lora_str = lora.command_format(node_list[i].getId_str(), 114, 0);
                        lora.send(lora_str);
                        String rec_str = "";
                        String cut_reply = "";

                        //if not reply, show error
                        rec_str = lora.receive();

                        if (rec_str.equals(""))
                        {
                            rec_str = "NULL";
                            cut_reply = "NO REPLY";
                        }
                        else if (!rec_str.startsWith(node_list[i].getId_str()))
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

                        //Analsy reply

                        String show_txt = node_list[i].getId_str() + " " + node_list[i].getType_str();
                        b_node[i].setText(show_txt);
                        b_node[i].setText2(cut_reply);
                        b_node[i].setTextSize(1);
                        drawButton_2L(b_node[i]);
                    }
                }
                runtime = millis();
            }

            //button check

            if (get_touch(pos))
            {
                int button_value = UNABLE;

                if ((button_value = b_add.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Add Button");
                    }

                    if (lora_num == 5)
                    {
                        page_alert("The number of Nodes is full");
                        break;
                    }

                    if (add_page() == SUCCESS)
                    {
                        b_node[lora_num - 1].setValue(ENABLE);
                    }
                    break;
                }

                if ((button_value = b_del.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("DEL Button");
                    }

                    if (del_page() == SUCCESS)
                    {
                        //Init all nodes
                        for (int i = 0; i < 5; i++)
                        {
                            b_node[i].set(20, 60 + 50 * i, 300, 40, "NULL", UNABLE);
                        }
                    }
                    break;
                }

                if ((button_value = b_rec.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("REC Button");
                    }

                    receive_page();
                    break;
                }

                int refresh_flag = 0;
                for (int i = 0; i < 5; i++)
                {
                    if ((button_value = b_node[i].checkTouch(pos[0], pos[1])) != UNABLE)
                    {
                        if (DEBUG)
                        {
                            Serial.printf("Pos is :%d,%d\n", pos[0], pos[1]);
                            Serial.printf("Value is :%d\n", button_value);
                            Serial.printf("Text is :");
                            Serial.println(b_node[i].getText());
                        }
                        page_swtich(node_list[i].getType(), i);
                        runtime = millis();
                        refresh_flag = 1;
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
int add_page()
{
    Serial.println("PAGE:add_page");

    Button b_send(40, 220, 180, 80, "Add", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);
    Button b_id(40, 60, 300, 50, "ID000000", 0);
    Button b_type(40, 120, 300, 50, "DIMMER", -1);

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("ADD LORA NODE");

        drawButton(b_send);
        drawButton(b_back);
        drawButton(b_id);
        drawButton(b_type);

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
                if ((button_value = b_send.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Add Lora Node");
                    }

                    //Add a lora node
                    node_list[lora_num].setNode(b_id.getValue(), b_id.getValue() / 10000);
                    lora_num++;

                    page_alert("Add....");
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

                    int type_num = temp / 10000;
                    b_type.setText(get_type_str(type_num));
                    //drawButton(b_id);

                    if (DEBUG)
                    {
                        Serial.print("Button value:");
                        Serial.println(temp);
                        Serial.print("Button text:");
                        Serial.println(id_cstr);
                    }
                    break;
                }

                //Button:Type
                //Now Don't Use
                if ((button_value = b_type.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }

                    int temp = num_pad(b_type.getValue());
                    if (temp > 99)
                        temp = 99;
                    b_type.setValue(temp);
                    char type_cstr[20];
                    sprintf(type_cstr, "TYPE%02d", temp);
                    b_type.setText((String)type_cstr);
                    drawButton(b_type);

                    if (DEBUG)
                    {
                        Serial.print("Button value:");
                        Serial.println(temp);
                        Serial.print("Button text:");
                        Serial.println(type_cstr);
                    }
                    break;
                }
            }
        }
    }
}
int del_page()
{
    Serial.println("PAGE:del_page");

    Button b_clean(40, 220, 180, 80, "Clean", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("DELETE PAGE");

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
                    for (int i = 0; i < 5; i++)
                    {
                        node_list[i].clean();
                    }
                    lora_num = 0;

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

void receive_page()
{
    Serial.println("PAGE:del_page");

    Button b_back(270, 260, 150, 50, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    String rec_list[RECEIVE_LEN];
    int rec_index = 0;
    int rec_num = 0;

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("RECEIVE PAGE");

        drawButton(b_back);
        tft.drawRect(9, 44, 462, 202, TFT_WHITE);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            String rec_str = "";
            rec_str = lora.receive();
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
            //button check
            if (get_touch(pos))
            {

                int button_value = UNABLE;
                //Button:Back
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
    return;
}

void icon_switch(int type, int num)
{
    int y_pos = 64 + 50 * num;
    switch (type)
    {
    case 0:
        print_img_small(SD, "/s00.bmp", 330, y_pos, 32, 32);
        break;
    case 1:
        //print_img_small(SD, "/s01.bmp", 330, y_pos, 32, 32);
        break;
    case 2:
        print_img_small(SD, "/s02.bmp", 330, y_pos, 32, 32);
        break;
    case 3:
        print_img_small(SD, "/s03.bmp", 330, y_pos, 32, 32);
        break;
    case 4:
        print_img_small(SD, "/s04.bmp", 330, y_pos, 32, 32);
        break;
    case 5:
        print_img_small(SD, "/s05.bmp", 330, y_pos, 32, 32);
        break;
    default:
        break;
    }
}
void page_swtich(int type, int node_num)
{
    switch (type)
    {
    case 0:
        ac_dimmer_page(node_list[node_num].getId_str());
        break;
    case 1:
        soil_page(node_list[node_num].getId_str());
        break;
    case 2:
        relay_page(node_list[node_num].getId_str());
        break;
    case 3:
        relay4_page(node_list[node_num].getId_str());
        break;
    case 4:
        tds_page(node_list[node_num].getId_str());
        break;
    case 5:
        soil_pro_page(node_list[node_num].getId_str());
        break;
    case 6:
        null_type_page();
        break;
    default:
        null_type_page();
        break;
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

//TYPE00    DIMMER
void ac_dimmer_page(String Node_id)
{

    Serial.println("PAGE:ac_dimmer_page");

    String ac_dimmer_id = Node_id;

    //Button init
    Button b_control(40, 220, 180, 80, "Control", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("AC DIMMER  ");
        tft.setCursor(20, 50);
        tft.println(ac_dimmer_id);

        drawButton(b_control);
        drawButton(b_back);

        print_img_small(SD, "/l00.bmp", 320, 40, 128, 128);
        //tft.fillRect(270, 10, 128, 128, TFT_RED);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > ASK_CYCLE)
            {
                String cut_reply = typical_lora_check_request(Node_id);
                //show dimmer status

                tft.fillRect(20, 100, 300, 50, TFT_BLACK);
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
                    ac_dimmer_control_page(Node_id);
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
void ac_dimmer_control_page(String Node_id)
{

    Serial.println("PAGE:ac_dimmer_control_page");

    Button b_send(40, 220, 180, 80, "Send", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);
    Button b_param(40, 80, 300, 80, "Dimmer:0", 0);

    long runtime = millis();
    int pos[2] = {0, 0};
    String temp_str = "";

    //Page refresh loop
    while (1)
    {
        //page init
        page_title("AC DIMMER CONTROL");

        drawButton(b_send);
        drawButton(b_back);
        drawButton(b_param);

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
                    String lora_str = lora.command_format(Node_id, 2, b_param.getValue());
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
                //Button:Param
                if ((button_value = b_param.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Back Button");
                    }

                    int temp = num_pad(b_param.getValue());
                    if (temp > 255)
                        temp = 255;
                    b_param.setValue(temp);
                    temp_str = (String) "Dimmer:" + temp;
                    b_param.setText(temp_str);
                    drawButton(b_param);

                    if (DEBUG)
                    {
                        Serial.print("Button value:");
                        Serial.println(temp);
                        Serial.print("Button text:");
                        Serial.println(temp_str);
                    }
                    break;
                }
            }
        }
    }
}

//TYPE01    SOIL
void soil_page(String Node_id)
{

    Serial.println("PAGE:soil_page");

    //Button init
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("LORA SOIL");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_back);
        print_img_small(SD, "/l00.bmp", 320, 40, 128, 128);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > ASK_CYCLE)
            {
                String cut_reply = typical_lora_check_request(Node_id);
                //show dimmer status

                tft.fillRect(20, 100, 300, 50, TFT_BLACK);
                tft.setCursor(20, 100);
                tft.setTextSize(2);
                tft.println(cut_reply);

                runtime = millis();
            }

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

//TYPE02    RELAY
void relay_page(String Node_id)
{

    Serial.println("PAGE:lora_relay_page");

    //Button init
    Button b_control(40, 220, 180, 80, "Control", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("LORA RELAY");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_control);
        drawButton(b_back);

        print_img_small(SD, "/l02.bmp", 320, 40, 128, 128);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > ASK_CYCLE)
            {
                String cut_reply = typical_lora_check_request(Node_id);
                //show relay status

                tft.fillRect(20, 100, 300, 50, TFT_BLACK);
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
                    relay_control_page(Node_id);
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
void relay_control_page(String Node_id)
{
    Serial.println("PAGE:relay_control_page");

    Button b_send(40, 220, 180, 80, "Send", 1);
    Button b_back(240, 220, 180, 80, "Back", 2);
    Button b_relay0(40, 80, 160, 60, "Relay0:OFF", 0);

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
        drawButton(b_relay0);

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
                    String lora_str = lora.command_format(Node_id, 2, b_relay0.getValue());
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
                //Button:Param
                if ((button_value = b_relay0.checkTouch(pos[0], pos[1])) != UNABLE)
                {
                    if (DEBUG)
                    {
                        Serial.println("Param Button");
                    }

                    int temp = b_relay0.getValue();
                    temp = !temp;

                    b_relay0.setValue(temp);
                    temp_str = (String) "Relay0:" + (temp != 0 ? "ON" : "OFF");
                    b_relay0.setText(temp_str);

                    if (DEBUG)
                    {
                        Serial.print("Button value:");
                        Serial.println(temp);
                        Serial.print("Button text:");
                        Serial.println(temp_str);
                    }
                    break;
                }
            }
        }
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
        print_img_small(SD, "/l03.bmp", 320, 40, 128, 128);

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
                tft.fillRect(20, 100, 300, 50, TFT_BLACK);
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

    Button b_relay_list[4];
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            String relay_name = (String) "Relay" + (2 * i + j) + ":OFF";
            b_relay_list[2 * i + j].set(40 + 200 * j, 80 + 70 * i, 160, 60, relay_name, 0);
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
            drawButton(b_relay_list[i]);

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
                        message_param += b_relay_list[i].getValue();
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
                for (int i = 0; i < 5; i++)
                {
                    if ((button_value = b_relay_list[i].checkTouch(pos[0], pos[1])) != UNABLE)
                    {
                        if (DEBUG)
                        {
                            Serial.println("Relay Button");
                        }

                        int temp = b_relay_list[i].getValue();
                        temp = !temp;

                        b_relay_list[i].setValue(temp);
                        temp_str = (String) "Relay" + i + (temp != 0 ? ":ON" : ":OFF");
                        b_relay_list[i].setText(temp_str);

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

//TYPE04    TDS
void tds_page(String Node_id)
{

    Serial.println("PAGE:tds_page");

    //Button init
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("LORA TDS");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_back);
        print_img_small(SD, "/l04.bmp", 320, 40, 128, 128);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > 5000)
            {
                String cut_reply = typical_lora_check_request(Node_id);
                //show dimmer status

                if (!cut_reply.equals("NO REPLY"))
                {
                    int temperature = cut_reply.substring(cut_reply.indexOf("TEM") + 4, cut_reply.indexOf("TDV")).toInt();
                    int tds_value = cut_reply.substring(cut_reply.indexOf("TDV") + 4).toInt();

                    tft.fillRect(20, 100, 300, 100, TFT_BLACK);
                    tft.setTextSize(2);
                    tft.setCursor(20, 100);
                    tft.println((String) "Tds Value:" + tds_value + " ppm");
                    tft.setCursor(20, 120);
                    tft.println((String) "Temperature:" + temperature / 10.0 + " C");
                }
                else
                {
                    tft.fillRect(20, 100, 300, 100, TFT_BLACK);
                    tft.setCursor(20, 100);
                    tft.setTextSize(2);
                    tft.println(cut_reply);
                }

                runtime = millis();
            }

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

//TYPE05    SOIL-PRO
void soil_pro_page(String Node_id)
{

    Serial.println("PAGE:soil_pro_page");

    //Button init
    Button b_back(240, 220, 180, 80, "Back", 2);

    long runtime = millis();
    int pos[2] = {0, 0};

    //Page fresh loop
    while (1)
    {
        //page init
        page_title("LORA SOIL PRO");
        tft.setCursor(20, 50);
        tft.println(Node_id);

        drawButton(b_back);
        print_img_small(SD, "/l05.bmp", 320, 40, 128, 128);

        //Prevent accidental touch.
        delay(1000);

        //Working loop
        while (1)
        {
            //Every 5s
            if (millis() - runtime > 5000)
            {
                String cut_reply = typical_lora_check_request(Node_id);

                if (!cut_reply.equals("NO REPLY"))
                {
                    int humi = cut_reply.substring(cut_reply.indexOf("HUM") + 4, cut_reply.indexOf("TEM")).toInt();
                    int temperature = cut_reply.substring(cut_reply.indexOf("TEM") + 4, cut_reply.indexOf("PH")).toInt();
                    int ph = cut_reply.substring(cut_reply.indexOf("PH") + 3).toInt();

                    tft.fillRect(20, 100, 300, 100, TFT_BLACK);
                    tft.setTextSize(2);
                    tft.setCursor(20, 100);
                    tft.println((String) "Humidty:" + humi / 10.0 + " % ");
                    tft.setCursor(20, 120);
                    tft.println((String) "Temperature:" + temperature / 10.0 + " C");
                    tft.setCursor(20, 140);
                    tft.println((String) "PH:" + ph / 10.0);
                }
                else
                {
                    tft.fillRect(20, 100, 300, 100, TFT_BLACK);
                    tft.setCursor(20, 100);
                    tft.setTextSize(2);
                    tft.println(cut_reply);
                }

                runtime = millis();
            }

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

void page_title(String title)
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(20, 10);
    tft.setTextSize(4);
    tft.println(title);
}

void page_alert(String msg)
{
    tft.fillRect(80, 80, 320, 160, TFT_WHITE);
    tft.setCursor(100, 120);
    tft.setTextColor(TFT_BLACK);
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

    tft.fillScreen(TFT_BLACK);
    tft.drawRect(20, 20, 440, 80, TFT_WHITE);
    tft.setCursor(40, 40);
    tft.setTextColor(TFT_WHITE);
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

                        tft.fillRect(40, 40, 300, 50, TFT_BLACK);
                        tft.setCursor(40, 40);
                        tft.setTextSize(4);
                        tft.setTextColor(TFT_WHITE);
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
    String text;
    int textSize;

    b.getFoDraw(&b_x, &b_y, &b_w, &b_h, &text, &textSize);

    tft.fillRect(b_x, b_y, b_w, b_h, TFT_BLACK);
    tft.drawRect(b_x, b_y, b_w, b_h, TFT_WHITE);
    tft.setCursor(b_x + 20, b_y + 20);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(textSize);
    tft.println(text);
}

void drawButton_2L(Button b)
{
    int b_x;
    int b_y;
    int b_w;
    int b_h;
    String text;
    String text2;
    int textSize;

    b.getFoDraw(&b_x, &b_y, &b_w, &b_h, &text, &textSize);
    text2 = b.getText2();

    tft.fillRect(b_x, b_y, b_w, b_h, TFT_BLACK);
    tft.drawRect(b_x, b_y, b_w, b_h, TFT_WHITE);
    tft.setCursor(b_x + 20, b_y + textSize * 10);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(textSize);
    tft.println(text);
    tft.setCursor(b_x + 20, b_y + 2 * textSize * 10);
    tft.println(text2);
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
        tft.pushImage(0, row, X, 1, (lgfx::rgb888_t *)RGB);
    }

    f.close();
    return 0;
}

//Display image from file
//BMP is 32 bit...
int print_img_small(fs::FS &fs, String filename, int x, int y, int w, int h)
{
    File f = fs.open(filename, "r");
    if (!f)
    {
        Serial.println("Failed to open file for reading");
        f.close();
        return 0;
    }

    f.seek(54);
    int X = w;
    int Y = h;
    uint8_t RGB[3 * X];
    uint8_t RGBA[4 * X];
    for (int row = 0; row < Y; row++)
    {
        f.seek(54 + 4 * X * row);
        f.read(RGBA, 4 * X);

        for (int i = 0; i < X; i++)
        {
            RGB[3 * i] = RGBA[4 * X - 4 * i - 4];
            RGB[3 * i + 1] = RGBA[4 * X - 4 * i - 3];
            RGB[3 * i + 2] = RGBA[4 * X - 4 * i - 2];
        }
        tft.pushImage(x, y + row, X, 1, (lgfx::rgb888_t *)RGB);
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