#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "I2C_AHT10.h"

//#define NODENAME "LORA_POWER_1"
// String node_id = String("ID") + "010000";
uint8_t node_id[7] = "011538";

//Set sleep time, when value is 1 almost sleep 20s,when value is 450, almost 1 hour.
#define SLEEP_CYCLE 21//450

//Lora set
//Set Lora frequency
// #define FREQUENCY 434.0
// #define FREQUENCY 868.0

// #define FREQUENCY 915.0
// #define BANDWIDTH 125.0
// #define SPREADING_FACTOR 9
// #define CODING_RATE 7
// #define OUTPUT_POWER 10
// #define PREAMBLE_LEN 8
// #define GAIN 0
#define FREQUENCY 915//434.0
#define BANDWIDTH 125.0     //   203.125-1625.0   125khz  250khz  500khz    带宽越大，数据传输速率越高，但接收灵敏度降低，通信距离缩短
#define SPREADING_FACTOR 12   //   7-12  扩频因子越大，通信距离越远，但数据传输速率越低
#define CODING_RATE 7    //    5-8   4/5、4/6、4/7、4/8   编码率越高，抗干扰能力越强，但传输时间增加，数据速率降低
#define OUTPUT_POWER 13   //  -18--13        10 dBm 到 22 dBm   发射功率越高，通信距离越远，但能耗增加
#define PREAMBLE_LEN 8   //前导码长度
#define GAIN 0

//328p
#define DIO0 2
#define DIO1 6

#define LORA_RST 4
#define LORA_CS 10

#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 13

//pin set
#define VOLTAGE_PIN A3
#define PWM_OUT_PIN 9
#define SENSOR_POWER_PIN 5
#define ADC_PIN A2

#define DEBUG_OUT_ENABLE 1

#define Lora_Init  0
#define Lora_Send  1
#define Lora_Recv  2
#define Lora_Sleep 3

uint8_t lora_mode = 0;

SX1276 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1);
AHT10 humiditySensor;

uint8_t send_data[20];

bool readSensorStatus = false;
uint16_t sensorValue = 0; // variable to store the value coming from the sensor
int batValue = 0;    // the voltage of battery
int count = 0;
int ADC_O_1;           // ADC Output First 8 bits
int ADC_O_2;           // ADC Output Next 2 bits
uint16_t packetnum = 0; // packet counter, we increment per xmission
float temperature = 0.0;
float humidity = 0.0;
uint16_t temp_value = 0;
uint16_t humi_value = 0;
uint8_t transmit_bat = 0;
int rx_timeout_cnts = 0;

bool AHT_init()
{
    bool ret = false;
    Wire.begin();
    if (humiditySensor.begin() == false)
    {
#if DEBUG_OUT_ENABLE
        Serial.println("AHT10 not detected. Please check wiring. Freezing.");
#endif
    }

    if (humiditySensor.available() == true)
    {
        temperature = humiditySensor.getTemperature();
        humidity = humiditySensor.getHumidity();
        ret = true;
    }
    if (isnan(humidity) || isnan(temperature))
    {
#if DEBUG_OUT_ENABLE
        Serial.println(F("Failed to read from AHT sensor!"));
#endif
    }
    temp_value = (uint16_t)(temperature * 10);
    humi_value = (uint16_t)(humidity * 10);

    return ret;
}
void Lora_init()
{
    int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
    if (state == ERR_NONE)
    {
#if DEBUG_OUT_ENABLE
        Serial.println(F("success!"));
#endif
    }
    else
    {
#if DEBUG_OUT_ENABLE
        Serial.print(F("failed, code "));
        Serial.println(state);
#endif
        // while (true)
        //     ;
    }
}
void setup()
{
#if DEBUG_OUT_ENABLE
    Serial.begin(115200);
    Serial.println("Soil start.");
#endif
//     delay(100);

//     // set up Timer 1
//     pinMode(PWM_OUT_PIN, OUTPUT);

//     TCCR1A = bit(COM1A0);            // toggle OC1A on Compare Match
//     TCCR1B = bit(WGM12) | bit(CS10); // CTC, scale to clock
//     OCR1A = 1;

//     pinMode(LORA_RST, OUTPUT);
//     digitalWrite(LORA_RST, HIGH);
//     delay(100);

//     pinMode(SENSOR_POWER_PIN, OUTPUT);
//     digitalWrite(SENSOR_POWER_PIN, HIGH); //Sensor power on
//     delay(100);

//     Lora_init();

//     Wire.begin();
//     if (humiditySensor.begin() == false)
//     {

// #if DEBUG_OUT_ENABLE
//         Serial.println("AHT10 not detected. Please check wiring. Freezing.");
// #endif
//     }
// #if DEBUG_OUT_ENABLE
//     else
//         Serial.println("AHT10 acknowledged.");
// #endif

//     do_some_work();
// //setup over
// #if DEBUG_OUT_ENABLE
//     Serial.println("[Set]Sleep Mode Set");
// #endif
//     low_power_set();

    // lora_main();
}

