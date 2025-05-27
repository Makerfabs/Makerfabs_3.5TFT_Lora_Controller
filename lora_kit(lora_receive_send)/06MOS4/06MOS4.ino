
/*

-----------NOT READY-----------

Author:         Vincent
Create date:    2022/3/23
Version:        1.0

Command example:

  ACT = 0 Close
    ID001ACT000PARAM000000#

  ACT = 1 All Open
    ID001ACT001PARAM000000#

  ACT = 2 PWM 
    PARAM = 0000-7777 MOS Status
    ID001ACT002PARAM001010

*/

// include the library
#include <RadioLib.h>
// #include <SoftPWM.h>

//UNO pwm 3, 5, 6, 9, 10, 11

#define DIO0 2
#define DIO1 A1
#define DIO2 7
#define DIO5 8

#define LORA_RST 4
#define LORA_CS 10

#define SPI_MOSI 11
#define SPI_MISO 12
#define SPI_SCK 13

#define MOS_RATIO 100

//Mosfet
int mos_status[4] = {0, 0, 0, 0}; //0 close 8 full open
int mos_pin[4] = {5, 6, 9, 3};
int mos_value[9] = {0, 32, 64, 96, 128, 160, 192, 224, 255}; //0% ~ 100%

// String type_name = "MOS4";

// String node_id = String("ID") + "060000";
// int id_number_length = 6;
// String debug_id = "IDXDEBUG";

uint8_t mos_id[7] = "060192";

uint8_t lora_send_data[20];
uint8_t lora_send_len = 0;

uint8_t receive_data[20];

uint8_t mos_control_flag = 0;

// Begin method:
// Carrier frequency: 434.0 MHz (for SX1276/77/78/79 and RFM96/98) or 915.0 MHz (for SX1272/73 and RFM95/97)
// Bandwidth: 125.0 kHz (dual-sideband)
// Spreading factor: 9
// Coding rate: 4/7
// Sync word: SX127X_SYNC_WORD (0x12)
// Output power: 10 dBm
// Preamble length: 8 symbols
// Gain: 0 (automatic gain control enabled)
// Other:
// Over-current protection: 60 mA
// Inaccessible:
// LoRa header mode: explicit
// Frequency hopping: disabled

#define FREQUENCY 915//434.0
#define BANDWIDTH 125.0     //   203.125-1625.0   125khz  250khz  500khz    带宽越大，数据传输速率越高，但接收灵敏度降低，通信距离缩短
#define SPREADING_FACTOR 12   //   7-12  扩频因子越大，通信距离越远，但数据传输速率越低
#define CODING_RATE 7    //    5-8   4/5、4/6、4/7、4/8   编码率越高，抗干扰能力越强，但传输时间增加，数据速率降低
#define OUTPUT_POWER 13   //  -18--13        10 dBm 到 22 dBm   发射功率越高，通信距离越远，但能耗增加
#define PREAMBLE_LEN 8   //前导码长度
#define GAIN 0

SX1276 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1);
//SX1278 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1, SPI, SPISettings());

void setup()
{
    Serial.begin(115200);

    mos_control(0);

    // initialize SX1278 with default settings
    Serial.print(F("[SX1278] Initializing ... "));

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
}

