#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include "esp32-hal-cpu.h"
#include <LiquidCrystal_I2C.h>
#include <AT24CX.h>
#include "BT_user.h"
#include "ESP_MEM_IO.h"
#include "PzemModbus.h"
#include "Relaycmd.h"
#include "RTC_user.h" 
#include "SYS_Config.h"
#include "GPS_main.h"
#include "OTAcore.h"
#include "TCP_MQTT_CORE.h"
#include "WIFI_USER.h"
#include "MODEM_USER.h"

//#define  MAIN_DEBUG
#define USE_LCD

void RTCMen_ini_defecto(void);
void Peripherals_ini(void);


///  ///////VARIALBLE_GLOBALES ///////////////////////////////////////
int           Reset_ini_counter=0;

//Sistema de tiempo y eventos
long          time_muestreo_ref=0;
long          time_salvardatos_ref=0;
long          time_borrar_LCD_ref=0;
long          time_reattempt_com_ref=0;


//Medidor energia
PZEM_DataIO   PZEM_004T;
unsigned char PZEM_read_OK=0;
char          fist_time_muestreo=1;

//Sistema comunicacion
static char   modo_conexcion_st=MODO_COMUNICACION_DEFAULT;
char          MQTT_StatusConect;
boolean       fist_time_com=true;
boolean       Stop_reattempt_com=false;
char          Attempt_contador;
String        messageBT;


char          msg_LCD[16];

//////////////////////////// OBJETOS  ///////////////////////////////////////

LiquidCrystal_I2C   lcd(0x27,16,4);

//Sistema Memoria
All_flag       EE_flag_main;

int      EE_User_main;
#define SerialAT Serial2



void setup()
{
  SerialAT.begin(9600);   //Inicia puerto Serial comicacion(Moden SIM800)
  Serial.begin(9600);     //Inicia puerto Serial comicacion(PZEM, Programacion,DEBUG)
                          //GPS puerto Serial es inicializado en GSP_Setup() 
  //Serial.println(F_CPU);
  //Serial.println(getCpuFrequencyMhz());                       
  RTCMen_ini_defecto();   //Inicializa la memoria a sus valores por defecto, solo una vez.
  

 
  if(!OTA_Activado())
  {
    Peripherals_ini();//Activa todos los perifeciso (wifi,bluetooth,gsm,gps,lcd,bombillos led)
                      //y lee desde la EEPROM sus valores anteriores antes de apagar sistema. 
    GPS_Task_ini();   //Inicializa la tarea GPS_task in core 0.
    lcd.clear(); 
  }
  else
  {
    Serial.print("Modo OTA_detectado");
    if(Get_AS3_file()==1)
    {
      OTAsetup();
      execOTA();
    }
    else
    {
      Serial.print("Error AWS3 transfer ino file");
      ESP.restart();
    }
  }  
}

void loop() 
{
  /////////////////// sistema de conexcon TCP ///////////////////////////
  if(Stop_reattempt_com==false)
    MQTT_StatusConect= TCP_MQTT_manager(modo_conexcion_st);
  /*if(MQTT_StatusConect!=true)
    Attempt_contador++;
  if(MQTT_StatusConect==true)
     Attempt_contador=0;
  if(Attempt_contador==3)*/
  if(MQTT_StatusConect==TCP_MQTT_TIMEUP)
  {
    if(fist_time_com==true || time_validar_seg(&time_reattempt_com_ref,60)==1)
    {
      fist_time_com=false;
      Stop_reattempt_com=false;
      if(modo_conexcion_st!=MODO_WIFI)
      {
        Serial.println("Cambio a WIFI");
        modo_conexcion_st=MODO_WIFI;
      }
      else
      {
        Serial.println("Cambio a GSM");
        modo_conexcion_st=MODO_GSM;
      }
      Serial.println("SET MQTT");
      Setup_MQTT(modo_conexcion_st);      
    }
    else
    {
      Stop_reattempt_com=true;
    }

  }

  //////////////////// Imprime fecha y hora /////////////////////////////
  #ifdef USE_LCD
  if(time_validar_seg(&time_borrar_LCD_ref,59))
  {
    lcd.setCursor(0,0);
    lcd.print("                ");
  }

  Datetochar(msg_LCD);
  lcd.setCursor(0,0);//xy
  lcd.print(msg_LCD);

  Timetochar(msg_LCD);
  lcd.setCursor(9,0);
  lcd.print(msg_LCD);
  #endif

  ///////////////// Lee GPS y imprime medida /////////////////////////////
  if(time_validar_seg(&time_muestreo_ref,10)==1 || fist_time_muestreo==1) 
  { 
    GPS_RUN();
    
    if(fist_time_muestreo)
      delay(3000);
    fist_time_muestreo=0;
    PZEM_read_OK=PZEM_all_measurements(&PZEM_004T);  
    if(PZEM_read_OK)
    { 
      PZEM_004T.energy_alCorte=Energia_Corte_Run(PZEM_004T.energy_user);

      #ifdef USE_LCD
      lcd.setCursor(0,1);
      lcd.print("                ");      
      snprintf (msg_LCD,15,"E.A:%4.2f KWh",PZEM_004T.energy_user);
      lcd.setCursor(0,1);
      lcd.print(msg_LCD);

      lcd.setCursor(0,2);
      lcd.print("                "); 
      snprintf (msg_LCD,15,"C.I:%4.2f A",PZEM_004T.current);
      lcd.setCursor(0,2);
      lcd.printstr(msg_LCD);

      lcd.setCursor(0,3);
      lcd.print("                ");
      snprintf (msg_LCD,15,"V.I:%4.2f V",PZEM_004T.voltage);
      lcd.setCursor(0,3);
      lcd.printstr(msg_LCD);
      #endif
      if(MQTT_StatusConect==true)
      {
        MQTT_publish_PZEM_DOSI_PRE(PZEM_004T,modo_conexcion_st);
        MQTT_publish_GPS(modo_conexcion_st);
      }

      Planificador_tareas(PZEM_004T);
    }
    else
    {
      Serial.println("Error al leer PEZM:main");
    }
    ///////////////// Almacena toda informacion /////////////////////////////
    if(time_validar_seg(&time_salvardatos_ref,35)==1)  
    {
        //Serial.println("Store all Data!");
        Almacenar_GPS_Data();
        Almacenar_PZEM_Data();          //Almacemo parametros de error. 
        Almacenar_Configuracion_Data(); //
        //Serial.println("===END_Store all Data!===");
    }
  }

  if(BT_pair()==true)
  {
    BT_CMD_RUM(BT_CMD_incomming(messageBT));
  }
 
  if(MQTT_StatusConect==true)
  {
    MQTT_LED_CONNECTED_OK();
  }
  else
  {
    MQTT_LED(0);
  }
}