void loop()
{
    wdt_disable();

    lora_main();

    if (count > SLEEP_CYCLE) //(7+1) x 8S  450
    {
        lora_mode = Lora_Init;
// #if DEBUG_OUT_ENABLE
//         //code start
//         Serial.println("Code start>>");

//         do_some_work();
//         all_pins_low();

// #if DEBUG_OUT_ENABLE
//         //code end
//         Serial.println("Code end<<");#endif

// #endif
        //count init
        count = 0;
    }

    // low_power_set();
}

ISR(WDT_vect)
{
#if DEBUG_OUT_ENABLE
    Serial.print("[Watch dog]");
    Serial.println(count);
#endif
    delay(100);
    count++;
    //wdt_reset();
    wdt_disable(); // disable watchdog
}

//Set low power mode and into sleep
void low_power_set()
{
    all_pins_low();
    delay(10);
    // disable ADC
    ADCSRA = 0;

    sleep_enable();
    watchdog_init();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    delay(10);
    noInterrupts();
    sleep_enable();

    // turn off brown-out enable in software
    MCUCR = bit(BODS) | bit(BODSE);
    MCUCR = bit(BODS);
    interrupts();

    sleep_cpu();
    sleep_disable();
}

//Enable watch dog
void watchdog_init()
{
    // clear various "reset" flags
    MCUSR = 0;
    // allow changes, disable reset
    WDTCSR = bit(WDCE) | bit(WDE);
    WDTCSR = bit(WDIE) | bit(WDP3) | bit(WDP0); // set WDIE, and 8 seconds delay
    wdt_reset();                                // pat the dog
}

void do_some_work()
{
    Serial.println("-----lora_init----\r\n");
    pinMode(SENSOR_POWER_PIN, OUTPUT);
    digitalWrite(SENSOR_POWER_PIN, HIGH); // Sensor/RF95 power on
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, HIGH);
    delay(5);
    pinMode(PWM_OUT_PIN, OUTPUT);    //digitalWrite(PWM_OUT_PIN, LOW);
    TCCR1A = bit(COM1A0);            // toggle OC1A on Compare Match
    TCCR1B = bit(WGM12) | bit(CS10); // CTC, scale to clock
    OCR1A = 1;                       // compare A register value (5000 * clock speed / 1024).When OCR1A == 1, PWM is 2MHz

    Lora_init();
    delay(50);

    //ADC2  AVCC as reference voltage
    ADMUX = _BV(REFS0) | _BV(MUX1);

    //ADC2  internal 1.1V as ADC reference voltage
    //ADMUX = _BV(REFS1) |_BV(REFS0) | _BV(MUX1);

    // 8  分频
    ADCSRA = _BV(ADEN) | _BV(ADPS1) | _BV(ADPS0);
    delay(50);
    for (int i = 0; i < 3; i++)
    {
        //start ADC conversion
        ADCSRA |= (1 << ADSC);

        delay(10);

        if ((ADCSRA & 0x40) == 0)
        {
            ADC_O_1 = ADCL;
            ADC_O_2 = ADCH;

            sensorValue = (ADC_O_2 << 8) + ADC_O_1;
            ADCSRA |= 0x40;
#if DEBUG_OUT_ENABLE
            Serial.print("ADC:");
            Serial.println(sensorValue);
#endif

            if (readSensorStatus == false)
                readSensorStatus = AHT_init();
        }
        ADCSRA |= (1 << ADIF); //reset as required
        delay(50);
    }

    //ADC3  internal 1.1V as ADC reference voltage
    ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX1) | _BV(MUX0);

    delay(50);
    for (int i = 0; i < 3; i++)
    {
        //start ADC conversion
        ADCSRA |= (1 << ADSC);

        delay(10);
        
        if ((ADCSRA & 0x40) == 0)
        {
            ADC_O_1 = ADCL;
            ADC_O_2 = ADCH;

            batValue = (ADC_O_2 << 8) + ADC_O_1;
            ADCSRA |= 0x40;
#if DEBUG_OUT_ENABLE
            Serial.print("BAT:");
            Serial.println(batValue);
            float bat = (float)batValue * 3.3;
            bat = bat / 1024.0;
            transmit_bat = bat * 10;
            Serial.print(bat);
            Serial.print("V");
#endif
        }
        ADCSRA |= (1 << ADIF); //reset as required
        delay(50);
    }
    // send_lora();
    // delay(1000);
    // radio.sleep();

    // packetnum++;
    // readSensorStatus = false;
    // digitalWrite(SENSOR_POWER_PIN, LOW); // Sensor/RF95 power off
    // delay(100);
}

