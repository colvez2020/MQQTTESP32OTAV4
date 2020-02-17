#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h> 
#include <PubSubClient.h>
#include <WiFi.h>
#include "ESP_MEM_IO.h"
#include "MODEM_USER.h"
#include "TCP_MQTT_CORE.h"
#include "SYS_Config.h"
#include "OTAcore.h"
#include "GPS_main.h"
#include "PzemModbus.h"

#define TCPMQTT_DEBUG

TinyGsm                GPRS_SESION(Serial2);
TinyGsmClient          GSM_TCP_client(GPRS_SESION);
WiFiClient             WIFI_TCP_client; // Use WiFiClient class to create TCP connections
PubSubClient  	       ESP32_MQTT_client;
static Parametros_CFG  ESP32_Parametros;

int             SERVERPORT   = 1883;
String          CLIENT_ID = "E";
String          MQTT_OUTBONE_TOPIC = "";
String          MQTT_OUT_DATA = "";
String          MQTT_INBONE_TOPIC = "";
const char      MQTT_SERVER[20]   = "tecno5.myddns.me"; 
const char      USERNAME[10]= "ado";  
const char      PASSWORD_MQTT[10] = "a";


////////////////////Manager//////////////////
int             waitCount = 0;                 // counter
static char     conn_stat = 0;                       // Connection status for WiFi and MQTT:
static char     first_attempt=1;
//Se encarga de mantener la conexcion WIFI y MTQQ.
bool TCP_Connect_ok(char modo_conexcion)
{
	switch(modo_conexcion)
  {
    case MODO_WIFI:
      if (WiFi.status() == WL_CONNECTED)
          return true;
    break;
    case MODO_GSM:
      if (GSM_gprs_link_up())
          return true;
    break;
  }
	return false;
}


void TCP_Start(char modo_conexcion)
{
	switch(modo_conexcion)
  {
    case MODO_WIFI:
      #ifdef TCPMQTT_DEBUG
      Serial.print("Connecting to ");
      Serial.println(ESP32_Parametros.SSID_char);
      Serial.print("Pass ");
      Serial.println(ESP32_Parametros.SSID_PASS_char);
      #endif 
      WiFi.begin(ESP32_Parametros.SSID_char, ESP32_Parametros.SSID_PASS_char);
    break;
    case MODO_GSM:
      GSM_set_gprs_link();                               //Si no se pude conectar.
    break;
  }
}

void TCP_Connect_reattempt(char modo_conexcion)
{
	switch(modo_conexcion)
  {
    case MODO_WIFI:
      delay(1000);
    break;
    case MODO_GSM:
      Setup_GSM();                          //Si no se pude conectar.
      conn_stat=TCP_START_ST;
    break;
  }
}


char TCP_MQTT_manager(char modo_conexcion)
{
  if (TCP_Connect_ok(modo_conexcion))                             //gPRS OK
  { 
    if(!ESP32_MQTT_client.connected()) 
    {
      if(conn_stat != MQTT_START_ST) 
       	waitCount = 0;
      conn_stat=MQTT_START_ST;                                    //ir a conectar MQTT
    }
  }
  else
  {
    
    if(first_attempt)
      waitCount = 0;
    if(conn_stat != TCP_WAIT_ST)  
      conn_stat = TCP_START_ST;                                  //ir a coenctar gPRS
  }

  switch (conn_stat) 
  {
    case TCP_START_ST: 
      #ifdef TCPMQTT_DEBUG   
      Serial.println("MQTT and TCP down: start TCP");
      #endif
     	TCP_Start(modo_conexcion);
      conn_stat = TCP_WAIT_ST;
      first_attempt=0;
    break;
    case TCP_WAIT_ST:                                                       // WiFi starting, esperando conexion
      Serial.println("TCP starting, wait : "+ String(waitCount));
      waitCount++;
      TCP_Connect_reattempt(modo_conexcion);
      if(waitCount==15)
      {
          Serial.println("TCP_CONNECT_TIMEOUT");
          waitCount = 0;
          return TCP_MQTT_TIMEUP;
      }   
    break;
    case MQTT_START_ST:                                                       // WiFi up, MQTT down: start MQTT
      #ifdef TCPMQTT_DEBUG 
      Serial.println("TCP up, MQTT down: start MQTT");
      #endif
      if(MQTT_reconnect())
      {
        Serial.println("MQTT connected");
        conn_stat = MQTT_MANTENICE_ST;
        waitCount = 0;
        break;
      }
      waitCount++;
      Serial.println("TCP up, MQTT starting, wait : "+ String(waitCount));
      if(waitCount==10)
      {
          Serial.println("MQTT_CONNECT_TIMEOUT");
         	waitCount = 0;
          return TCP_MQTT_TIMEUP;
      }       
    break;
    case MQTT_MANTENICE_ST:                                                       // WiFi up, MQTT up: Mantener MQTT
      return MQTT_Maintenice_connect();                  
    break;
  }
  return conn_stat;
}

void Setup_MQTT(char modo_conexcion)
{
	read_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),
                  Data_size(TYPE_PARAMETROS_CFG),
                  (char*)&ESP32_Parametros);

	switch (modo_conexcion)
  {
    case MODO_WIFI:
      ESP32_MQTT_client.setClient(WIFI_TCP_client);        //Permite una conexcion TCP
      Serial.println("MQTT_MODO_WIFI");
    break;
    case MODO_GSM:
      ESP32_MQTT_client.setClient(GSM_TCP_client);
      Serial.println("MQTT_MODO_GSM");
    break;
  }
  ESP32_MQTT_client.setServer(MQTT_SERVER,SERVERPORT);  //Configura parametros de conexion
  ESP32_MQTT_client.setCallback(MQTT_callback);              //Asigna funcion_callaback
}


