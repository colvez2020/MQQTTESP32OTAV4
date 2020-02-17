#ifndef TCPMQTT_H
#define TCPMQTT_H


#define TCP_START_ST         2
#define TCP_WAIT_ST          3
#define MQTT_START_ST        4
#define MQTT_MANTENICE_ST    5
#define TCP_MQTT_TIMEUP      6

#define MODO_WIFI						 7
#define MODO_GSM						 8
//#define MODO_INDETERMINADO	 9   //intento por los modos y no pudo conectar


bool TCP_Connect_ok(char modo_conexcion);
void TCP_Start(char modo_conexcion);
void TCP_Connect_reattempt(char modo_conexcion);
char TCP_MQTT_manager(char modo_conexcion);


bool MQTT_Maintenice_connect(void);
char MQTT_publish(char* info);
char MQTT_reconnect(void);
void Setup_MQTT(char modo_conexcion);
void MQTT_publish_topic(char MQTT_OUTBONE_TOPIC,char* info);
void MQTT_callback(char* topic, byte* payload, unsigned int len); 

#endif