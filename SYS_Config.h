#ifndef SYSCONFIG_H
#define SYSCONFIG_H

//Estados Usuarios
#define EST_NORMAL      1     //Sin limitacion 80 max
#define EST_LIMITADO    2     //Limitado a un valor
#define EST_PENALIZADO  3     //Desconectado por sobrecorriente
#define EST_DOFICACION  4     //En Dosifi
#define EST_PREPAGO     5     //En prepago
#define EST_SUS_NORMA   6     //En Suspencion (6-9) 
#define EST_SUS_LIMI    7     //En Suspencion (6-9)
#define EST_SUS_DOSI    8     //En Suspencion (6-9)
#define EST_SUS_PREPA   9     //En Suspencion (6-9)

//Estados relay

#define EST_RELAY_CERRADO      1      
#define EST_RELAY_ABIERTO      2     

#include "ESP_MEM_IO.h"
#include "PzemModbus.h"

int  return_User_Config_Data(void);
void return_User_Config_char_20(char* User_config_char);
void return_User_relay_estado(boolean* Relay_estado);

float Energia_Corte_Run(float Energia_leida);

void Limitacion_Setup(float parametro);
void Limitacion_Run(float Lectura_A);
void return_Limitacion_char_5(char* Corriente_lim_configurada);

void Docificacion_Setup(float parametro);
void Docificacion_Run(float Energia_leida);
void return_Dosifi_info(Dosifi_info* Configuracion_actual);
void Estatus_KWLIMDOFI_50(char* Status_info);

void Prepago_Setup(float parametro);
void Prepago_Run(float Energia_leida);
void return_Prepago_info(Prepago_info* Prepago_Configuracion_actual);
void Estatus_prepago_50(char* Status_info);

void Normaliza_Setup(void);
void Suspension_Setup(void);
void Reconect_Setup(void);

void Tareas_Setup(void);
void Planificador_tareas(PZEM_DataIO   PZEM_004T);
void Almacenar_Configuracion_Data(void);

#endif
