#include "LoraNode.h"

int node_type_count = 8;
String node_type_list[100] =
    {
        "NULL",
        "DIMMER",       //TYPE 00
        "SOIL",         //TYPE 01
        "RELAY",        //TYPE 02
        "RELAY4",       //TYPE 03
        "TDS",          //TYPE 04
        "SOIL-PRO",      //TYPE 05
        "MOS4"
        };    

LoraNode::LoraNode()
{
    this->id = 0;
    this->type = NOTYPE;
    this->enable_flag = 0;

    this->text = "NULL";

    this->id_str = "NULL";
    this->type_str = "NULL";
}

void LoraNode::setNode(int id, int type)
{
    this->id = id;
    this->type = type;
    this->enable_flag = 1;
    this->createID_str();
    this->createType_str();
}

void LoraNode::clean()
{
    this->id = 0;
    this->type = 0;
    this->enable_flag = 0;

    this->text = "NULL";

    this->id_str = "NULL";
    this->type_str = "NULL";
}

int LoraNode::getId()
{
    return id;
}
void LoraNode::setId(int i)
{
    id = i;
}

int LoraNode::getType()
{
    return type;
}
void LoraNode::setType(int i)
{
    type = i;
}

String LoraNode::getText()
{
    return text;
}
void LoraNode::setText(String ts)
{
    text = ts;
}

String LoraNode::getId_str()
{
    return id_str;
}
void LoraNode::setId_str(String ts)
{
    id_str = ts;
}

String LoraNode::getType_str()
{
    return type_str;
}
void LoraNode::setType_str(String ts)
{
    type_str = ts;
}

int LoraNode::getEnable_flag()
{
    return enable_flag;
}
void LoraNode::setEnable_flag(int i)
{
    enable_flag = i;
}

void LoraNode::createID_str()
{
    id_str = "";
    char id_cstr[20];
    sprintf(id_cstr, "ID%06d", id);
    id_str += id_cstr;
}

void LoraNode::createType_str()
{
    type_str = "";

    char type_cstr[20];
    sprintf(type_cstr, "TYPE%02d", type);
    type_str += type_cstr;

    if (type + 1 < node_type_count)
    {
        type_str = node_type_list[type + 1];
    }
}


String get_type_str(int type)
{
    return node_type_list[type + 1];
}