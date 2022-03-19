#include "Lora.h"

Lora::Lora(SX1278 *radio)
{
    this->radio = radio;
}

void Lora::init()
{
    int state = this->radio->begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
    if (state == ERR_NONE)
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
        Serial.print(F("[SX1278] Waiting for incoming transmission ... "));

    String str;
    int state = this->radio->receive(str);

    if (state == ERR_NONE)
    {
        if (DEBUG)
        {
            // packet was successfully received
            Serial.println(F("success!"));

            // print the RSSI (Received Signal Strength Indicator)
            // of the last received packet
            Serial.print(F("[SX1278] RSSI:\t\t\t"));
            Serial.print(this->radio->getRSSI());
            Serial.println(F(" dBm"));

            // print the SNR (Signal-to-Noise Ratio)
            // of the last received packet
            Serial.print(F("[SX1278] SNR:\t\t\t"));
            Serial.print(this->radio->getSNR());
            Serial.println(F(" dB"));

            // print frequency error
            // of the last received packet
            Serial.print(F("[SX1278] Frequency error:\t"));
            Serial.print(this->radio->getFrequencyError());
            Serial.println(F(" Hz"));
        }

        // print the data of the packet
        Serial.print(F("[SX1278] Data:\t\t\t"));
        Serial.println(str);

        return str;
    }
    else if (state == ERR_RX_TIMEOUT)
    {
        // timeout occurred while waiting for a packet
        if (DEBUG)
            Serial.println(F("timeout!"));
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
        if (state == ERR_NONE)
        {
            if (DEBUG)
            {
                // packet was successfully received
                Serial.println(F("success!"));

                // print the RSSI (Received Signal Strength Indicator)
                // of the last received packet
                Serial.print(F("[SX1278] RSSI:\t\t\t"));
                Serial.print(this->radio->getRSSI());
                Serial.println(F(" dBm"));

                // print the SNR (Signal-to-Noise Ratio)
                // of the last received packet
                Serial.print(F("[SX1278] SNR:\t\t\t"));
                Serial.print(this->radio->getSNR());
                Serial.println(F(" dB"));

                // print frequency error
                // of the last received packet
                Serial.print(F("[SX1278] Frequency error:\t"));
                Serial.print(this->radio->getFrequencyError());
                Serial.println(F(" Hz"));
            }

            // print the data of the packet
            Serial.print(F("[SX1278] Data:\t\t\t"));
            Serial.println(str);
        }
        else if (state == ERR_CRC_MISMATCH)
        {
            // packet was received, but is malformed
            Serial.println(F("[SX1278] CRC error!"));
        }
        else
        {
            // some other error occurred
            Serial.print(F("[SX1278] Failed, code "));
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