bool MQTT_Maintenice_connect(void)
{
  //Mantenimiento de conexcion
  if (!ESP32_MQTT_client.connected()) 
  {
     MQTT_reconnect();
  }
  return ESP32_MQTT_client.loop();  
}

void MQTT_publish_topic(char* MQTT_OUTBONE_TOPIC,char* info)
{   
  ESP32_MQTT_client.publish(MQTT_OUTBONE_TOPIC,info);
}

//linea modificada en casa
char MQTT_reconnect(void) 
{

  MQTT_INBONE_TOPIC= "";
  MQTT_INBONE_TOPIC+= String(ESP32_Parametros.IMYCO_ID_char);
  MQTT_INBONE_TOPIC+=String("/CMD"); 

  CLIENT_ID = String(ESP32_Parametros.IMYCO_ID_char);  //E-0001 identifica al medidor en el sistema MQTT
  Serial.println("Attempting MQTT connection...");
  
  #ifdef TCPMQTT_DEBUG
  Serial.print("Connecting to ");
  Serial.println(MQTT_SERVER);
  Serial.print("Cliente_ID: ");
  Serial.println(CLIENT_ID.c_str());
  #endif
  
  if (ESP32_MQTT_client.connect(CLIENT_ID.c_str(),
                                USERNAME,
                                PASSWORD_MQTT)) 
  {
    
    ESP32_MQTT_client.subscribe(MQTT_INBONE_TOPIC.c_str()); //recibe_info
    MQTT_OUT_DATA=String("START_");
    MQTT_OUT_DATA+=String(ESP32_Parametros.IMYCO_ID_char);
    ESP32_MQTT_client.publish("GSM_CLIENT/INI",MQTT_OUT_DATA.c_str()); //Manda START_(MEDIDOR_NUM)     
  } 
  else 
  {
    Serial.print("MQTT_connect_failed, rc=");
    Serial.println(ESP32_MQTT_client.state());
    return 0;
  }
  Serial.println("MQTT_INI_connected!!");
  return 1;
}


void MQTT_callback(char* topic, byte* payload, unsigned int len) 
{
  char      ESP32_Parametro_char[10]="";
  char      ESP32_Mensaje_MQTT[100];
  char      Topic_respuesta[] = "RPST_CAJA";
  OTA_info  ESP32_Estado_OTA_MQTT;



  //#ifdef TCPMQTT_DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(len);
  for (int i = 0; i < len; i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
 // #endif
  if(len>1)
  {
    for (int i = 1; i < len; i++) 
    {
      ESP32_Parametro_char[i-1]=(char)payload[i];
    }
  }
  snprintf (ESP32_Mensaje_MQTT,5,"CMDOK");
  
  switch (payload[0]) 
  {
    case 'N':  
       Normaliza_Setup();
       MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    case 'S':
      Suspension_Setup();
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    case 'R': 
      Reconect_Setup();
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break; 
    case 'L':
      Limitacion_Setup_4(ESP32_Parametro_char);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;  
    case 'D':
      Docificacion_Setup_4(ESP32_Parametro_char);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT); 
    break;
    case 'P':
      Prepago_Setup_4(ESP32_Parametro_char);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);  
    break;
    case 'O': //INFORMACION PREPAGO
      //AA,MM,DD,RW,SPW,DW
      //(AA,MM,DD) Fecha ultima recarga
      //(RW)       Cantidad ultima recarga
      //(SPW)      Saldo previo
      //(DW)       Saldo disponible
      Estatus_prepago_50(ESP32_Mensaje_MQTT);
      Serial.println(ESP32_Mensaje_MQTT);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    case 'I': //INFORMACION DE PARAMETROS
      //AA,MM,DD, CW, LA, DW, PW, ES, RR 
      //(AA,MM,DD) Fecha ultimo corte
      //(CW)       Energuia activa ultimo corte
      //(LA)       Limitado a 
      //(DW)       Energia Dofisicacion cuota
      //(PW)       Energia Saldo Dosificacion
      //(ES)       Estado o Modo de la caja (Normalizado (Lim a 80A), Limitado, Pregago, Dosificacion)
      //(RR)       Estado actual del relay (Abierto o Cerrado)
      Estatus_KWLIMDOFI_50(ESP32_Mensaje_MQTT);
      Serial.println(ESP32_Mensaje_MQTT);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    case 'G': //INFORMACION GPS
      GPS_Status_100(ESP32_Mensaje_MQTT);
      Serial.println(ESP32_Mensaje_MQTT);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    case 'E': //INFORMACION MEDIDAS INSTANTANEA
    //W/H,A,W,FP,V
      Estatus_PZEM_50(ESP32_Mensaje_MQTT);
      snprintf (ESP32_Mensaje_MQTT,100,"%s, %d",ESP32_Mensaje_MQTT,return_User_Config_Data());
      Serial.println(ESP32_Mensaje_MQTT);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    case 'F':  //Activa el modo_OTA (Leer archibo.bin de la AW3 y quemarlo usando Update.h)
      ESP32_Estado_OTA_MQTT.OTA_Activado=1;
      Almacenar_OTA_Data(&ESP32_Estado_OTA_MQTT);
      Serial.print("Activo_modo OTA");
      Serial.print("Reinicio para exe OTA");
      ESP.restart();
    break;
    case 'W': //INFO_Estado_OTA
      Estatus_OTA_50(ESP32_Mensaje_MQTT);
      Serial.println(ESP32_Mensaje_MQTT);
      MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
    default:
    snprintf (ESP32_Mensaje_MQTT,16,"CMD_DESCONOCIDO");
    MQTT_publish_topic(Topic_respuesta,ESP32_Mensaje_MQTT);
    break;
  }
}

