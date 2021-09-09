#include <RadioLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

String node_id = String("ID") + "040000";
int id_number_length = 6;
String debug_id = "IDXDEBUG";

//328p
#define DIO0 2
#define DIO1 6
//#define DIO2 7
//#define DIO5 8

#define LORA_RST 4
#define LORA_CS 10

#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 13

#define FREQUENCY 434.0
#define BANDWIDTH 125.0
#define SPREADING_FACTOR 9
#define CODING_RATE 7
#define OUTPUT_POWER 10
#define PREAMBLE_LEN 8
#define GAIN 0

SX1278 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1, SPI, SPISettings());

#define TdsSensorPin A0
#define VREF 3.3          // analog reference voltage(Volt) of the ADC
#define SCOUNT 30         // sum of sample point
int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;

bool LEDState = false;
#define LEDPin 5
#define TDSpower A2

#define one_wire_bus A1

OneWire oneWire(one_wire_bus);
DallasTemperature sensors(&oneWire);

void setup()
{
    Serial.begin(115200);
    pinMode(TdsSensorPin, INPUT);
    pinMode(TDSpower, OUTPUT);
    digitalWrite(TDSpower,HIGH);

    delay(1000);
    Serial.println("Test Begin:");

    SPI.begin();

    //int state = radio.begin();
    int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
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

    sensors.begin();
}

static unsigned long analogSampleTimepoint = 0;
static unsigned long printTimepoint = 0;

void loop()
{

    //TDS Checking
    if (millis() - analogSampleTimepoint > 400U) //every 40 milliseconds,read the analog value from the ADC
    {
        analogSampleTimepoint = millis();
        analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); //read the analog value and store into the buffer
        analogBufferIndex++;
        if (analogBufferIndex == SCOUNT)
            analogBufferIndex = 0;
    }

    if (millis() - printTimepoint > 8000U)
    {
        //Get temperature
        sensors.requestTemperatures();
        Serial.print("T:");
        temperature = sensors.getTempCByIndex(0);
        Serial.println(temperature);

        printTimepoint = millis();
        for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
            analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
        averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;                                                                                                  // read the analog value more stable by the median filtering algorithm, and convert to voltage value
        float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);                                                                                                               //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
        float compensationVolatge = averageVoltage / compensationCoefficient;                                                                                                            //temperature compensation
        tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
        Serial.print("voltage:");
        Serial.print(averageVoltage, 2);
        Serial.print("V   ");
        Serial.print("TDS Value:");
        Serial.print(tdsValue, 0);
        Serial.println("ppm");
    }

    lora_task();
}

void lora_task()
{
    String str;
    int state_r = radio.receive(str);

    if (state_r == ERR_NONE)
    {
        // packet was successfully received
        Serial.println(F("success!"));

        // print the data of the packet
        Serial.print(F("[SX1278] Data:\t\t\t"));
        Serial.println(str);

        // print the RSSI (Received Signal Strength Indicator)
        // of the last received packet
        Serial.print(F("[SX1278] RSSI:\t\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

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
            String back_str = node_id + " REPLY : TDS " + create_reply_data();
            radio.transmit(back_str);
        }
    }
    else if (state_r == ERR_RX_TIMEOUT)
    {
        // timeout occurred while waiting for a packet
        Serial.println(F("timeout!"));
    }
    else if (state_r == ERR_CRC_MISMATCH)
    {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state_r);
    }
}

int getMedianNum(int bArray[], int iFilterLen)
{
    int bTab[iFilterLen];
    for (byte i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++)
    {
        for (i = 0; i < iFilterLen - j - 1; i++)
        {
            if (bTab[i] > bTab[i + 1])
            {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if ((iFilterLen & 1) > 0)
        bTemp = bTab[(iFilterLen - 1) / 2];
    else
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    return bTemp;
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
    reply = reply + "TEM " + (int)(temperature * 10) + " TDS " + (int)tdsValue;
    return reply;
}
