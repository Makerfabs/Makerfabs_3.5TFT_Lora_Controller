
#ifndef _Lora_H_
#define _Lora_H_

#include <arduino.h>
#include <RadioLib.h>

#include "makerfabs_pin.h"

class Lora
{
private:
    SX1276 *radio;

public:
    Lora(SX1276 *radio);
    void init();

    String receive();
    String receive_intr();
    void send(String command);

    String reply_analyse(String reply);
    String command_format(String id, int act, int param);

    static void setFlag(void);

    static volatile bool receivedFlag;
    static volatile bool enableInterrupt;
};

#endif