#include <AT24CX.h>
#include <EEPROM.h> //512 bytes (microcontrolador)
#include <Wire.h>
#include "ESP_MEM_IO.h"
#include "SYS_Config.h"
#include "WIFI_USER.h"

//#define MEN_DEBUG

char            index_overdrive[9];
char            intermitente_wifi_flag=0;

All_flag        EEROM_flagvar; 
Medidor_info    EEROM_Medidor;
Limitador_info  EEROM_Limitador;
Prepago_info    EEROM_Prepago;
Dosifi_info     EEROM_Dosifi;
GPS_info        EEROM_GPS;
int             EEROM_Estado_User;
OTA_info        EEROM_Estado_OTA;

Parametros_CFG  EEROMESP32_Para_CFG;
char            ESP_WIFI_CFG_MODO;
AT24C32 I2CMEN;

void  Read_index_reinicio(int* pReinicio_index)
{
  *pReinicio_index=I2CMEN.readInt(0);
}

void Store_index_reinicio(int Reinicio_index)
{
  I2CMEN.writeInt(0,Reinicio_index);
}

void  ini_I2C_Data(char            flag_var_randon)
{
  unsigned int END_ADD;
  int i;
  
  EEROM_flagvar.Memoria_ini_flag=flag_var_randon;        //Indica si la memoria no se ha inicializado a los valores por defecto
  EEROM_flagvar.Actualizar_ref_KW_flag=1;                //Solo se graba la primera vez, marca el inicio el consumo del usuario

  EEROM_Medidor.KW_Inicio_Medida_ref=0;          //Valor unicio del consumo usuario
  EEROM_Medidor.KW_Inicio_Corte_ref=0;           //Acumulado a la fecha de corte
  EEROM_Medidor.KM_delta_error=0;                //Error actual por offset
 
  EEROM_Limitador.A_limitacion=80;               //Limitacion por defecto
  EEROM_Limitador.Tiempo_reconeccion_seg=300;    //Limitacion a 5 minutos
  
  EEROM_Prepago.Fecha_recarga[0]='X';     //Sin configurar
  EEROM_Prepago.KW_Pre_Inicio_ref=0;                    
  EEROM_Prepago.KM_Consumo_total=0;              
  EEROM_Prepago.KM_Saldo_anterior=0;
  EEROM_Prepago.KM_Saldo=0;
  EEROM_Prepago.KM_Recarga=0;

  EEROM_Dosifi.KW_Cuota_Dia=0;
  EEROM_Dosifi.KM_Disponible=0;
  EEROM_Dosifi.KW_Dofi_Inicio_ref=0;
  EEROM_Dosifi.KW_Consumo_total=0;
  EEROM_Dosifi.Horas_estimadas=0;

  EEROM_Estado_User=EST_NORMAL;

  EEROM_GPS.Satelites_found=0;
  EEROM_GPS.Latitud_leida=0;
  EEROM_GPS.Longitud_leida=0;
  EEROM_GPS.Latitud_ultima_conocida=0;
  EEROM_GPS.Longitud_ultima_conocida=0;
  EEROM_GPS.failedChecksum_data=0;
  EEROM_GPS.passedChecksum_data=0;
  EEROM_GPS.charsProcessed_data=0;
  EEROM_GPS.Fix_data=0;

  //Los demas valores no importan, simpre se reinician en cada OTA
  EEROM_Estado_OTA.OTA_Activado=0;  

  //Reinicio index de memoria
  for(i=0;i<9;i++) //0-8 index
  {
    index_overdrive[i]=0;
    I2CMEN.writeInt(sizeof(int)*i,0);
    #ifdef MEN_DEBUG
    Serial.print("Contador i(");
    Serial.print(i);
    Serial.print(")=>");
    Serial.println( I2CMEN.readInt(sizeof(int)*i));
    #endif
  }
   
  store_flashI2C(TYPE_FLAGS,         (char*)&EEROM_flagvar);
  store_flashI2C(TYPE_MEDIDOR_INFO,  (char*)&EEROM_Medidor);
  store_flashI2C(TYPE_LIMITADOR_INFO,(char*)&EEROM_Limitador);
  store_flashI2C(TYPE_PREPAGO_INFO,  (char*)&EEROM_Prepago);
  store_flashI2C(TYPE_DOFI_INFO,     (char*)&EEROM_Dosifi);
  store_flashI2C(TYPE_USER_INFO,     (char*)&EEROM_Estado_User);
  store_flashI2C(TYPE_GPS_INFO,      (char*)&EEROM_GPS);
  store_flashI2C(TYPE_OTA_INFO,      (char*)&EEROM_Estado_OTA);

  END_ADD=DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_PREPAGO_INFO*DATA_INDEX_OVERDRIVE
             +TAM_DOFI_INFO*DATA_INDEX_OVERDRIVE
             +TAM_USER_INFO*DATA_INDEX_OVERDRIVE
             +TAM_GPS_INFO*DATA_INDEX_OVERDRIVE
             +TAM_OTA_INFO*DATA_INDEX_OVERDRIVE;
             
  //#ifdef MEN_DEBUG            
  Serial.print("32752=>END_ADD_SPI=>");
  Serial.println(END_ADD);
  //#endif  
  
  //Activar para usar opcion configurar WIFI y parametros por WEB
  //ESP_WIFI_CFG_MODO=WIFI_ID_SCAN;  
  
  //Activar para usar valores por defecto (De todos los parametros)
  ESP_WIFI_CFG_MODO=WIFI_ID_DEFAULT;  

  //Iniciacilizacion men parametros
  for(i=0;i<20;i++)  
  {
    EEROMESP32_Para_CFG.SSID_char[i]='\0';
    EEROMESP32_Para_CFG.SSID_PASS_char[i]='\0';
    EEROMESP32_Para_CFG.ORCOM_char[i]='\0';
    EEROMESP32_Para_CFG.GSM_char[i]='\0';
  }
  for(i=0;i<12;i++)  
  {
     EEROMESP32_Para_CFG.CDTRAFO_ID_char[i]='\0';
     EEROMESP32_Para_CFG.CTOSUS_char[i]='\0';
  }
  for(i=0;i<6;i++)  
  {
    EEROMESP32_Para_CFG.IMYCO_ID_char[i]='\0';
  }
  for(i=0;i<30;i++) //0-8 index
  {
     EEROMESP32_Para_CFG.USERCAJA_char[i]='\0';
  }

  for(i=0;i<16;i++) //0-8 index
  {
     EEROMESP32_Para_CFG.IMEI_char[i]='\0';
  }

  for(i=0;i<11;i++) //0-8 index
  {
     EEROMESP32_Para_CFG.N_telefono[i]='\0';
  }

  EEROMESP32_Para_CFG.MEN_LOCK_ID=0xFF;

  if(ESP_WIFI_CFG_MODO==WIFI_ID_DEFAULT)
  {
     //BYTES 157
    Serial.println("Set_atributos x defecto");
    snprintf (EEROMESP32_Para_CFG.IMYCO_ID_char,5,"0241");
    snprintf (EEROMESP32_Para_CFG.SSID_char,19,"Tecno5");
    snprintf (EEROMESP32_Para_CFG.SSID_PASS_char,19,"Cuatin05321");
    snprintf (EEROMESP32_Para_CFG.ORCOM_char,19,"EMCALI");
    snprintf (EEROMESP32_Para_CFG.CDTRAFO_ID_char,11,"TRAFO12343");
    snprintf (EEROMESP32_Para_CFG.CTOSUS_char,11,"#3433246");
    snprintf (EEROMESP32_Para_CFG.USERCAJA_char,29,"NICOLAS TESLA");
    snprintf (EEROMESP32_Para_CFG.IMEI_char,15,"#1232123451776");
    snprintf (EEROMESP32_Para_CFG.N_telefono,11,"#3023144320");
    snprintf (EEROMESP32_Para_CFG.GSM_char,20,"#1234567890876543212");
    snprintf (EEROMESP32_Para_CFG.DIRECCION_char,29,"Cl 53 # 12-39");
    
    Serial.print("SSID_char=>");
    Serial.println(EEROMESP32_Para_CFG.SSID_char);
    Serial.print("SSID_PASS_char=>");
    Serial.println(EEROMESP32_Para_CFG.SSID_PASS_char);
    Serial.print("ORCOM_char=>");
    Serial.println(EEROMESP32_Para_CFG.ORCOM_char);
    Serial.print("CDTRAFO_ID_char=>");
    Serial.println(EEROMESP32_Para_CFG.CDTRAFO_ID_char);
    Serial.print("IMYCO_ID_char=>");
    Serial.println(EEROMESP32_Para_CFG.IMYCO_ID_char);
    Serial.print("CTOSUS_char=>");
    Serial.println(EEROMESP32_Para_CFG.CTOSUS_char);
    Serial.print("USERCAJA_char=>");
    Serial.println(EEROMESP32_Para_CFG.USERCAJA_char);
    Serial.print("IMEI_char=>");
    Serial.println(EEROMESP32_Para_CFG.IMEI_char);
    Serial.print("N_telefono=>");
    Serial.println(EEROMESP32_Para_CFG.N_telefono);
    Serial.print("GSM_char=>");
    Serial.println(EEROMESP32_Para_CFG.GSM_char); 
    ESP_WIFI_CFG_MODO=WIFI_ID_SET;
  }
  //grabo en la EEPROM del ESP 32
  store_flashESP32(Add_flash(TYPE_ID_CFG_MODO),Data_size(TYPE_ID_CFG_MODO),(char*)&ESP_WIFI_CFG_MODO);
  store_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),Data_size(TYPE_PARAMETROS_CFG),(char*)&EEROMESP32_Para_CFG);  
  
  END_ADD=TAM_ID_CFG_MODO+TAM_PARAMETROS_CFG+25;
  Serial.print("512=>END_ADD_EErom_ESP32=>");
  Serial.println(END_ADD);
 
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
void  store_flashESP32(int add_start,int size_data,char* byte_data)
{
  int end_add=size_data+add_start;
  int store_add;

  for(store_add=add_start;store_add<end_add;store_add++)
  {
      EEPROM.write(store_add,byte_data[store_add-add_start]);
      EEPROM.commit();
  }
  #ifdef MEN_DEBUG
  Serial.print("bytes EEPROM_store:");
  Serial.println(size_data); 
  #endif
}

