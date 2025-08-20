#include "Lora.h"

Lora::Lora(SX1276 *radio)
{
    this->radio = radio;
}

void Lora::init()
{
    Serial.print("Lora frequency ");
    Serial.println(FREQUENCY);
    int state = this->radio->begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, RADIOLIB_SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println(F("Lora init success!"));
    }
    else
    {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true)
            ;
    }

    /*
    //Enable interrupt
    this->radio->setDio0Action(this->setFlag);
    state = this->radio->startReceive();
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
    */
}

String Lora::receive()
{
    if (DEBUG)
        Serial.print(F("[SX1276] Waiting for incoming transmission ... "));

    String str;
    
    int state = this->radio->receive(str);

    // RADIOLIB_ERR_LORA_HEADER_DAMAGED can be ignored
    // https://github.com/jgromes/RadioLib/pull/1262/files
    if (state == RADIOLIB_ERR_NONE || state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)
    {
        if (DEBUG)
        {
            // packet was successfully received
            Serial.println(F("success!"));

            // print the RSSI (Received Signal Strength Indicator)
            // of the last received packet
            Serial.print(F("[SX1276] RSSI:\t\t\t"));
            Serial.print(this->radio->getRSSI());
            Serial.println(F(" dBm"));

            // print the SNR (Signal-to-Noise Ratio)
            // of the last received packet
            Serial.print(F("[SX1276] SNR:\t\t\t"));
            Serial.print(this->radio->getSNR());
            Serial.println(F(" dB"));

            // print frequency error
            // of the last received packet
            Serial.print(F("[SX1276] Frequency error:\t"));
            Serial.print(this->radio->getFrequencyError());
            Serial.println(F(" Hz"));
        }

        // print the data of the packet
        Serial.print(F("[SX1276] Data:\t\t\t"));
        Serial.println(str);

        return str;
    }
    else if (state == RADIOLIB_ERR_RX_TIMEOUT)
    {
        // timeout occurred while waiting for a packet
        if (DEBUG) {
            // timeout means no data received- but sensors only transmit once per hour so 99.9% of calls end here
            Serial.println(F("timeout!"));
        }
    }
    else if (state == RADIOLIB_ERR_CRC_MISMATCH)
    {
        // packet was received, but is malformed
        Serial.println(F("CRC error!"));
    }
    else if (state == RADIOLIB_ERR_LORA_HEADER_DAMAGED)
    {
        Serial.println(F("HEADER damaged!"));
    }
    else
    {
        // some other error occurred
        Serial.print(F("failed, code "));
        Serial.println(state);
    }
    //Serial.println("returning from receive()");
    // must always return something or get error: abort() was called at PC
    return "";
}

String Lora::receive_intr()
{
    String str = "";

    if (receivedFlag)
    {
        Serial.println(F("Receieve something!"));
        // disable the interrupt service routine while
        // processing the data
        this->enableInterrupt = false;

        // reset flag
        this->receivedFlag = false;

        // you can read received data as an Arduino String

        int state = this->radio->readData(str);
        if (state == RADIOLIB_ERR_NONE)
        {
            if (DEBUG)
            {
                // packet was successfully received
                Serial.println(F("success!"));

                // print the RSSI (Received Signal Strength Indicator)
                // of the last received packet
                Serial.print(F("[SX1276] RSSI:\t\t\t"));
                Serial.print(this->radio->getRSSI());
                Serial.println(F(" dBm"));

                // print the SNR (Signal-to-Noise Ratio)
                // of the last received packet
                Serial.print(F("[SX1276] SNR:\t\t\t"));
                Serial.print(this->radio->getSNR());
                Serial.println(F(" dB"));

                // print frequency error
                // of the last received packet
                Serial.print(F("[SX1276] Frequency error:\t"));
                Serial.print(this->radio->getFrequencyError());
                Serial.println(F(" Hz"));
            }

            // print the data of the packet
            Serial.print(F("[SX1276] Data:\t\t\t"));
            Serial.println(str);
        }
        else if (state == RADIOLIB_ERR_CRC_MISMATCH)
        {
            // packet was received, but is malformed
            Serial.println(F("[SX1276] CRC error!"));
        }
        else
        {
            // some other error occurred
            Serial.print(F("[SX1276] Failed, code "));
            Serial.println(state);
        }

        // put module back to listen mode
        this->radio->startReceive();

        // we're ready to receive more packets,
        // enable interrupt service routine
        this->enableInterrupt = true;
    }

    return str;
}

void Lora::send(String command)
{
    this->radio->transmit(command);
    if (DEBUG)
        Serial.println(command);
}

//Command format like:
//  ID000123ACT114PARAM012345
String Lora::command_format(String id, int act, int param)
{

    String lora_str = id;
    char act_param[20];
    sprintf(act_param, "ACT%03dPARAM%06d", act, param);
    lora_str += act_param;

    return lora_str;
}

//Relay always like:
//  ID000004 REPLY : DIM 0

String Lora::reply_analyse(String reply)
{
    int index = 0;
    index = reply.indexOf("REPLY");
    return reply.substring(index + 8);
}

// volatile bool receivedFlag = false;
// volatile bool enableInterrupt = true;
volatile bool Lora::receivedFlag = false;
volatile bool Lora::enableInterrupt = true;

void Lora::setFlag(void)
{
    // check if the interrupt is enabled
    if (!enableInterrupt)
    {
        return;
    }

    // we got a packet, set the flag
    receivedFlag = true;
}
