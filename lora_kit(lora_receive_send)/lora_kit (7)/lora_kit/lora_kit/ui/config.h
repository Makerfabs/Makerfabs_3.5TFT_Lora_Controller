#ifndef CONFIG_H
#define CONFIG_H

//esp32s3
#define LORA_MOSI 10
#define LORA_MISO 4
#define LORA_SCK 5
#define LORA_CS 6

#define LORA_RST 17
#define LORA_DIO0 9
#define LORA_DIO1 46


#define FREQUENCY 915//434.0
#define BANDWIDTH 125.0     //   203.125-1625.0   125khz  250khz  500khz    带宽越大，数据传输速率越高，但接收灵敏度降低，通信距离缩短
#define SPREADING_FACTOR 12   //   7-12  扩频因子越大，通信距离越远，但数据传输速率越低
#define CODING_RATE 7    //    5-8   4/5、4/6、4/7、4/8   编码率越高，抗干扰能力越强，但传输时间增加，数据速率降低
#define OUTPUT_POWER 13   //  -18--13        10 dBm 到 22 dBm   发射功率越高，通信距离越远，但能耗增加
#define PREAMBLE_LEN 8   //前导码长度
#define GAIN 0

#define Lora_Init  0
#define Lora_Send  1
#define Lora_Recv  2
#define Lora_Sleep 3

// 任务自定义事件标识
// 报文上报事件
#define LORA_RECV_EV          (1u << 0)
#define LORA_SEND_EV      	  (1u << 1)
#define LORA_INIT_EV      	  (1u << 2)
#define LORA_REST_EV      	  (1u << 3)
#define LORA_LISTEN_EV        (1u << 4)


#define SD_SCK 42
#define SD_MISO 41
#define SD_MOSI 2
#define SD_CS 1

#define SPI_ON_SD   digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD  digitalWrite(SD_CS, HIGH)


#endif 