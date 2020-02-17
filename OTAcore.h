#ifndef OTACORE_H
#define OTACORE_H

#include "ESP_MEM_IO.h"

void OTAsetup(void);
void execOTA(void);
boolean Get_AS3_file(void);
unsigned char OTA_Activado(void);
void Almacenar_OTA_Data(OTA_info* pEstado_OTA);
void Estatus_OTA_50(char* Status_info);

#endif
