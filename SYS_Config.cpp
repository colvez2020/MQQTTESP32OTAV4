#include <Arduino.h>
#include <stdlib.h>  
#include "ESP_MEM_IO.h"
#include "SYS_Config.h"
#include "Relaycmd.h"
#include "Rtc_user.h"
#include "TCP_MQTT_CORE.h"

//#define SYSCONFIG_DEBUG

char           Actualizar_estado_user_flag=1;
char           Actualizar_KW_Pre_flag=1;
long           time_limitador,time_limitador_ref;
long           Dofi_backup,Dofi_backup_ref;


Limitador_info Lim_Config_Data;
Medidor_info   Medidor_Config_Data;
Prepago_info   Prepago_Config_Data;
Dosifi_info    Dosifi_Config_Data;
int            User_Config_Data;
char           User_relay_estado=EST_RELAY_CERRADO; //por defecto esta cerrado



//////////////////////COSAS USER//////////////////////
int return_User_Config_Data(void)
{
  return User_Config_Data;
}

void return_User_Config_char_20(char* User_config_char)
{
  switch (User_Config_Data) 
  {
    case EST_NORMAL:
      snprintf (User_config_char,19,"NORMALIZADO");
    break;
    case EST_LIMITADO:
      snprintf (User_config_char,19,"LIMITADO");
    break;
    case EST_PENALIZADO:
      snprintf (User_config_char,19,"PENALIZADO");
    break;
    case EST_DOFICACION:
      snprintf (User_config_char,19,"CON_DOSIFICACION");
    break;
    case EST_PREPAGO:
      snprintf (User_config_char,19,"CON_PREPAGO");
    break;
    case EST_SUS_NORMA:
      snprintf (User_config_char,19,"EN_SUS_NORMAL");
    break;
    case EST_SUS_LIMI:
      snprintf (User_config_char,19,"EN_SUS_LIMITADO");
    break;
    case EST_SUS_DOSI:
      snprintf (User_config_char,19,"EN_SUS_DOSIFICADO");
    break;
    case EST_SUS_PREPA:
      snprintf (User_config_char,19,"EN_SUS_PREPAGO");
    break;
  }
}