void RTCMen_ini_defecto(void)
{
  char fecha_inicial[17];
  LED_Setup();             //No tiene datos de inicializacion en EEPROM
  Setup_ESP_flash();
  read_flashI2C(TYPE_FLAGS,(char*)&EE_flag_main);
  #ifdef MAIN_DEBUG
  Serial.println("EEPROM_Memoria_ini_flag:EEPROM_INI_VAL");
  Serial.print(EE_flag_main.Memoria_ini_flag,HEX);
  Serial.print(":");
  Serial.println(EE_INI_VAL,HEX);
  #endif
  if(EE_flag_main.Memoria_ini_flag!=EE_INI_VAL)
  {
    Serial.println("!!!inicializando EEPROM Y HORA!!!");
    ini_I2C_Data(EE_INI_VAL);
    Setup_RTC();             //No tiene datos de inicializacion en EEPROM
    Datetochar(fecha_inicial);
    Serial.print("Fecha_fabrica=>");
    Serial.println(fecha_inicial);
    store_flashESP32(Add_flash(TYPE_FECHA_FAB),Data_size(TYPE_FECHA_FAB),(char*)&fecha_inicial);  
 
  }
  Read_index_reinicio(&Reset_ini_counter);
  Serial.print("Contador Reinicios=>");
  Serial.println(Reset_ini_counter);
  Reset_ini_counter++;
  Store_index_reinicio(Reset_ini_counter);
  Serial.print("Hora del sistema=>");
  printDate();

}

void Peripherals_ini(void)
{
  unsigned char        ID_WIFIMOdo;

  #ifdef USE_LCD
  lcd.init();  
  lcd.backlight();
  lcd.clear();
  lcd.home();
  lcd.print("Ini sys..");
  #endif

  
  Setup_relay();           //No tiene datos de inicializacion en EEPROM
  Setup_PZEM();            //Se lee en EEPROm los parametros de inicializacion.
  GPS_Setup();             //Se lee en EEPROM los parametros de inicializacion.
  Tareas_Setup();          //Se lee en EEPROM los parametros de inicializacion para  las tareas de: -Limitacion 
                                                                                                 // -Dosificacion 
                                                                                                 // -Prepago.
                                                                                                 // -User
  
  switch (modo_conexcion_st) 
  {
    case MODO_WIFI:
      read_flashESP32(Add_flash(TYPE_ID_CFG_MODO),
                      Data_size(TYPE_ID_CFG_MODO),
                      (char*)&ID_WIFIMOdo);
      if(ID_WIFIMOdo==WIFI_ID_SCAN)
      {
        #ifdef USE_LCD
        lcd.setCursor(0,0);
        lcd.clear();
        lcd.print("Modo web config");
        #endif
        Setup_WiFI();
      }
      else
      {
        #ifdef USE_LCD
        lcd.setCursor(0,0);
        lcd.clear();
        lcd.print("Inicio WIFI..");
        #endif
      }
    break;
    case MODO_GSM:  //intenta por GSM si no conecta usa WIFI por defecto
      #ifdef USE_LCD
      lcd.setCursor(0,0);
      lcd.print("Inicio GSM..");
      #endif
      //if(Setup_GSM()==true)
      //  GSM_set_gprs_link();
    break;
  }
  Setup_MQTT(modo_conexcion_st);
  lcd.clear();
  Setup_BT();
}




