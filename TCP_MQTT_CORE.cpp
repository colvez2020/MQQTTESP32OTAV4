#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h> 
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "ESP_MEM_IO.h"
#include "MODEM_USER.h"
#include "SYS_Config.h"
#include "OTAcore.h"
#include "GPS_main.h"
#include "PzemModbus.h"
#include "TCP_MQTT_CORE.h"
#include "WIFI_USER.h"

#define TCPMQTT_DEBUG

TinyGsm                GPRS_SESION(Serial2);
TinyGsmClient          GSM_TCP_client(GPRS_SESION);
WiFiClient             WIFI_TCP_client; // Use WiFiClient class to create TCP connections
PubSubClient  	       ESP32_MQTT_client;
static Parametros_CFG  ESP32_Parametros;

int             SERVERPORT   = 1883;
String          CLIENT_ID;
String          MQTT_OUTBONE_TOPIC;
String          PASSWORD_MQTT;
const char      MQTT_SERVER[20]   = "tecno5.ddns.net";  
//int test=0;
//float KM_float=0.0;
boolean         GSM_Setup_succes_flag=false;

////////////////////Manager//////////////////
int             waitCount = 0;                 // counter
static char     conn_stat = TCP_START_ST;                       // Connection status for WiFi and MQTT:
static char     first_attempt=1;
//Se encarga de mantener la conexcion WIFI y MTQQ.

void Set_GSM_Setup_succes_flag(void)
{
  GSM_Setup_succes_flag=true;
}

void Clear_GSM_Setup_succes_flag(void)
{
  GSM_Setup_succes_flag=false;
}
bool TCP_Connect_ok(char modo_conexcion)
{
	switch(modo_conexcion)
  {
    case MODO_WIFI:
      if (WiFi.status() == WL_CONNECTED)
          return true;
    break;
    case MODO_GSM:
      if(GSM_Setup_succes_flag==true)
      {
        return GSM_gprs_link_up();
      }
      return false;
    break;
  }
	return false;
}

void TCP_Start(char modo_conexcion)
{
	switch(modo_conexcion)
  {
    case MODO_WIFI:
      Connect_WIFI(ESP32_Parametros.SSID_char, ESP32_Parametros.SSID_PASS_char);
    break;
    case MODO_GSM:
      if(GSM_Setup_succes_flag==false)
      {
        if(Setup_GSM())
        {
          GSM_Setup_succes_flag=true;
           GSM_set_gprs_link();
        }
      }
    break;
  }
}

void TCP_Connect_reattempt(char modo_conexcion)
{
	switch(modo_conexcion)
  {
    case MODO_WIFI:
    break;
    case MODO_GSM:
      GSM_Setup_succes_flag=false;
    break;
  }
  conn_stat=TCP_START_ST;
}


