
#ifndef _Lora_H_
#define _Lora_H_

#include <arduino.h>
#include <RadioLib.h>

#include "makerfabs_pin.h"

class Lora
{
private:
    SX1278 *radio;

public:
    Lora(SX1278 *radio);
    void init();

    String receive();
    void send(String command);

    String reply_analyse(String reply);
    String command_format(String id, int act, int param);
};

#endif