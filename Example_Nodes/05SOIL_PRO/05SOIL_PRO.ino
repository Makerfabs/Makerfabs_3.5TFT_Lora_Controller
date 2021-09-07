#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define Display_power 19
#define RS485_power 18

//esp32
#define DIO0 25
#define DIO1 27

#define LORA_RST 33
#define LORA_CS 32

#define SPI_MOSI 13
#define SPI_MISO 12
#define SPI_SCK 14

#define FREQUENCY 434.0
#define BANDWIDTH 125.0
#define SPREADING_FACTOR 9
#define CODING_RATE 7
#define OUTPUT_POWER 10
#define PREAMBLE_LEN 8
#define GAIN 0

#define MYSerialRX 23
#define MYSerialTX 22

String node_id = String("ID") + "050000";
int id_number_length = 6;
String debug_id = "IDXDEBUG";

HardwareSerial MySerial(1);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

SX1278 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1, SPI, SPISettings());

unsigned char testcode[8] = {0X01, 0X03, 0X00, 0X00, 0X00, 0X04, 0X44, 0X09};
unsigned char responsecode[13] = {};
int moisture;
int tem;
int ph;
float moisture_value; //百分比
float tem_value;      //wendu
float ph_value;       //0-14
int P_value;
int N_value;
int K_value;
float P_float_value;
float N_float_value;
float K_float_value;

long runtime = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(4800);

  MySerial.begin(4800, SERIAL_8N1, 23, 22);

  pinMode(Display_power, OUTPUT);
  pinMode(RS485_power, OUTPUT);
  delay(50);
  digitalWrite(Display_power, HIGH);
  digitalWrite(RS485_power, HIGH);
  delay(1000);

  Serial.println(" Test Begin!");

  Wire.begin();
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println("SSD1306 found");
  logo_show();

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  //SPI.begin();

  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
  //int state = radio.begin();
  if (state == ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }
}

void loop()
{
  if (millis() - runtime > 5000)
  {
    refresh_data();
    value_show(moisture_value, tem_value, ph_value);

    runtime = millis();
  }
  lora_task();
}

void logo_show()
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); // Draw 2X-scale text
  display.setCursor(10, 0);
  display.println(F("Makerfabs"));
  display.setTextSize(1);
  display.setCursor(10, 16);
  display.println(F("RS485-LoRa"));
  display.display(); // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x01);
  delay(4000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
}

int caculatevalue(int x, int y)
{
  int t = 0;
  t = x * 256;
  t = t + y;
  return t;
}

void value_show(float h, float t, float ph_f)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); // Draw 2X-scale text
  display.setCursor(2, 0);
  display.print(F("T:"));
  display.print(t, 1);
  display.print(F("C"));

  display.setCursor(66, 0);
  display.print(F("H:"));
  display.print(h, 1);
  display.print(F("%"));

  display.setCursor(2, 16);
  display.print(F("PH:"));
  display.print(ph_f, 1);

  display.display(); // Show initial text
  delay(100);
}
/*
 * unsigned char testcode[236] = {0XFD,0XFD,0X30,0X3,...
 * Serial.write(testcode,236);
 * 
 */

void refresh_data()
{
  MySerial.write(testcode, 8);
  int i = 0;
  while (MySerial.available() > 0 && i < 13)
  {
    responsecode[i] = MySerial.read();
    i++;
    yield();
  }
  for (int j = 0; j < 13; j++)
  {
    Serial.print((int)responsecode[j]);
    Serial.print("  ");
  }
  Serial.print("\n");

  moisture = caculatevalue((int)responsecode[3], (int)responsecode[4]);
  moisture_value = moisture * 0.1;
  tem = caculatevalue((int)responsecode[5], (int)responsecode[6]);
  tem_value = tem * 0.1;
  ph = caculatevalue((int)responsecode[9], (int)responsecode[10]);
  ph_value = ph * 0.1;

  Serial.println(moisture);
  Serial.println(moisture_value);
  Serial.println(tem_value);
  Serial.println(ph_value);
}

void lora_task()
{
  String str;
  int state = radio.receive(str);

  if (state == ERR_NONE)
  {
    // packet was successfully received
    Serial.println(F("success!"));

    // print the data of the packet
    Serial.print(F("[SX1278] Data:\t\t\t"));
    Serial.println(str);

    // print the RSSI (Received Signal Strength Indicator)
    // of the last received packet
    // Serial.print(F("[SX1278] RSSI:\t\t\t"));
    // Serial.print(radio.getRSSI());
    // Serial.println(F(" dBm"));

    // print the SNR (Signal-to-Noise Ratio)
    // of the last received packet
    Serial.print(F("[SX1278] SNR:\t\t\t"));
    Serial.print(radio.getSNR());
    Serial.println(F(" dB"));

    // print frequency error
    // of the last received packet
    Serial.print(F("[SX1278] Frequency error:\t"));
    Serial.print(radio.getFrequencyError());
    Serial.println(F(" Hz"));

    if (command_explain(str))
    {
      String back_str = node_id + " REPLY : SOIL-PRO " + create_reply_data();
      radio.transmit(back_str);
    }
  }
  else if (state == ERR_RX_TIMEOUT)
  {
    // timeout occurred while waiting for a packet
    //Serial.println(F("timeout!"));
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
}

int command_explain(String str)
{
  //string spilt
  String txt = str;
  if (txt.startsWith(node_id) || txt.startsWith(debug_id))
  {
    //int node_id = (txt.substring(2, 5)).toInt();
    long node_act = txt.substring(id_number_length + 5, id_number_length + 8).toInt();
    int node_param = txt.substring(id_number_length + 14, id_number_length + 20).toInt();

    Serial.println("ACT:  " + String(node_act));
    Serial.println("PARAM: " + String(node_param));

    switch (node_act)
    {
    case 0:
      Serial.println("ALL CLOSE");
      break;

    case 114:
      Serial.println("CHECK STATUS");
      break;

    default:
      Serial.println("UNKNOWN ACT!");
      return 0;
    }
    return 1;
  }

  return 0;
}

String create_reply_data()
{
  String reply = "";
  reply = reply + "HUM " + moisture + " TEM " + tem + " PH " + ph;
  return reply;
}