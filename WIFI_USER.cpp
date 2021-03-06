#include <WiFiManager.h> 
#include <EEPROM.h> //512 bytes (microcontrolador)
#include <WiFi.h>
#include "WIFI_USER.h"
#include "ESP_MEM_IO.h"


#define ENC_TYPE_NONE 7
#define WIFI_DEBUG

////////////////////WIFI_CFG//////////////////

String               Temp;
unsigned char        ID_WIFIMOdo_status;
Parametros_CFG       ESP32_Para_CFG;
WiFiManager          Wificfg_server;
WiFiManagerParameter custom_OPERADOR("OPERA","OPERADOR/COMERCIALIZADOR", ESP32_Para_CFG.ORCOM_char,19);
WiFiManagerParameter custom_CENTRO("CENTRO","CENTRO DE DISTRIBUCION", ESP32_Para_CFG.CDTRAFO_ID_char,11);
WiFiManagerParameter custom_ID("CAJA_ID", "CAJA_ID", ESP32_Para_CFG.IMYCO_ID_char, 5);
WiFiManagerParameter custom_CONTRATO("CONTRATO","CONTRATO/SUSCRIPTOR", ESP32_Para_CFG.CTOSUS_char,11);
WiFiManagerParameter custom_USUARIO("USUARIO","USUARIO", ESP32_Para_CFG.USERCAJA_char,29);
WiFiManagerParameter custom_IMEI("IMEI","NUMERO_IMEI", ESP32_Para_CFG.IMEI_char,15);
WiFiManagerParameter custom_TELEFONO("TELEFONO","NUMERO_TELEFONO", ESP32_Para_CFG.N_telefono,10);
WiFiManagerParameter custom_ID_GSM("ID_GSM","ID_GSM", ESP32_Para_CFG.GSM_char,19);


//Se encarga de conectar por primera vez y si esta en modo instalacion 
//Levantar un servidor WEB a travez de una access point
//Para parametrizar
//SSID
//PASSWORD
//NUMERO DE CAJA
bool Setup_WiFI(void)
{

  String ID_CONFIG_AP;
  

  read_flashESP32(Add_flash(TYPE_ID_CFG_MODO),
                  Data_size(TYPE_ID_CFG_MODO),
                  (char*)&ID_WIFIMOdo_status);
  read_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),
                  Data_size(TYPE_PARAMETROS_CFG),
                  (char*)&ESP32_Para_CFG);

  /////////MODO DE CONFIGURACION ///////////////////
  if(ID_WIFIMOdo_status==WIFI_ID_SCAN)
  {
      Wificfg_server.resetSettings();
      Wificfg_server.addParameter(&custom_OPERADOR);
      Wificfg_server.addParameter(&custom_CENTRO);
      Wificfg_server.addParameter(&custom_ID);
      Wificfg_server.addParameter(&custom_CONTRATO);
      Wificfg_server.addParameter(&custom_USUARIO);
      Wificfg_server.addParameter(&custom_IMEI);
      Wificfg_server.addParameter(&custom_TELEFONO);
      Wificfg_server.addParameter(&custom_ID_GSM);

      Wificfg_server.setConnectTimeout(20);
      //if (Wificfg_server.startConfigPortal("WIFI_MERCELIN","Cuatin05321")) 
      if(ESP32_Para_CFG.IMYCO_ID_char[0]=='\0')
      {
        #ifdef WIFI_DEBUG
        Serial.println("CAJA_SIN CODIGO");
        #endif
        ESP32_Para_CFG.IMYCO_ID_char[0]='S';
        ESP32_Para_CFG.IMYCO_ID_char[1]='N';
      }

      ID_CONFIG_AP="WIFI_";
      ID_CONFIG_AP+= String(ESP32_Para_CFG.IMYCO_ID_char);

      
      if (Wificfg_server.autoConnect(ID_CONFIG_AP.c_str(),"Cuatin05321")) 
      
      {
          Serial.println("connected...yeey :)");
          Temp=Wificfg_server.getWiFiSSID();
          strcpy (ESP32_Para_CFG.SSID_char, Temp.c_str());      
          Temp=Wificfg_server.getWiFiPass();
          strcpy (ESP32_Para_CFG.SSID_PASS_char,Temp.c_str());
      }  
      Serial.println("Imprimo_PARAMETOS...");
      strcpy (ESP32_Para_CFG.ORCOM_char, custom_OPERADOR.getValue());
      strcpy (ESP32_Para_CFG.CDTRAFO_ID_char, custom_CENTRO.getValue());
      strcpy (ESP32_Para_CFG.IMYCO_ID_char, custom_ID.getValue());
      strcpy (ESP32_Para_CFG.CTOSUS_char, custom_CONTRATO.getValue());
      strcpy (ESP32_Para_CFG.USERCAJA_char, custom_USUARIO.getValue());
    
      strcpy (ESP32_Para_CFG.IMEI_char, custom_IMEI.getValue());
      strcpy (ESP32_Para_CFG.N_telefono, custom_TELEFONO.getValue());
      strcpy (ESP32_Para_CFG.GSM_char, custom_ID_GSM.getValue()); 
      
      //ESP32_Para_CFG.MEN_LOCK_ID=EE_INI_VAL;
      ID_WIFIMOdo_status=WIFI_ID_SET;
      Serial.print("SSID_char=>");
      Serial.println(ESP32_Para_CFG.SSID_char);
      Serial.print("SSID_PASS_char=>");
      Serial.println(ESP32_Para_CFG.SSID_PASS_char);
      Serial.print("ORCOM_char=>");
      Serial.println(ESP32_Para_CFG.ORCOM_char);
      Serial.print("CDTRAFO_ID_char=>");
      Serial.println(ESP32_Para_CFG.CDTRAFO_ID_char);
      Serial.print("IMYCO_ID_char=>");
      Serial.println(ESP32_Para_CFG.IMYCO_ID_char);
      Serial.print("CTOSUS_char=>");
      Serial.println(ESP32_Para_CFG.CTOSUS_char);
      Serial.print("USERCAJA_char=>");
      Serial.println(ESP32_Para_CFG.USERCAJA_char);
      Serial.print("IMEI_char=>");
      Serial.println(ESP32_Para_CFG.IMEI_char);
      Serial.print("N_telefono=>");
      Serial.println(ESP32_Para_CFG.N_telefono);
      Serial.print("GSM_char=>");
      Serial.println(ESP32_Para_CFG.GSM_char);      
      //Serial.print("MEN_LOCK_ID=>");
      //Serial.println(ESP32_Para_CFG.MEN_LOCK_ID,HEX);

      store_flashESP32(Add_flash(TYPE_ID_CFG_MODO),Data_size(TYPE_ID_CFG_MODO),(char*)&ID_WIFIMOdo_status);
      store_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),Data_size(TYPE_PARAMETROS_CFG),(char*)&ESP32_Para_CFG);  
      delay(3000);
      ESP.restart();
  }
  
  Serial.println("**************************");
  Serial.print("IMYCO_ID_char=>");
  Serial.println(ESP32_Para_CFG.IMYCO_ID_char);
  Serial.println("**************************");
  

  return Connect_WIFI(ESP32_Para_CFG.SSID_char, ESP32_Para_CFG.SSID_PASS_char);
}





