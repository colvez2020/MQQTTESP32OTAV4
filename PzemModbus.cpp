#include <PZEM004Tv30.h>
#include "PzemModbus.h"
#include "ESP_MEM_IO.h"

//#define PZEM_DEBUG

char           Actualizar_ref_KW_Error_flag=1; //Indica presencia de offset
char           Actualizar_KM_delta_error_flag=1; //Indica presencia de offset

static uint8_t pzemSlaveAddr = 0x01;
Medidor_info   EEPROMMedidor_info;
PZEM_DataIO    PZEM_Status_data;
All_flag       EE_flag_Medidor;


PZEM004Tv30 pzem(&Serial,pzemSlaveAddr);


void Setup_PZEM(void)
{
  //Cada vez que se enciende la maquina leer los parametros.
  read_flashI2C( TYPE_FLAGS        ,(char*)&EE_flag_Medidor);
  read_flashI2C(TYPE_MEDIDOR_INFO,(char*)&EEPROMMedidor_info);
  
  #ifdef PZEM_DEBUG
  Serial.print("KW_Inicio_Medida:");
  Serial.println(EEPROMMedidor_info.KW_Inicio_Medida_ref);
  #endif
  
  //Se configura Energia de inicio
   delay(1000); //Evita error por texto anterior.
   if(!PZEM_all_measurements(&PZEM_Status_data))
      Serial.print("ERROR_NO_PUEDE_INICIAR_MEDIDOR!"); 
   else
   {
      if(EE_flag_Medidor.Actualizar_ref_KW_flag==1)
     {
        EEPROMMedidor_info.KW_Inicio_Medida_ref=PZEM_Status_data.energy;
        EE_flag_Medidor.Actualizar_ref_KW_flag=0;
        
        store_flashI2C( TYPE_FLAGS        ,(char*)&EE_flag_Medidor);
        store_flashI2C( TYPE_MEDIDOR_INFO ,(char*)&EEPROMMedidor_info);
        
        //#ifdef PZEM_DEBUG
        Serial.print("Set KW_Inicio_Medida a =>");
        Serial.println(EEPROMMedidor_info.KW_Inicio_Medida_ref);
        //#endif
     }
   }

}

bool PZEM_all_measurements(PZEM_DataIO *p_measurements)
{
   
  if(!pzem.all_measurements(&p_measurements->voltage,
                            &p_measurements->current,
                            &p_measurements->power,
                            &p_measurements->energy,
                            &p_measurements->frequeny,
                            &p_measurements->pf,
                            &p_measurements->alarms))
    return false;

  #ifdef PZEM_DEBUG  
    Serial.print("kw_leido:");
    Serial.println(p_measurements->energy,4);
    Serial.print("corriente_leida:");
    Serial.println(p_measurements->current,4);
  #endif
  
  ////////////////////////////corriente de fuga/////////////////////////////////////
  if(p_measurements->current<0.04 && p_measurements->current!=0.0 ) 
  {
    if(Actualizar_ref_KW_Error_flag==1)
    {
       Actualizar_ref_KW_Error_flag=0;
       EEPROMMedidor_info.KM_ref_error=p_measurements->energy;
       #ifdef PZEM_DEBUG
        Serial.print("Inicio_errorfuga_ en:"); //primer incidencia, de normal a error
        Serial.println(EEPROMMedidor_info.KM_ref_error);
       #endif
    }
    p_measurements->current=0; //Se aproxima a cero.
  }
  else //////////////////////////caso normal/////////////////////////////////////////.
  {
    Actualizar_ref_KW_Error_flag=1;         //Permito nuevas actualizaciones de puntos de error inicial
    EEPROMMedidor_info.KM_delta_error_anterior=EEPROMMedidor_info.KM_delta_error; //Grabo Delta error pasado;
    EEPROMMedidor_info.KM_ref_error=p_measurements->energy;    //Borro Delta error actual;
  }
  //delta de error = delta_pasado+ (lectura actual - primer incidencia) caso corriente fuga
  //delta de error = delta_pasado+ (lectura actual -lectura actual) caso normal.
  EEPROMMedidor_info.KM_delta_error=EEPROMMedidor_info.KM_delta_error_anterior+
                                    (p_measurements->energy-EEPROMMedidor_info.KM_ref_error);
                                    
  #ifdef PZEM_DEBUG
    Serial.print("delta_error_now:");
    Serial.println(EEPROMMedidor_info.KM_delta_error);
  #endif
  
  //Resultado final= lectura- delta_error;
  p_measurements->energy=p_measurements->energy-
                         EEPROMMedidor_info.KM_delta_error;
  //p_measurements->energy_user= p_measurements->energy-
  //                             EEPROMMedidor_info.KW_Inicio_Medida_ref;
  p_measurements->energy_user=p_measurements->energy_user+(random(0,300)/100);
  //#ifdef PZEM_DEBUG
  Serial.print("resultado_final:");
  Serial.println(p_measurements->energy);
  Serial.print("resultado_final_consumo:");
  Serial.println(p_measurements->energy_user);
  //#endif
  
  return true;
}

//triger by comando
void Estatus_PZEM_50(char* Status_info)
{
      delay(500);
      if(!PZEM_all_measurements(&PZEM_Status_data))
        Serial.print("MQTT_callback_error_PZEM");
      snprintf (Status_info,49,"%4.4f, %2.3f, %4.2f, %1.2f, %3.2f",PZEM_Status_data.energy_user,
                                                                   PZEM_Status_data.current,
                                                                   PZEM_Status_data.power,
                                                                   PZEM_Status_data.pf,
                                                                   PZEM_Status_data.voltage);
      #ifdef PZEM_DEBUG
        Serial.print("Mando energia=>");
        Serial.println(PZEM_Status_data.energy_user,5);
      #endif
}

void Almacenar_PZEM_Data(void)
{
  //Almacena una stampa de los parametros criticos del medidor.
  store_flashI2C(TYPE_MEDIDOR_INFO,  (char*)&EEPROMMedidor_info);
}
