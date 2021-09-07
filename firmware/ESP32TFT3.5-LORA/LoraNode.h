#ifndef _LoraNode_H_
#define _LoraNode_H_

#include <arduino.h>

#define NOTYPE -1

//Define Lora Node for save node message
class LoraNode
{
private:
    int id;
    int type;
    int enable_flag;
    String id_str;
    String type_str;
    String text;

public:
    LoraNode();
    void setNode(int id, int type);
    void clean();

    int getId();
    void setId(int i);
    int getType();
    void setType(int i);
    String getText();
    void setText(String ts);
    String getId_str();
    void setId_str(String ts);
    String getType_str();
    void setType_str(String ts);
    int getEnable_flag();
    void setEnable_flag(int i);

    void createID_str();
    void createType_str();
};

String get_type_str(int type);

#endif