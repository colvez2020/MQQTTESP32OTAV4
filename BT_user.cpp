#include "BluetoothSerial.h"
#include "ESP_MEM_IO.h"
#include "WIFI_USER.h"
#include "BT_user.h"
#include "Relaycmd.h"
#include "MODEM_USER.h"

Parametros_CFG       ESP32_CFG;

char   ESP_WIFI_CFG;


#define DT_DEBUG

//Bluetooth 
BluetoothSerial SerialBT;
static char  BT_incomingChar;
static char   BT_msg[41];


bool BT_pair(void)
{
 return SerialBT.hasClient();                        
}




void Setup_BT(void)
{

  String ID_BLUETOOTH;

  read_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),
                  Data_size(TYPE_PARAMETROS_CFG),
                  (char*)&ESP32_CFG);

  if(ESP32_CFG.IMYCO_ID_char[0]=='\0')
  {
    #ifdef DT_DEBUG
    Serial.println("CAJA_SIN CODIGO");
    #endif
    ESP32_CFG.IMYCO_ID_char[0]='S';
    ESP32_CFG.IMYCO_ID_char[1]='N';
  }

  ID_BLUETOOTH="BT_";
  ID_BLUETOOTH+= String(ESP32_CFG.IMYCO_ID_char);
  
  if(!SerialBT.begin(ID_BLUETOOTH.c_str()))
  {
    Serial.println("Bluetooth ERROR");
  }
  else
  {
    // SerialBT.enableSSP();
    // SerialBT.setPin(pin);
     Serial.println("Bluetooth OK");

  }
}

char BT_CMD_incomming(String messageBT)
{
  if (SerialBT.available())
  {
    BT_incomingChar = SerialBT.read();
    if (BT_incomingChar != '\n')
    {
      messageBT += String(BT_incomingChar); 
    }
    Serial.print(messageBT);  
  }
  // Check received message and control output accordingly
  if (BT_incomingChar =='C' || BT_incomingChar =='W' || BT_incomingChar =='1' || BT_incomingChar =='0' || BT_incomingChar =='S' )
  {
    #ifdef DT_DEBUG
    Serial.println("Llego_CMD_BT");
    #endif
    return BT_incomingChar;
  }
  return 0;
}


void BT_CMD_RUM(char CMD)
{
  switch(CMD)
  {
    case 'C':
       Estatus_ESP32CFG();
    break;
    case 'W':
      ESP_WIFI_CFG=WIFI_ID_SCAN;
      store_flashESP32(Add_flash(TYPE_ID_CFG_MODO),Data_size(TYPE_ID_CFG_MODO),(char*)&ESP_WIFI_CFG);
      ESP.restart();
    break;
    case '1':
      Relay_Action(1);
    break;
    case '0':
      Relay_Action(0);
    break;   
    case 'S':
      GET_RSSID_20(BT_msg);
      SerialBT.println(BT_msg);
    break;
  }  
}



void Estatus_ESP32CFG(void)
{
  snprintf (BT_msg,40," ");

  snprintf (BT_msg,40,"WI: %s",ESP32_CFG.SSID_char );
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"WIPSW: %s",ESP32_CFG.SSID_PASS_char );
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"OC: %s",ESP32_CFG.ORCOM_char);
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"CD: %s",ESP32_CFG.CDTRAFO_ID_char);
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"ID: %s",ESP32_CFG.IMYCO_ID_char);
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"USR: %s",ESP32_CFG.USERCAJA_char );
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"IME: %s",ESP32_CFG.IMEI_char);
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"T#: %s",ESP32_CFG.N_telefono);
  SerialBT.println(BT_msg);
  snprintf (BT_msg,40,"GPS_ID: %s",ESP32_CFG.GSM_char);
  SerialBT.println(BT_msg);
}

//buffer del bluetooh 32 bytes
/*void BT_send_data_32(char * mensaje)
{
  char BT_messeger32[32]; //0-31 32 bytes
  int mensaje_longitud=strlen(mensaje);
  int mensaje_total_index, mensaje32_index;

  Serial.print("longitud_a_mandar=");
  Serial.println(mensaje_longitud);


  for(mensaje_total_index=0;mensaje_total_index<mensaje_longitud;mensaje_total_index++)
  {
    mensaje32_index=0;
    do{
   
        Serial.println("test");
        BT_messeger32[mensaje32_index]=mensaje[mensaje_total_index+mensaje32_index];
        mensaje32_index++;
        if(mensaje_total_index+mensaje32_index>mensaje_longitud)
        {
          mensaje_total_index=mensaje_longitud;
          Serial.println("FIN");
          break;
        }

    } while (mensaje32_index<32);

    if(mensaje_total_index!=mensaje_longitud)       //No he llegado a fin
    {
      if(mensaje_total_index+32<mensaje_longitud)   //No e desvordo
        mensaje_total_index=mensaje_total_index+32; //Aumento para leer los siguientes 32 bytes
    }  
    SerialBT.print(BT_messeger32);
    Serial.print("Print=>");
    Serial.println(BT_messeger32); 

  }
  Serial.println("FINT"); 
}*/