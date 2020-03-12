#ifndef GPSMAIN_H
#define GPSMAIN_H

void GPS_Task_ini(void);
void GPS_READ( void * pvParameters );
void GPS_Setup(void);
void GPS_Maintenice(void);
bool GPS_Longitud_Latitud(double* Latitud, double* longitud, unsigned int* SAT );
void GPS_Debug(void);
void return_GPS_info_data(GPS_info* Actual_GPS_DATA);
void GPS_Status_100(char* Status_info);
void Almacenar_GPS_Data(void);
void GPS_RUN(void);

#endif