void return_User_relay_estado(boolean * Relay_estado )
{
  switch (User_relay_estado) 
  {
    case EST_RELAY_ABIERTO:
      *Relay_estado=false;
    break;
    case EST_RELAY_CERRADO:
      *Relay_estado=true;
    break;
  }
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////


///////////////////CORTE ENERGIA//////////////////////

float Energia_Corte_Run(float Energia_leida)
{

   if(time_equal(-1,-1,1,0,0)) //Todos lo primeros de cada mes
   {
      read_flashI2C(TYPE_MEDIDOR_INFO,  (char*)&Medidor_Config_Data);
      Medidor_Config_Data.KW_Inicio_Corte_ref=Energia_leida;
      store_flashI2C(TYPE_MEDIDOR_INFO,  (char*)&Medidor_Config_Data);
   }
   if(Medidor_Config_Data.KW_Inicio_Corte_ref!=0)
   {
      return (Energia_leida-Medidor_Config_Data.KW_Inicio_Corte_ref);
   }
    return -1.0;
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////


///////////////////LIMITACION/////////////////////////
void Limitacion_Setup(float parametro)
{
  //int Corriente_lim_int=0;
  //int Tiempo_int=0;


  User_Config_Data=EST_LIMITADO;
  if(parametro>0.0)
    Lim_Config_Data.A_limitacion=parametro;
  Lim_Config_Data.Tiempo_reconeccion_seg=300; //5 minutos por defecto
  
  time_limitador_ref=0;  //Se reinicia el contrador.
  Relay_Action(1);       //Se reinicia el relay
  User_relay_estado=EST_RELAY_CERRADO;

  store_flashI2C(TYPE_USER_INFO,      (char*)&User_Config_Data);
  store_flashI2C(TYPE_LIMITADOR_INFO,(char*)&Lim_Config_Data);
}

void Limitacion_Run(float Amp_leido)
{
 
  read_flashI2C(TYPE_LIMITADOR_INFO ,(char*)&Lim_Config_Data); 

  #ifdef SYSCONFIG_DEBUG
  Serial.println("Amp_leido>SCR_Data.A_limitacion");
  Serial.print(Amp_leido);
  Serial.print(" > ");
  Serial.println(Lim_Config_Data.A_limitacion);
  #endif
  
  if(Amp_leido>Lim_Config_Data.A_limitacion)
  {
       Serial.print("RELAYoff->penalizacion");
       Relay_Action(0);
       User_Config_Data=EST_PENALIZADO;
       User_relay_estado=EST_RELAY_ABIERTO;
  }
  
  if(User_Config_Data==EST_PENALIZADO)
  {
    #ifdef SYSCONFIG_DEBUG
    Serial.print("tiempo_reconec:");
    Serial.println(Lim_Config_Data.Tiempo_reconeccion_seg);
    #endif
    if(time_validar_seg(&time_limitador_ref,
                        Lim_Config_Data.Tiempo_reconeccion_seg))
    {
      User_Config_Data=EST_LIMITADO;
      Serial.print("RELAYoN->Des_penalizacion");
      Relay_Action(1);
      User_relay_estado=EST_RELAY_CERRADO;
    }
  }

}

void return_Limitacion_char_5(char* Corriente_lim_configurada)
{
   snprintf (Corriente_lim_configurada,4,"%2.1f",Lim_Config_Data.A_limitacion); 
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////


///////////////////DOCIFICACION////////////////////////
void Docificacion_Setup(float parametro)
{
  //read_flashI2C(TYPE_DOFI_INFO,     (char*)&Dosifi_Config_Data);
  if(parametro>0.0)
  {
    User_Config_Data=EST_DOFICACION;
    Dosifi_Config_Data.KW_Cuota_Dia=parametro;
  }
  else
  {
    Dosifi_Config_Data.KW_Cuota_Dia=0;
  }
 
  Relay_Action(1); 
  User_relay_estado=EST_RELAY_CERRADO;
  
  store_flashI2C(TYPE_USER_INFO,     (char*)&User_Config_Data);
  store_flashI2C(TYPE_DOFI_INFO,     (char*)&Dosifi_Config_Data);
}

void Docificacion_Run(float Energia_leida)
{
  read_flashI2C(TYPE_DOFI_INFO,     (char*)&Dosifi_Config_Data);
  //a las 8 pm todos los dias se reinicia
  if(time_equal(-1,-1,-1,19,-1))
  {
    Dosifi_Config_Data.KW_Dofi_Inicio_ref=Energia_leida;
    Relay_Action(1);
    User_relay_estado=EST_RELAY_CERRADO;
  }
  //En modo Dosificador.
  Dosifi_Config_Data.KW_Consumo_total= Dosifi_Config_Data.KW_Consumo_total+
                                       (Energia_leida-Dosifi_Config_Data.KW_Dofi_Inicio_ref);
  Dosifi_Config_Data.KM_Disponible=    Dosifi_Config_Data.KW_Cuota_Dia-
                                       (Energia_leida-Dosifi_Config_Data.KW_Dofi_Inicio_ref);

  if(Dosifi_Config_Data.KM_Disponible<=0.0)
  {
    Relay_Action(0);    
    User_relay_estado=EST_RELAY_ABIERTO;
  }
     
}

void return_Dosifi_info(Dosifi_info* Dofi_Configuracion_actual)
{
  *Dofi_Configuracion_actual=Dosifi_Config_Data;
}

//AA,MM,DD, CW, LA, DW, PW, ES, RR 
//(AA,MM,DD) Fecha ultimo corte
//(CW)       Energuia activa ultimo corte
//(LA)       Limitado a 
//(DW)       Energia Dofisicacion cuota
//(PW)       Energia Saldo Dosificacion
//(ES)       Estado o Modo de la caja (Normalizado (Lim a 80A), Limitado, Pregago, Dosificacion)
//(RR)       Estado actual del relay (Abierto o Cerrado)
void Estatus_KWLIMDOFI_50(char* Status_info)
{
  snprintf (Status_info,50,"xx,xx,01, %4.3f, %2.1f, %3.1f, %4.3f, %d, %d",Medidor_Config_Data.KW_Inicio_Corte_ref,
                                                                          Lim_Config_Data.A_limitacion,
                                                                          Dosifi_Config_Data.KW_Cuota_Dia,
                                                                          Dosifi_Config_Data.KM_Disponible,
                                                                          User_Config_Data,
                                                                          User_relay_estado);
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////


//////////////////////PREPAGO//////////////////////////
void Prepago_Setup(float parametro)
{
  //read_flashI2C(TYPE_PREPAGO_INFO,  (char*)&Prepago_Config_Data);

  Actualizar_KW_Pre_flag=1;
  User_Config_Data=EST_PREPAGO;
  Datetochar(Prepago_Config_Data.Fecha_recarga); //Leo desde el RTC la fecha actual.
  if(parametro>0.0)
  {
    Prepago_Config_Data.KM_Recarga=parametro;
    Prepago_Config_Data.KM_Saldo_anterior=Prepago_Config_Data.KM_Saldo;    
  }
  else
  {
    Serial.print("reset_prepago=>");
    Prepago_Config_Data.KM_Recarga=0.0;
    Prepago_Config_Data.KM_Saldo_anterior=0.0;
    Prepago_Config_Data.KM_Saldo=0.0;
    Serial.println(Prepago_Config_Data.KM_Saldo_anterior);
  }

  store_flashI2C(TYPE_USER_INFO,     (char*)&User_Config_Data);
  store_flashI2C(TYPE_PREPAGO_INFO,  (char*)&Prepago_Config_Data);
}

void Prepago_Run(float Energia_leida)
{
  double Delta_consumo;
  double Carga_actual;

  read_flashI2C(TYPE_PREPAGO_INFO,  (char*)&Prepago_Config_Data);

  Serial.println("Modo_prepago");
  if(Actualizar_KW_Pre_flag) 
  {
    Serial.print("inicio_prepago en ");
    Serial.println(Energia_leida);
    Prepago_Config_Data.KW_Pre_Inicio_ref=Energia_leida;
    Actualizar_KW_Pre_flag=0;
    store_flashI2C(TYPE_PREPAGO_INFO,  (char*)&Prepago_Config_Data);
  }
  Delta_consumo=Energia_leida-Prepago_Config_Data.KW_Pre_Inicio_ref;
  Carga_actual=Prepago_Config_Data.KM_Saldo_anterior+Prepago_Config_Data.KM_Recarga;
   //Consumo_total=Consumo_total+Delta(Consumo)                                   
  Prepago_Config_Data.KM_Consumo_total=Prepago_Config_Data.KM_Consumo_total+Delta_consumo;
   //Saldo=Saldo(anterior)+Recarga -Delta(Consumo)

  Prepago_Config_Data.KM_Saldo=Carga_actual-Delta_consumo;
                                
  Serial.print("Carga_prepago ");
  Serial.println(Carga_actual);
  Serial.print("delta consumo prepago ");
  Serial.println(Delta_consumo);
  if(Prepago_Config_Data.KM_Saldo<=0.0)
  {
    Prepago_Config_Data.KM_Saldo=0;
    Relay_Action(0); 
    User_relay_estado=EST_RELAY_ABIERTO;
  }
  else
  {
    Relay_Action(1); 
    User_relay_estado=EST_RELAY_CERRADO;
  }
  MQTT_publish_led();
}

void return_Prepago_info(Prepago_info* Prepago_Configuracion_actual)
{
  *Prepago_Configuracion_actual=Prepago_Config_Data;
  //Serial.print("prepago_info=>");

}
//AA,MM,DD,RW,SPW,DW
//(AA,MM,DD) Fecha ultima recarga
//(RW)       Cantidad ultima recarga
//(SPW)      Saldo previo
//(DW)       Saldo disponible
void Estatus_prepago_50(char* Status_info)
{
  snprintf (Status_info,50,"%s, %4.1f, %4.2f, %4.2f, %4.3f, %d",Prepago_Config_Data.Fecha_recarga,
                                                                Prepago_Config_Data.KM_Recarga,
                                                                Prepago_Config_Data.KM_Saldo_anterior,
                                                                Prepago_Config_Data.KM_Saldo,
                                                                Prepago_Config_Data.KM_Consumo_total,
                                                                User_Config_Data);
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////


////////////////////////SCR///////////////////////////
void Suspension_Setup(void)
{
  Relay_Action(0);
   User_relay_estado=EST_RELAY_ABIERTO;
  switch(User_Config_Data) 
  {
    case EST_NORMAL:
      User_Config_Data=EST_SUS_NORMA;
    break;
    case EST_PENALIZADO: //Limitado y relay abierto
    case EST_LIMITADO:   //Limitado y relay cerrado
      User_Config_Data=EST_SUS_LIMI;
    break;
    case EST_DOFICACION:
      User_Config_Data=EST_SUS_DOSI;
    break;
    case EST_PREPAGO:
      User_Config_Data=EST_SUS_PREPA;
    break;
  }
  store_flashI2C(TYPE_USER_INFO,     (char*)&User_Config_Data);

}

void Reconect_Setup(void)
{
  Relay_Action(1);
  User_relay_estado=EST_RELAY_CERRADO;
  switch(User_Config_Data) 
  {
    case EST_SUS_NORMA:
      User_Config_Data=EST_NORMAL;
    break;
    case EST_SUS_LIMI:
      User_Config_Data=EST_LIMITADO;
    break;
    case EST_SUS_DOSI:
      User_Config_Data=EST_DOFICACION;
    break;
    case EST_SUS_PREPA:
      User_Config_Data=EST_PREPAGO;
    break;
  }
  store_flashI2C(TYPE_USER_INFO,     (char*)&User_Config_Data);   
}

void Normaliza_Setup(void)
{
  User_Config_Data=EST_NORMAL;
  Lim_Config_Data.A_limitacion=80;
  Relay_Action(1);
  User_relay_estado=EST_RELAY_CERRADO;
  store_flashI2C(TYPE_USER_INFO,      (char*)&User_Config_Data);
  store_flashI2C(TYPE_LIMITADOR_INFO,(char*)&Lim_Config_Data);  
}
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////



////////////////////////UTIL///////////////////////////
void Tareas_Setup(void)
{
  read_flashI2C(TYPE_LIMITADOR_INFO,(char*)&Lim_Config_Data);
  read_flashI2C(TYPE_PREPAGO_INFO,  (char*)&Prepago_Config_Data);
  read_flashI2C(TYPE_DOFI_INFO,     (char*)&Dosifi_Config_Data);
  read_flashI2C(TYPE_USER_INFO,     (char*)&User_Config_Data); 
  //#ifdef SYSCONFIG_DEBUG
  Serial.print("Read_tipo_user:");
  Serial.println(User_Config_Data);
 // #endif
}



void Almacenar_Configuracion_Data(void)
{
  store_flashI2C(TYPE_LIMITADOR_INFO,(char*)&Lim_Config_Data);
  store_flashI2C(TYPE_PREPAGO_INFO,  (char*)&Prepago_Config_Data);
  store_flashI2C(TYPE_DOFI_INFO,     (char*)&Dosifi_Config_Data);
  #ifdef SYSCONFIG_DEBUG
  Serial.print("Suser:");
  Serial.println(User_Config_Data);
  #endif
  store_flashI2C(TYPE_USER_INFO,     (char*)&User_Config_Data);

}