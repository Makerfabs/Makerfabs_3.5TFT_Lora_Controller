# Makerfas 3.5“ TFT Lora Controller

```c++
/*
Version:		V1.2
Author:			Vincent
Create Date:	2021/9/18
Note:
		V1.1 Add more description.
		V1.2 Add wiki and shop link. Add Lora Kit firmware.
*/
```
![](md_pic/main.gif)



[toc]

# Makerfabs

[Makerfabs home page](https://www.makerfabs.com/)

[Makerfabs Wiki](https://makerfabs.com/wiki/index.php?title=Main_Page)



# Lora Radio Expansion

## Intruduce

Product Link ：[Lora-radio-expansion-for-esp32-display](https://www.makerfabs.com/lora-radio-expansion-for-esp32-display.html)

Wiki Link :  [LoRa_Radio_Expansion](https://www.makerfabs.com/wiki/index.php?title=LoRa_Radio_Expansion)


LoRa Radio expansion based on the LoRa module and provided a wireless Lot solution (ESP32-LoRa). This board is designed for ESP32 3.5" TFT Touch with Camera (also for the 3.2" one), mainly used with the ESP32 3.5" TFT Touch board.

We used LoRa Radio expansion and  ESP32 3.5" TFT Touch with Camera to make a Lora control terminal, which can control multiple Lora devices at the same time through the touch screen and display the device status in real time.

It is an extension of Touch Screen Camera series.

[ESP32 TFT LCD with Camera(3.5'')](https://www.makerfabs.com/wiki/index.php?title=ESP32_TFT_LCD_with_Camera(3.5%27%27))

[ESP32 TFT LCD with Camera(3.2'')](https://www.makerfabs.com/wiki/index.php?title=ESP32_TFT_LCD_with_Camera(3.2%27%27))

## Feature

- Onboard LoRa module (433Mhz, 868Mhz or 915Mhz)
- Long communication distance: 2km or more
- Active buzzer
- Power by 3.3V

### Front:

![front](md_pic/front.jpg)

### Back:
![back](md_pic/back.jpg)



# Makerfas 3.5“ TFT Lora Controller

**Disclaimer, this is only a demo, not as a hardware product for sale, only for reference.**

## Introduce

[ESP32 TFT LCD with Camera(3.5'')](https://www.makerfabs.com/wiki/index.php?title=ESP32_TFT_LCD_with_Camera(3.5%27%27)) was used as a demonstration.

An open Lora control terminal that currently supports 6 Makerfabs of Lora nodes (some are not for sale).  Realizes UI interface, can increase sensor according to UID. And there are different control interfaces for different nodes. 

Five nodes can be online at the same time and the node status is updated periodically. 

## How to use

We're going to post a video to show you how to do that.

### Main page

![main-page](md_pic/main-page.jpg)

The main page allows you to add nodes, clear all nodes, and enter receive mode.

The status of each logged node can be updated periodically.

And you can click the node button to enter the node interface.

### Node Page

![ac-dimmer](md_pic/ac-dimmer.jpg)

In node page, you can view the node type, UID, and icon.

You can also refresh the node status periodically.

### Node Control Page

![](md_pic/relay-control.jpg)

![](md_pic/ac-dimmer-control.jpg)

Some types of nodes have control functions, such as controlling the Relay switch and the size of AC Dimmer.

Click the button to enter the keypad or change the switch state.

## Code Upload

Firmware is in .\firmware\ESP32TFT3.5-LORA

### Compiler Options

**If you have any questions，such as how to install the development board, how to download the code, how to install the library. Please refer to :[Makerfabs_FAQ](https://github.com/Makerfabs/Makerfabs_FAQ)**

- Install board : ESP32 (Version 1.0.6)

![](md_pic/board-version.jpg)

- Install library: LovyanGFX, RadioLib(Version 4.6.0)

![](md_pic/library-version.jpg)

- Copy all bmp file to TF cards and insert to esp32
- Upload codes, select "ESP32 Wrover Module" and "Huge APP"

*Use other version board or library maybe not be able to complied.*


### Change Lora Frequency

Makerfas 3.5“ TFT Lora Controller main code. The default Lora frequency is 434 MHz, you can change these codes:

```c++
//In .\firmware\ESP32TFT3.5-LORA\makerfabs_pin.h

#define FREQUENCY 434.0
#define BANDWIDTH 125.0
#define SPREADING_FACTOR 9
#define CODING_RATE 7
#define OUTPUT_POWER 10
#define PREAMBLE_LEN 8
#define GAIN 0
```

# Example Nodes

We removed the relevant Lora Nodes demo.
Now all based on the example of "328p + lora" has moved to the [Makerfabs_MaLora]( ()https://github.com/Makerfabs/Makerfabs_MaLora) in the project.



# Lora Soil Monitoring & Irrigation Kit

## Introduce

This Kit is based on ESP32 and Lora. The ESP32 3.5 inch display is the console for the system, it receives the Lora message from Lora moisture sensors (support up to 8 sensors in our default firmware) , and send control commands to Lora 4-chnanel MOSFET(2 4-channel MOSFET supported, with totally 8 channels), to control the connected valves open/close, and thus to control the irrigation for multiple points.

Shop Link: [Lora Soil Monitoring & Irrigation Kit](https://www.makerfabs.com/lora-soil-monitoring-irrigation-kit.html)

![kit](md_pic/kit.jpg)

Please refer to instructables for instructions:

Instructables Link: [Soil-Monitoring-and-Irrigation-With-Lora](https://www.instructables.com/Soil-Monitoring-and-Irrigation-With-Lora/)

Youtube Link: [Soil Monitoring and Irrigation with Lora - How it Works](https://www.youtube.com/watch?v=p6YZIw9I9zE&feature=emb_imp_woyt)