void  read_flashESP32(int add_start,int size_data,char* byte_data)
{
   int end_add=size_data+add_start;
   int read_add;
   
   for(read_add=add_start;read_add<end_add;read_add++)
   {
     byte_data[read_add-add_start]=EEPROM.read(read_add);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////



void  store_flashI2C(int Data_type,char* Info)
{
  int  Data_index=0;

  //escribo en direccion actual
  I2CMEN.write(Add_flash(Data_type),(byte*)Info,Data_size(Data_type));
  //Aumento uno para no sobre escribir
  if(Data_type>TYPE_FLAGS)
  {
    Data_index=I2CMEN.readInt(sizeof(int)*Data_type);
    Data_index++;
    if(Data_index==DATA_INDEX_OVERDRIVE)
    {
      #ifdef MEN_DEBUG
      Serial.println("tope_store");
      #endif
      Data_index=0;
      index_overdrive[Data_type]=1; // indica que estoy en 0 pero hubo posicion MAX_DATA_INDEX contiene datos.
    }
    else
    {
      index_overdrive[Data_type]=0;// indica que estoy en > 0 
    }
    I2CMEN.writeInt(sizeof(int)*Data_type,Data_index); 
  }
  #ifdef MEN_DEBUG
  Serial.print("bytes EEPROM_store:");
  Serial.println(Data_size(Data_type)); 
  #endif
}


void  read_flashI2C(int Data_type,char* Info)
{
  int  Data_index=0;

  
  //INDEX=INDEX-1. 
  //caso read_flashI2C+read_flashI2C (solo una vez se debe de restar)
  if(Data_type>TYPE_FLAGS)
  {
    if( index_overdrive[Data_type]==1)
    {
       I2CMEN.writeInt(sizeof(int)*Data_type, DATA_INDEX_OVERDRIVE-1); //index =4
    }
    else
    {
      Data_index=I2CMEN.readInt(sizeof(int)*Data_type);
      if(Data_index>0) //Si significa que index es mayor que cero
        Data_index--;
      I2CMEN.writeInt(sizeof(int)*Data_type,Data_index); //actualizo index
    }
  }
  //LEO EN INDEX POS.
  I2CMEN.read(Add_flash(Data_type),(byte*)Info ,Data_size(Data_type));  

  //coloco a i en posicion vacia denuevo
  if(Data_type>TYPE_FLAGS)
  {
   Data_index=I2CMEN.readInt(sizeof(int)*Data_type);
    Data_index++;
    if(Data_index==DATA_INDEX_OVERDRIVE)
    {
       #ifdef MEN_DEBUG
       Serial.println("tope_read");
       #endif
       Data_index=0;
       index_overdrive[Data_type]=1;
    }
    else
    {
       index_overdrive[Data_type]=0;// indica que estoy en > 0 
    }
    I2CMEN.writeInt(sizeof(int)*Data_type,Data_index); 
  } 
  #ifdef MEN_DEBUG
  Serial.print("bytes EEPROM_read:");
  Serial.println(Data_size(Data_type));
  #endif   
}

//Solo tiene 512 bytes
void Setup_ESP_flash(void)
{
  int tam=512;
  
   if (!EEPROM.begin(tam))
   { 
      Serial.print("Error al inicializar EEPRON"); 
     // return 0;   
   }
}

//sizeof(int)*0   reinicio_index, 
//sizeof(int)*1   xxxxxxxxxx, 
//sizeof(int)*2   Medi_index,
//sizeof(int)*3   limi_index,
//sizeof(int)*4   pre_index,
//sizeof(int)*5   dofi_index
//sizeof(int)*6   user_index
//sizeof(int)*7   gps_index
//sizeof(int)*8   ota_index
//sizeof(int)*9   TYPE_FLAGS-...
int Add_flash(int type)
{

  int result;
  int  Add_data_index=I2CMEN.readInt(sizeof(int)*type);  //0-7

  #ifdef MEN_DEBUG
  Serial.print("index:");
  Serial.println(Add_data_index);
  #endif
  switch (type) 
  {
    case TYPE_FLAGS:
      result= DATA_ADD_OFFSET;
    break;
    case TYPE_MEDIDOR_INFO:
      result= DATA_ADD_OFFSET
             +TAM_FLAGS 
             +TAM_MEDIDOR_INFO*Add_data_index;
    break;
    case TYPE_LIMITADOR_INFO:
      result= DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*Add_data_index;
    break;
    case TYPE_PREPAGO_INFO:
      result= DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_PREPAGO_INFO*Add_data_index;
    break;
    case TYPE_DOFI_INFO:
         result= DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_PREPAGO_INFO*DATA_INDEX_OVERDRIVE
             +TAM_DOFI_INFO*Add_data_index;
    break;
    case TYPE_USER_INFO:
         result= DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_PREPAGO_INFO*DATA_INDEX_OVERDRIVE
             +TAM_DOFI_INFO*DATA_INDEX_OVERDRIVE
             +TAM_USER_INFO*Add_data_index;
    break;  
    case TYPE_GPS_INFO:
         result= DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_PREPAGO_INFO*DATA_INDEX_OVERDRIVE
             +TAM_DOFI_INFO*DATA_INDEX_OVERDRIVE
             +TAM_USER_INFO*DATA_INDEX_OVERDRIVE
             +TAM_GPS_INFO*Add_data_index;
    break;
    case TYPE_OTA_INFO:
         result= DATA_ADD_OFFSET
             +TAM_FLAGS
             +TAM_MEDIDOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_LIMITADOR_INFO*DATA_INDEX_OVERDRIVE
             +TAM_PREPAGO_INFO*DATA_INDEX_OVERDRIVE
             +TAM_DOFI_INFO*DATA_INDEX_OVERDRIVE
             +TAM_USER_INFO*DATA_INDEX_OVERDRIVE
             +TAM_GPS_INFO*DATA_INDEX_OVERDRIVE
             +TAM_OTA_INFO*Add_data_index;
    break;
    //ESP32

    case TYPE_ID_CFG_MODO: 
        result=0; 
    break;
    case TYPE_PARAMETROS_CFG: 
        result=1; 
    break;
    case TYPE_FECHA_FAB:
        result=TAM_PARAMETROS_CFG+2;
    break;
    default:
     result=0;
    break;
  }
  #ifdef MEN_DEBUG
  Serial.print("ADD:");
  Serial.println(result);
  #endif
  return result;
}


int Data_size(int type)
{
  int result;
  
  switch (type) 
  {
    case TYPE_FLAGS:
       result= TAM_FLAGS;
    break;
    case TYPE_MEDIDOR_INFO:
       result=  TAM_MEDIDOR_INFO;
    break;
    case TYPE_LIMITADOR_INFO:
       result=  TAM_LIMITADOR_INFO;
    break;
    case TYPE_PREPAGO_INFO:
       result=  TAM_PREPAGO_INFO;
    break;
    case TYPE_DOFI_INFO:
       result=  TAM_DOFI_INFO;
    break;
    case TYPE_USER_INFO:
       result=  TAM_USER_INFO;
    break;
    case TYPE_GPS_INFO:
       result=  TAM_GPS_INFO;
    break;
    case TYPE_OTA_INFO:
       result=  TAM_OTA_INFO;
    break;    
    //ESP32
    case TYPE_ID_CFG_MODO:
       result=  TAM_ID_CFG_MODO;
    break;  
     case TYPE_PARAMETROS_CFG:
       result=  TAM_PARAMETROS_CFG;
    break;           
    default:
     result=0;
    break;
  }
  return result;
}




void LED_Setup(void)
{
  pinMode(MQQT_LED,OUTPUT);
  digitalWrite(MQQT_LED, HIGH);
}

void MQTT_LED(char action)
{
  if(action) //Prendido
     digitalWrite(MQQT_LED,LOW ); 
  else
     digitalWrite(MQQT_LED,HIGH); 
}

void MQTT_LED_CONNECTED_OK(void)
{
    if(intermitente_wifi_flag==0)
    {
      digitalWrite(MQQT_LED, LOW); 
      intermitente_wifi_flag=1; //Prendo
    }
    else
    {
      digitalWrite(MQQT_LED, HIGH); 
      intermitente_wifi_flag=0; //Apago
    }
    delay(300);
}