/*
WL_CONNECTED: assigned when connected to a WiFi network;
WL_NO_SHIELD: assigned when no WiFi shield is present;
WL_IDLE_STATUS: it is a temporary status assigned when 
                WiFi.begin() is called and remains active 
                until the number of attempts expires 
                (resulting in WL_CONNECT_FAILED) or a connection is 
                established (resulting in WL_CONNECTED);
WL_NO_SSID_AVAIL: assigned when no SSID are available;
WL_SCAN_COMPLETED: assigned when the scan networks is completed;
WL_CONNECT_FAILED: assigned when the connection fails for all the attempts;
WL_CONNECTION_LOST: assigned when the connection is lost;
WL_DISCONNECTED: assigned when disconnected from a network;*/

boolean Connect_WIFI(char* SSID_char,char* SSID_PASS_char)
{
  int WIFItimeout=0;

  WiFi.mode(WIFI_STA);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID_char);
  Serial.print("Pass ");
  Serial.println(SSID_PASS_char);
  //Mostrar_redes();
  if(!strcmp ("NULL",SSID_PASS_char))
  {
    Serial.println("Sin PASSWORDs");
    WiFi.begin(SSID_char);
  }
  else
  {
    Serial.println("Con PASSWORDs");
    WiFi.begin(SSID_char, SSID_PASS_char);
  }
 

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(10);
    Serial.print(".");
    WIFItimeout++;
    if(WIFItimeout==150)
      Serial.println(".");
    if(WIFItimeout==300)
    {
      Serial.println(".");
      Serial.println("WiFi NO connected_ini");
      return false;
    }   
  }
  Serial.println(".");
  Serial.println("WiFi connected");
  //#ifdef WIFIMQTT_DEBUG
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  //#endif 

  return true;
}


boolean Mostrar_redes(void)
{

  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) 
  {
    Serial.println("no networks found");
    return false;
  } 
  else 
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) 
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
      
    }
  }
  return true;
}

void Get_MAC_19(char * MAC)
{
  byte mac_byte[6];

  WiFi.macAddress(mac_byte);
  snprintf (MAC,18,"%X:%X:%X:%X:%X:%X",mac_byte[5],
                                       mac_byte[4],
                                       mac_byte[3],
                                       mac_byte[2],
                                       mac_byte[1],
                                       mac_byte[0]);
}