void loop()
{
    //Serial.print(F("[SX1278] Waiting for incoming transmission ... "));

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


void analyse_lora_data(uint8_t *data)
{
    if(data[8] == 0x02)
    {
        Serial.print("MOS data type");
        uint16_t mos_analyse_crc = CRC16_CCITT(data, data[10]);
        Serial.print("mos_analyse_crc:");
        Serial.println(mos_analyse_crc, HEX);
        uint16_t mos_crc = (data[11] << 8) | data[12];
        Serial.print("mos_crc:");
        Serial.println(mos_crc, HEX);
        if(mos_crc == mos_analyse_crc)
        {
            if(memcmp(mos_id, data, 6) == 0)
            {
                Serial.println("This is my MOS data");
                mos_control_flag = data[9];
                mos_control(mos_control_flag);

                lora_send_len = 0;
                memset(lora_send_data, 0, sizeof(lora_send_data));
                memcpy(lora_send_data, data, 8);
                lora_send_len += 8;
                lora_send_data[lora_send_len++] = 0x02;
                lora_send_data[lora_send_len++] = mos_control_flag;
                uint8_t len = lora_send_len;
                lora_send_data[lora_send_len++] = len;
                uint16_t crc_send = CRC16_CCITT(lora_send_data, len);
                lora_send_data[lora_send_len++] = crc_send >> 8;
                lora_send_data[lora_send_len++] = crc_send & 0xff;

                for(int i = 0; i < lora_send_len; i++)
                {
                Serial.println(lora_send_data[i], HEX);
                // Serial.println("--");
                }
                radio.transmit(lora_send_data, lora_send_len);

                memset(receive_data, 0, sizeof(receive_data));                
            }
            else
            {
                Serial.println("This is not my MOS data");
            }


        }
    }
}

// int command_explain(String str)
// {
//     //string spilt
//     String txt = str;
//     if (txt.startsWith(node_id) || txt.startsWith(debug_id))
//     {
//         //int node_id = (txt.substring(2, 5)).toInt();

//         //IDXDEBUG ACT 001 PARAM 000000
//         long node_act = txt.substring(id_number_length + 5, id_number_length + 8).toInt();
//         int node_param = txt.substring(id_number_length + 14, id_number_length + 20).toInt();

//         Serial.println("ACT:  " + String(node_act));
//         Serial.println("PARAM: " + String(node_param));

//         switch (node_act)
//         {
//         case 0:
//             Serial.println("ALL CLOSE");
//             mos_control(0);
//             break;

//         case 1:
//             Serial.println("ALL OPEN");
//             mos_control(8888);
//             break;

//         case 2:
//             Serial.println("MOS Control");
//             mos_control(node_param);
//             break;

//         case 114:
//             Serial.println("CHECK STATUS");
//             break;

//         default:
//             Serial.println("UNKNOWN ACT!");
//             return 0;
//         }
//         return 1;
//     }

//     return 0;
// }

// void mos_control(int value)
// {
//     for (int i = 0; i < 4; i++)
//     {
//         mos_status[i] = value % 10;
//         mos_set(mos_pin[i], mos_value[mos_status[i]]);
//         value /= 10;
//     }
// }
void mos_control(uint8_t control_value)
{
    Serial.println("control_value");
    Serial.println(control_value, HEX);
    if(control_value & (1<<0))
    {
        analogWrite(mos_pin[0], 0);
    }
    else
    {
        analogWrite(mos_pin[0], 255);
    }

    if(control_value & (1<<1))
    {
        analogWrite(mos_pin[1], 0);
    }
    else
    {
        analogWrite(mos_pin[1], 255);
    }

    if(control_value & (1<<2))
    {
        analogWrite(mos_pin[2], 0);
    }
    else
    {
        analogWrite(mos_pin[2], 255);
    }

    if(control_value & (1<<3))
    {
        analogWrite(mos_pin[3], 0);
    }
    else
    {
        analogWrite(mos_pin[3], 255);
    }
}

// int get_mos_status()
// {
//     int status = 0;
//     for (int i = 3; i >= 0; i--)
//     {
//         status *= 10;
//         status += mos_status[i];
//     }

//     return status;
// }

// //Mos set
// void mos_init()
// {
//     // SoftPWMBegin();
// }

// void mos_set(int8_t pin, uint16_t value)
// {
//     if (value > 255)
//         value = 255;
//     if (value < 0)
//         value = 0;
//     value = value * MOS_RATIO / 100;
//     // SoftPWMSet(pin, 255 - value);
//     analogWrite(pin, 255 - value);
// }

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