void all_pins_low()
{
    pinMode(PWM_OUT_PIN, INPUT);
    pinMode(A4, INPUT_PULLUP);
    pinMode(A5, INPUT_PULLUP);

    delay(50);
}

void send_lora()
{
    Serial.println("-----send_lora----\r\n");
    int i = 0;
    memset(send_data, 0, sizeof(send_data));
    memcpy(send_data, node_id, 6);
    i += 6;
    send_data[i++] = packetnum >> 8;
    send_data[i++] = packetnum & 0xff;

    send_data[i++] = 0x01;  //身份标志位

    send_data[i++] = humi_value >> 8;
    send_data[i++] = humi_value & 0xff;

    send_data[i++] = temp_value >> 8;
    send_data[i++] = temp_value & 0xff;

    send_data[i++] = sensorValue >> 8;
    send_data[i++] = sensorValue & 0xff; 

    send_data[i++] = transmit_bat;

    uint8_t len = i;
    send_data[i++] = len;

    uint16_t crc = CRC16_CCITT(send_data, len);
    send_data[i++] = crc >> 8;
    send_data[i++] = crc & 0xff;



    for(int j = 0; j < i; j++)
    {
      Serial.println(send_data[j], HEX);
      // Serial.println("--");
    }
    radio.transmit(send_data, i);

    // String message = "INEDX:" + (String)packetnum + " H:" + (String)humidity + " T:" + (String)temperature + " ADC:" + (String)sensorValue + " BAT:" + (String)batValue;
    // String back_str = node_id + " REPLY : SOIL " + message;

// #if DEBUG_OUT_ENABLE
//     Serial.println(back_str);
// #endif

//     radio.transmit(back_str);
}

void receive_lora()
{
    Serial.println("-----receive_lora----\r\n");
    while(1)
    {
        uint8_t receive_data[20];
        int state = radio.receive(receive_data,20);

        if (state == ERR_NONE)
        {
            // packet was successfully received
            Serial.println(F("success!"));

            // print the data of the packet
            // Serial.print(F("[SX1278] Data:\t\t\t"));
            // Serial.println(str);

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

            if (command_explain(receive_data))
            {
                Serial.println("rx ok!!!  go to sleep");

                radio.sleep();

                packetnum++;
                readSensorStatus = false;
                digitalWrite(SENSOR_POWER_PIN, LOW); // Sensor/RF95 power off
                delay(100);

                rx_timeout_cnts = 0;

                lora_mode = Lora_Sleep;
                break;
            }
        }
        else if (state == ERR_RX_TIMEOUT)
        {
            // timeout occurred while waiting for a packet
            Serial.println(F("timeout!"));
            rx_timeout_cnts++;
            if(rx_timeout_cnts > 2)
            {
                rx_timeout_cnts = 0;
                lora_mode = Lora_Init;
                break;
                
            }
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
  
}
int command_explain(uint8_t * data)
{
    //string spilt
    Serial.print("command_explain");

    uint16_t command_crc = CRC16_CCITT(data, data[10]);
    Serial.print("command_crc:");
    Serial.println(command_crc);
    uint16_t crc = (data[11] << 8) | data[12];
    Serial.print("crc:");
    Serial.println(crc);
    if(command_crc == crc)
    {
        if ((memcmp(data, node_id, 6) == 0) && data[9] == 0x01)
        {
            return 1;
        }    
    }

    return 0;
}

void lora_sleep()
{
    Serial.println("lora_sleep\r\n");

    low_power_set();
}

void lora_main()
{
    switch(lora_mode)
    {
        case Lora_Init:
        {
            do_some_work();       //lora的初始化以及传感器的数据采集
            lora_mode = Lora_Send;
            break;
        }
        case Lora_Send:
        {
            send_lora();
            lora_mode = Lora_Recv;
            break;
        }
        case Lora_Recv:
        {
            receive_lora();
            break;
        }
        case Lora_Sleep:
        {
            lora_sleep();
        }
        break;
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