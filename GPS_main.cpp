#include <TinyGPS++.h>
#include <LiquidCrystal_I2C.h>
#include "ESP_MEM_IO.h"
#include "GPS_main.h"
#include "RTC_user.h"


HardwareSerial GPSserial(1);  
TinyGPSPlus    gpsNEO6M;
long           time_GPS,time_GPS_ref;
GPS_info       GPS_Status_data;
TaskHandle_t        GSM_Task;


void GPS_Task_ini(void)
{
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
   xTaskCreatePinnedToCore(
      GPS_READ,  
      "Task1", 
      10000,   
      NULL,   
      0,   
      &GSM_Task,  
      0);  
  delay(500);
}

void GPS_READ( void * pvParameters )
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for(;;)
  {
     GPS_Maintenice();
  } 
}

void GPS_Setup(void)
{
   //RX,TX
   GPSserial.begin(9600,SERIAL_8N1,34,0);
   GPSserial.setRxBufferSize(512);
   read_flashI2C(TYPE_GPS_INFO,      (char*)&GPS_Status_data);
}

void GPS_Debug(void)
{
  Serial.print("Sentences that failedChecksum=");
  Serial.println(gpsNEO6M.failedChecksum());
  
  Serial.print("Sentences that passedChecksum=");
  Serial.println(gpsNEO6M.passedChecksum());
 
  Serial.print("Sentences that charsProcessed=");
  Serial.println(gpsNEO6M.charsProcessed());

  Serial.print("Sentences that sentencesWithFix=");
  Serial.println(gpsNEO6M.sentencesWithFix());  
  
}
void GPS_Maintenice(void)
{
   unsigned long start = millis(); 
   do { 
        while (GPSserial.available())  
          gpsNEO6M.encode(GPSserial.read());  
      vTaskDelay(10 / portTICK_PERIOD_MS); 
   } while (millis() - start < 1000); 
}

bool GPS_Longitud_Latitud(double* Latitud, double* longitud, unsigned int* SAT )
{ 
  *SAT=gpsNEO6M.satellites.value();
  if (gpsNEO6M.location.isValid())
  {
     *Latitud=gpsNEO6M.location.lat();
     *longitud=gpsNEO6M.location.lng();      
     return true;
  }
  return false;
}

void return_GPS_info_data(GPS_info* Actual_GPS_DATA)
{
  *Actual_GPS_DATA=GPS_Status_data;
}

void GPS_Status_100(char* Status_info)
{
   snprintf (Status_info,100,"%f, %f, %f, %f, %i, %i, %ld, %ld, %ld",GPS_Status_data.Latitud_leida,
                                                                     GPS_Status_data.Longitud_leida,
                                                                     GPS_Status_data.Latitud_ultima_conocida,
                                                                     GPS_Status_data.Longitud_ultima_conocida,
                                                                     GPS_Status_data.Satelites_found,
                                                                     GPS_Status_data.Fix_data,
                                                                     GPS_Status_data.charsProcessed_data,
                                                                     GPS_Status_data.passedChecksum_data,
                                                                     GPS_Status_data.failedChecksum_data);
}

void GPS_RUN(void)
{
    if(gpsNEO6M.location.isValid())
    {
      GPS_Status_data.Latitud_leida=gpsNEO6M.location.lat();
      GPS_Status_data.Longitud_leida=gpsNEO6M.location.lng();
      GPS_Status_data.Latitud_ultima_conocida=GPS_Status_data.Latitud_leida;
      GPS_Status_data.Longitud_ultima_conocida=GPS_Status_data.Longitud_leida;
    }
    else
    {
      GPS_Status_data.Latitud_leida=-1;
      GPS_Status_data.Longitud_leida=-1;
    }
    GPS_Status_data.failedChecksum_data=gpsNEO6M.failedChecksum();
    GPS_Status_data.passedChecksum_data=gpsNEO6M.passedChecksum();
    GPS_Status_data.charsProcessed_data=gpsNEO6M.charsProcessed();
    GPS_Status_data.Fix_data=gpsNEO6M.location.isValid();
    GPS_Status_data.Satelites_found=gpsNEO6M.satellites.value();

}


void Almacenar_GPS_Data(void)
{
  store_flashI2C(TYPE_GPS_INFO,      (char*)&GPS_Status_data);
}

