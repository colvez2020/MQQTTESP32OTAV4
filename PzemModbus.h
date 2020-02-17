#ifndef PZEMMODBUS_H
#define PZEMMODBUS_H

struct PZEM_DataIO{
    float voltage;
    float current;
    float power;
    float energy;
    float energy_user;
    float frequeny;
    float pf;
    uint16_t alarms;
}; // Measured values

void Setup_PZEM(void);
bool PZEM_all_measurements(PZEM_DataIO *p_measurements);
void Estatus_PZEM_50(char* Status_info);
void Almacenar_PZEM_Data(void);


#endif