char TCP_MQTT_manager(char modo_conexcion)
{
  int Time_up_veces=2;
  
  if (TCP_Connect_ok(modo_conexcion))                             
  { 
    if(!ESP32_MQTT_client.connected()) 
    {
      if(conn_stat != MQTT_START_ST) 
       	waitCount = 0;
      conn_stat=MQTT_START_ST;                                    //ir a conectar MQTT
    }
    //Serial.println("TCP UP");
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
      if(modo_conexcion==MODO_WIFI)
        Time_up_veces=4;
      if(waitCount==Time_up_veces)
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
      if(waitCount==4)
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
  conn_stat=TCP_START_ST;
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


void MQTT_publish_Atributes_config(void)
{
  StaticJsonDocument<200> DOC_JSON_atributos;
  char DOC_JSON_atributos_buffer[201];
  char params_char[25];
  char temp;
  
  read_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),
                  Data_size(TYPE_PARAMETROS_CFG),
                  (char*)&ESP32_Parametros);
  //Parametos de caja congiguracion del USUARIO-WIFI
  //130 bytes
  //--------------------------------------------------------------------------
  DOC_JSON_atributos["USER"]=ESP32_Parametros.USERCAJA_char;     //29 +6
  DOC_JSON_atributos["TLF"]=ESP32_Parametros.N_telefono;         //10 +5 + 2
  DOC_JSON_atributos["DIR"]=ESP32_Parametros.DIRECCION_char;         //10 +5 + 2
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer);
  //--------------------------------------------------------------------------

  DOC_JSON_atributos.clear();
  DOC_JSON_atributos["RED_OP"]=ESP32_Parametros.ORCOM_char;      //10 +8
  DOC_JSON_atributos["CONTRATO"]=ESP32_Parametros.CTOSUS_char;   //11 +10
  DOC_JSON_atributos["TRAFO"]=ESP32_Parametros.CDTRAFO_ID_char;   //11 +10
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer);
  //--------------------------------------------------------------------------

  DOC_JSON_atributos.clear();

  DOC_JSON_atributos["VERSION_SW"]=SW_VER;  //5  +9
  DOC_JSON_atributos["SSID"]=ESP32_Parametros.SSID_char;         //10 +6
  DOC_JSON_atributos["SSIDPW"]=ESP32_Parametros.SSID_PASS_char;  //15 +8
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer); 
  //--------------------------------------------------------------------------

  DOC_JSON_atributos.clear();
  snprintf (params_char,19,"#%s",ESP32_Parametros.IMYCO_ID_char);
  DOC_JSON_atributos["CAJA_ID"]=params_char;  //5  +9
  return_User_Config_char_20(params_char);
  DOC_JSON_atributos["MODO"]=params_char;
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer);
  //--------------------------------------------------------------------------
 
  DOC_JSON_atributos.clear();
  read_flashESP32(Add_flash(TYPE_FECHA_CORTE_KW),Data_size(TYPE_FECHA_CORTE_KW),(char*)&temp);  
  DOC_JSON_atributos["DIA_COR_KW"]=temp;  //5  +9
  read_flashESP32(Add_flash(TYPE_FECHA_CORTE_PR),Data_size(TYPE_FECHA_CORTE_PR),(char*)&temp);  
  DOC_JSON_atributos["DIA_COR_PRE"]=temp;
  return_Limitacion_char_5(params_char);
  DOC_JSON_atributos["VALOR_LIMITACION"]=params_char;
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer);
  //--------------------------------------------------------------------------

  DOC_JSON_atributos.clear();
  DOC_JSON_atributos["PRE_RECAR"]="No aplica";         //Valor ultima recarga -1. Desactivado
  DOC_JSON_atributos["DOSI_DIAKW"]="No aplica";        //Valor de dosificacion diaria -1. Desactivado
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer);
  //--------------------------------------------------------------------------

  DOC_JSON_atributos.clear();
  DOC_JSON_atributos["DIV"]="IMYCO";
  DOC_JSON_atributos["FRA"]="TECNO5 S.A.S";
  read_flashESP32(Add_flash(TYPE_FECHA_FAB),16,(char*)&params_char);
  DOC_JSON_atributos["FECHA_F"]=params_char;
  snprintf (params_char,19,"BT%s",ESP32_Parametros.IMYCO_ID_char);
  DOC_JSON_atributos["BLUE_ID"]=params_char;  //5  +9
  Get_MAC_19(params_char);
  DOC_JSON_atributos["MAC"]=params_char;
  serializeJson(DOC_JSON_atributos,
                DOC_JSON_atributos_buffer, 
                200);
  Serial.println(DOC_JSON_atributos_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",DOC_JSON_atributos_buffer);
  
}

