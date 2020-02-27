#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


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
void Task_ini(void);



///  ///////VARIALBLE_GLOBALES ///////////////////////////////////////
int Reset_ini_counter=0;

//Sistema de tiempo y eventos
//long time_public,time_public_ref;
long          time_muestreo_ref=0;
long          time_salvardatos_ref=0;
long          time_borrar_LCD_ref=0;


//Medidor energia
PZEM_DataIO   PZEM_004T;
unsigned char PZEM_read_OK=0;
char          fist_time_muestreo=1;

//Sistema comunicacion
char   MQTT_StatusConect;
char   MODO_CONFIGURACION_CAJA;


char   msg_LCD[16];
static char modo_conexcion_st;
char   Attempt_contador;


String messageBT;

//test repositot
//////////////////////////// OBJETOS  ///////////////////////////////////////
TaskHandle_t        GSM_Task;
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
  RTCMen_ini_defecto();   //Inicializa la memoria a sus valores por defecto, solo una vez.
  

 
  if(!OTA_Activado())
  {
    Peripherals_ini(); //Activa todos los perifeciso (wifi,bluetooth,gsm,gps,lcd,bombillos led)
                        //y lee desde la EEPROM sus valores anteriores antes de apagar sistema. 
    Task_ini();         //Inicializa la tarea GPS_task in core 0.
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
  MQTT_StatusConect= TCP_MQTT_manager(modo_conexcion_st);
  if(MQTT_StatusConect!=true)
    Attempt_contador++;
  if(MQTT_StatusConect==true)
     Attempt_contador=0;
  if(Attempt_contador==20)
  {
    if(modo_conexcion_st!=MODO_WIFI)
    {
      Serial.print("Cambio a WIFI");
      modo_conexcion_st=MODO_WIFI;
    }
    else
    {
      Serial.print("Cambio a GSM");
      modo_conexcion_st=MODO_GSM;
    }
    Serial.print("SET MQTT");
    Setup_MQTT(modo_conexcion_st);
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
  if(time_validar_seg(&time_muestreo_ref,5)==1 || fist_time_muestreo==1) 
  { 
    GPS_RUN();
    MQTT_publish_GPS();
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
      MQTT_publish_PZEM_DOSI_PRE(PZEM_004T);
      
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
  }
  Read_index_reinicio(&Reset_ini_counter);
  Serial.print("Contador Reinicios=>");
  Serial.println(Reset_ini_counter);
  Reset_ini_counter++;
  Store_index_reinicio(Reset_ini_counter);

}

void Peripherals_ini(void)
{
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
  //prioridad   
  modo_conexcion_st=MODO_WIFI;
  //modo_conexcion_st=MODO_GSM;

  switch (modo_conexcion_st) 
  {
    case MODO_WIFI:
      #ifdef USE_LCD
      lcd.setCursor(0,0);
      lcd.clear();
      lcd.print("Inicio WIFI..");
      #endif
      if(!Setup_WiFI())
        Setup_WiFI();
    break;
    case MODO_GSM:  //intenta por GSM si no conecta usa WIFI por defecto
      #ifdef USE_LCD
      lcd.setCursor(0,0);
      lcd.print("Inicio GSM..");
      #endif
      if(Setup_GSM())
        GSM_set_gprs_link();
    break;
  }
  Setup_MQTT(modo_conexcion_st);
  lcd.clear();
  Setup_BT();
}

void Task_ini(void)
{
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
   xTaskCreatePinnedToCore(
      GSM_READ,  
      "Task1", 
      10000,   
      NULL,   
      0,   
      &GSM_Task,  
      0);  
  delay(500);
}

void GSM_READ( void * pvParameters )
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;)
  {
     GPS_Maintenice();
  } 
}

void Planificador_tareas(PZEM_DataIO PZEM_004T_Datos)
{
   read_flashI2C(TYPE_USER_INFO,     (char*)&EE_User_main);

   switch (EE_User_main) 
  {
    case EST_NORMAL:
    case EST_LIMITADO:
    case EST_PENALIZADO:
      Limitacion_Run(PZEM_004T.current);
    break;
    case EST_DOFICACION:
      Docificacion_Run(PZEM_004T.energy_user);
    break; 
    case EST_PREPAGO:
      Prepago_Run(PZEM_004T.energy_user);
    break;

  }
}
