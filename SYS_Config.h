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



void Energia_Corte_Run(float Energia_leida);
void Normaliza_Setup(void);
void Suspension_Setup(void);
void Reconect_Setup(void);
void Limitacion_Setup_4(char *parametro);
void Limitacion_Run(float Lectura_A);
void Docificacion_Setup_4(char *parametro);
void Docificacion_Run(float Energia_leida);
void Estatus_KWLIMDOFI_50(char* Status_info);
void Prepago_Setup_4(char* parametro);
void Prepago_Run(float Energia_leida);
void Estatus_prepago_50(char* Status_info);
void Almacenar_Configuracion_Data(void);
void Tareas_Setup(void);
int  return_User_Config_Data(void);

#endif
