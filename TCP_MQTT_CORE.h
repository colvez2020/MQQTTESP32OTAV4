#ifndef TCPMQTT_H
#define TCPMQTT_H


#define TCP_START_ST         2
#define TCP_WAIT_ST          3
#define MQTT_START_ST        4
#define MQTT_MANTENICE_ST    5
#define TCP_MQTT_TIMEUP      6

#define MODO_WIFI						 7
#define MODO_GSM						 8


//Tipos de publicaciones
#define pSET_SW_ESTADO       2
#define pSET_KW_DOSI         3
#define pSET_KW_PRE          4
#define pSET_I_CORTE    		 5
#define pFECHA_corte         6
#define pFECHA_prepago       7
#define pUSER_DATA           8
#define pGPS_DATA            9
#define pMEDIDA_DATA         10
#define pGSM_DATA            11

//#define MODO_INDETERMINADO	 9   //intento por los modos y no pudo conectar

#include "PzemModbus.h"

bool TCP_Connect_ok(char modo_conexcion);
void TCP_Start(char modo_conexcion);
void TCP_Connect_reattempt(char modo_conexcion);
char TCP_MQTT_manager(char modo_conexcion);


bool MQTT_Maintenice_connect(void);
//char MQTT_publish(char* info);
char MQTT_reconnect(void);
void Setup_MQTT(char modo_conexcion);

void MQTT_callback(char* topic, byte* payload, unsigned int len); 

void MQTT_publish_topic(char* MQTT_OUTBONE_TOPIC,char* info);
void MQTT_publish_Atributes_config(void);
void MQTT_publish_GPS(void);
void MQTT_publish_PZEM_DOSI_PRE(PZEM_DataIO PZEM_actual_data);

#endif