//linea modificada en casa
// int - the client state, which can take the following values (constants defined in PubSubClient.h):
// -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
// -3 : MQTT_CONNECTION_LOST - the network connection was broken
// -2 : MQTT_CONNECT_FAILED - the network connection failed
// -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
//  0 : MQTT_CONNECTED - the client is connected
//  1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
//  2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
//  3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
//  4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
//  5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
char MQTT_reconnect(void) 
{
  PASSWORD_MQTT="";
  PASSWORD_MQTT=String("TOKEN_")+ String(ESP32_Parametros.IMYCO_ID_char); //TOKEN_XXXX
  CLIENT_ID="";
  CLIENT_ID=String("E_")+String(ESP32_Parametros.IMYCO_ID_char);  //EXXXX identifica al medidor en el sistema MQTT
  Serial.println("Attempting MQTT connection...");
  
  #ifdef TCPMQTT_DEBUG
  Serial.print("Connecting to ");
  Serial.println(MQTT_SERVER);
  Serial.print("Cliente_ID: ");
  Serial.println(CLIENT_ID.c_str());
  Serial.print("Pass: ");
  Serial.println(PASSWORD_MQTT.c_str());
  #endif
  //boolean connect (clientID, username, password)
  if (ESP32_MQTT_client.connect(CLIENT_ID.c_str(),
                                PASSWORD_MQTT.c_str(),
                                NULL)) 
  {
    ESP32_MQTT_client.subscribe("v1/devices/me/rpc/request/+"); //llegan las peticiones de botones, indicador y perillas
    ESP32_MQTT_client.subscribe("v1/devices/me/attributes");    //llegan las peticiones de los cuadro de dialogo
    //Mando todos los atributos del dispositivo
    MQTT_publish_Atributes_config();
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




void MQTT_publish_topic(char* MQTT_OUTBONE_TOPIC,char* info)
{ 
  //ESP32_MQTT_client.publish(ESP32_Topic_respuesta.c_str(),ESP32_JSON_respuesta_char);
  ESP32_MQTT_client.publish(MQTT_OUTBONE_TOPIC,info);

}
//TELEMETRIA

void MQTT_publish_PZEM_DOSI_PRE(PZEM_DataIO PZEM_actual_data, char modo_conexcion)
{
  StaticJsonDocument<200> DOC_PZEM_DOSI_PRE;
  char                    JSON_PZEM_DOSI_PRE_buffer[201];
  Dosifi_info             Dosi_Configuracion_actual;
  Prepago_info            Prepago_Configuracion_actual;
  int                     modo_opecion;

  return_Dosifi_info(&Dosi_Configuracion_actual);
  return_Prepago_info(&Prepago_Configuracion_actual);
  modo_opecion=return_User_Config_Data();
  //KM_float = KM_float + (random(0,100)/10.4);
  #define EST_NORMAL      1     //Sin limitacion 80 max
#define EST_LIMITADO    2     //Limitado a un valor
#define EST_PENALIZADO  3     //Desconectado por sobrecorriente
#define EST_DOFICACION  4     //En Dosifi
#define EST_PREPAGO     5     //En prepago
#define EST_SUS_NORMA   6     //En Suspencion (6-9) 
#define EST_SUS_LIMI    7     //En Suspencion (6-9)
#define EST_SUS_DOSI    8     //En Suspencion (6-9)
#define EST_SUS_PREPA   9     //En Suspencion (6-9)
  if(modo_conexcion==MODO_GSM)
  {
    DOC_PZEM_DOSI_PRE["CKWH"]=PZEM_actual_data.energy_user;  //OK
    DOC_PZEM_DOSI_PRE["I"]=PZEM_actual_data.current;
    switch (modo_opecion) 
    {
      case EST_DOFICACION:
        DOC_PZEM_DOSI_PRE["DOSI_DISPO"]=Dosi_Configuracion_actual.KM_Disponible;
      break;
      case EST_PREPAGO:
        DOC_PZEM_DOSI_PRE["PRE_SALDO"]=Prepago_Configuracion_actual.KM_Saldo; //Saldo prepago disponible
        DOC_PZEM_DOSI_PRE["PRE_TOTALKW"]=Prepago_Configuracion_actual.KM_Consumo_total;   //Consumo total en modo prepago
      break;
    }
  }
  else
  {
    DOC_PZEM_DOSI_PRE["VOL"]=PZEM_actual_data.voltage;                                //Voltaje
    DOC_PZEM_DOSI_PRE["RKWH"]=PZEM_actual_data.energy;                                //Valor registro puro
    DOC_PZEM_DOSI_PRE["CKWH"]=PZEM_actual_data.energy_user;  //OK                                         //Consumo usuario
    //KM_float = KM_float + (random(0,100)/10.4);
    DOC_PZEM_DOSI_PRE["PRE_SALDO"]=Prepago_Configuracion_actual.KM_Saldo; //Saldo prepago disponible
    //Serial.print("SALDO=");
    //Serial.println(Prepago_Configuracion_actual.KM_Saldo);
    DOC_PZEM_DOSI_PRE["PRE_TOTALKW"]=Prepago_Configuracion_actual.KM_Consumo_total;   //Consumo total en modo prepago
    if(PZEM_actual_data.energy_alCorte==-1)
    {
      DOC_PZEM_DOSI_PRE["CORTE_KWH"]="No aplica";
    }
    else
    {
     DOC_PZEM_DOSI_PRE["CORTE_KWH"]=PZEM_actual_data.energy_alCorte;                  //Consumo desde el corte   
    }
    DOC_PZEM_DOSI_PRE["I"]=PZEM_actual_data.current;                                  //Corriente actual 
    DOC_PZEM_DOSI_PRE["DOSI_DISPO"]=Dosi_Configuracion_actual.KM_Disponible;          //Saldo actual disponible   
  }
  serializeJson(DOC_PZEM_DOSI_PRE,
                JSON_PZEM_DOSI_PRE_buffer, 
                200);
  Serial.println(JSON_PZEM_DOSI_PRE_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/telemetry",JSON_PZEM_DOSI_PRE_buffer);
}

void MQTT_publish_GPS(char modo_conexcion)
{
  GPS_info Actual_GPS_DATA;
  StaticJsonDocument<200> DOC_JSON_GPS;
  char                    JSON_GPS_buffer[201];


  return_GPS_info_data(&Actual_GPS_DATA);

  if(modo_conexcion==MODO_WIFI)
  {
    DOC_JSON_GPS["ULTIMA_LAT"]=Actual_GPS_DATA.Latitud_ultima_conocida;
    DOC_JSON_GPS["ULTIMA_LON"]=Actual_GPS_DATA.Longitud_ultima_conocida;
    DOC_JSON_GPS["SATELITES"]=Actual_GPS_DATA.Satelites_found;
    if(Actual_GPS_DATA.Fix_data==1)
      DOC_JSON_GPS["POSCION_ENCONTRADA"]="SI";
    else
      DOC_JSON_GPS["POSCION_ENCONTRADA"]="NO";
    serializeJson(DOC_JSON_GPS,
                JSON_GPS_buffer, 
                200);
    MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_GPS_buffer);
  }
  DOC_JSON_GPS.clear();
  DOC_JSON_GPS["latitude"]=Actual_GPS_DATA.Latitud_leida;
  DOC_JSON_GPS["longitude"]=Actual_GPS_DATA.Longitud_leida;
  serializeJson(DOC_JSON_GPS,
                JSON_GPS_buffer, 
                200);
  //Serial.println(JSON_GPS_buffer);
  MQTT_publish_topic((char*)"v1/devices/me/telemetry",JSON_GPS_buffer);
}

void MQTT_publish_led(void)
{
  boolean                 Parametro_bool; 
  StaticJsonDocument<200> DOC_JSON_led;
  char                    JSON_led_buffer[201];

  return_User_relay_estado(&Parametro_bool);
  DOC_JSON_led["RELAY"]=Parametro_bool; 
  serializeJson(DOC_JSON_led,
              JSON_led_buffer, 
              200);
  MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_led_buffer);
}

void MQTT_callback(char* topic, byte* payload, unsigned int len) 
{
  char                    JSON_temp_buffer[201];
  char                    JSON_respuesta_buffer[201];
  String                  ESP32_Topic_respuesta;
  boolean                 Parametro_bool;
  boolean                 Parametro_Dash=false;
  StaticJsonDocument<200> DOC_JSON_payload;
  StaticJsonDocument<200> DOC_JSON_Respuesta;
  //JsonObject root = DOC_JSON_payload.to<JsonObject>()
  float Limitacion_DASH,Dosificacion_DASH,Recarga_pregago_DASH;
  char   Dia_corte_KW;
  char params_char[20];
  

  strncpy (JSON_temp_buffer, (char*)payload, len);
  JSON_temp_buffer[len] = '\0';
  //JSON_respuesta_buffer
 
  //OTA_info  ESP32_Estado_OTA_MQTT;
  //boolean Param_boolean;

  //#ifdef TCPMQTT_DEBUG
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]: ");
  Serial.println(JSON_temp_buffer);


  //Des_señalizo (leeo la cadena para luego aceder a sus datos usando el metodo JSON)
  //JSON_payload_char se borra en el proceso.
  
  auto error= deserializeJson(DOC_JSON_payload,(char*)JSON_temp_buffer);
  if (error) 
  {
    Serial.print(F("Error al leer JSON_payload"));
    Serial.println(error.c_str());
    return;
  }
  else
  {
    //Verifico Configuracion
    Recarga_pregago_DASH  = DOC_JSON_payload["PRE_RECAR"];
    Dosificacion_DASH =  DOC_JSON_payload["DOSI_DIAKW"] ;
    Limitacion_DASH =  DOC_JSON_payload["VALOR_LIMITACION"] ;
    Dia_corte_KW =     DOC_JSON_payload["DIA_COR_KW"] ;
    if(Dia_corte_KW!=0)
    {
      store_flashESP32(Add_flash(TYPE_FECHA_CORTE_KW),Data_size(TYPE_FECHA_CORTE_KW),(char*)&Dia_corte_KW);  
      DOC_JSON_payload["DIA_COR_KW"]=Dia_corte_KW;
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);

    }
    if(Recarga_pregago_DASH!=0.0)
    {
      Parametro_Dash=true;
      Prepago_Setup(Recarga_pregago_DASH);
      if(Recarga_pregago_DASH<0.0)
      {
        DOC_JSON_payload["PRE_RECAR"]="No aplica";
        Normaliza_Setup();
      }
    }
    if(Dosificacion_DASH!=0.0)
    {
      Parametro_Dash=true;
      Docificacion_Setup(Dosificacion_DASH);
      if(Dosificacion_DASH<0.0)
      {
        DOC_JSON_payload["DOSI_DIAKW"]="No aplica";
        Normaliza_Setup();
      }
    }
    if(Limitacion_DASH!=0.0)
    {
      Parametro_Dash=true;
      Limitacion_Setup(Limitacion_DASH);
      if(Dosificacion_DASH<0.0)
      {
        DOC_JSON_payload["VALOR_LIMITACION"]="No aplica";
        Normaliza_Setup();
      }
    }

    if(Parametro_Dash)
    {
      return_User_Config_char_20(params_char);
      DOC_JSON_Respuesta["MODO"]=params_char;
      return_User_relay_estado(&Parametro_bool);
      DOC_JSON_Respuesta["RELAY"]=Parametro_bool; 
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
    }

    // Check request method
    String methodName = String((const char*)DOC_JSON_payload["method"]);
    
    if (methodName.equals("GET_ONLINE"))
    {
      ESP32_Topic_respuesta = String(topic);
      ESP32_Topic_respuesta.replace("request", "response");
      DOC_JSON_Respuesta["ONLINE"]=false;      
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
    }
    if (methodName.equals("SET_SUSPENDER")) //SUSPENDER USUARIO
    {
      ESP32_Topic_respuesta = String(topic);
      ESP32_Topic_respuesta.replace("request", "response");
      DOC_JSON_Respuesta["SUSPENDER_MODO"]=false;
      /*return_User_relay_estado(&Parametro_bool);
      DOC_JSON_Respuesta["RELAY"]=Parametro_bool; 
      return_User_Config_char_20(params_char);
      DOC_JSON_Respuesta["MODO"]=params_char;  */ 
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
      Suspension_Setup();
      //actualizo led y modo
      DOC_JSON_Respuesta.clear();
      return_User_relay_estado(&Parametro_bool);
      DOC_JSON_Respuesta["RELAY"]=Parametro_bool; 
      return_User_Config_char_20(params_char);
      DOC_JSON_Respuesta["MODO"]=params_char;   
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      //MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
    }
    
    if (methodName.equals("SET_RECONECTAR")) //RECONECTAR USUARIO
    {
      ESP32_Topic_respuesta = String(topic);
      ESP32_Topic_respuesta.replace("request", "response");
      DOC_JSON_Respuesta["RECONECTAR_MODO"]=false;
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
      Reconect_Setup();
      //actualizo led
      DOC_JSON_Respuesta.clear();
      return_User_relay_estado(&Parametro_bool);
      DOC_JSON_Respuesta["RELAY"]=Parametro_bool;      
      return_User_Config_char_20(params_char);
      DOC_JSON_Respuesta["MODO"]=params_char;   
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      //MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
    }
    if (methodName.equals("SET_NORMAL")) // NORMALIZAR
    {
      ESP32_Topic_respuesta = String(topic);
      ESP32_Topic_respuesta.replace("request", "response");
      DOC_JSON_Respuesta["NORMAL_MODO"]=false;
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
      Normaliza_Setup();
      //actualizo led
      DOC_JSON_Respuesta.clear();
      return_User_relay_estado(&Parametro_bool);
      DOC_JSON_Respuesta["RELAY"]=Parametro_bool;      
      return_User_Config_char_20(params_char);
      DOC_JSON_Respuesta["MODO"]=params_char;   
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      //MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);

    }
    if (methodName.equals("GET_RELAY_ESTADO")) //Controla la luz indicadora de conexcion electrica.
    {
      ESP32_Topic_respuesta = String(topic);
      ESP32_Topic_respuesta.replace("request", "response");
      return_User_relay_estado(&Parametro_bool);
      DOC_JSON_Respuesta["RELAY"]=Parametro_bool;      
      serializeJson(DOC_JSON_Respuesta,
                    JSON_respuesta_buffer, 
                    200);
      //Serial.print("topic_respuesta=>");
      //Serial.println(ESP32_Topic_respuesta.c_str());
      Serial.println(JSON_respuesta_buffer);
      MQTT_publish_topic((char*)ESP32_Topic_respuesta.c_str(),JSON_respuesta_buffer);
      MQTT_publish_topic((char*)"v1/devices/me/attributes",JSON_respuesta_buffer);
    }
  }
  /*
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
  }*/
}

