#include <MCP7940_ADO.h>
#include "RTC_user.h"

#define RTC_DEBUG



String      daysOfTheWeek[7] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
String      monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };
TIME_Assert Mes_Assert[13];
TIME_Assert Dia_Assert[32];
TIME_Assert Hora_Assert[25];
RTC_MCP7940 rtc;
DateTime    now_DATE;

//Temporizador por segundos
bool time_validar_seg(long *time_ref,long seg_segmento)
{
 if(*time_ref==0) //Empezado conteo (primera vez)
   *time_ref =  millis(); 
   
 if ( millis() - *time_ref > (seg_segmento*1000)) 
 {
   *time_ref =  millis(); 
   return true; 
 }
 return false;
}

void Setup_RTC(void)
{
  int i;

  if (!rtc.begin()) 
  {
   Serial.println(F("Couldn't find RTC"));
   //while (1);
  }
  // Si se ha perdido la corriente, fijar fecha y hora
  //if (!rtc.isrunning()) 
  {
   // Fijar a fecha y hora de compilacion
   #ifdef RTC_DEBUG
   Serial.println("Configuro RTC a fecha actual");
   #endif
   //Recordar reinicir el IDE, para tener la fecha actual de conpilacion.
   rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
   // rtc.adjust(DateTime(2019,11, 14,10,21, 0));   
  }
  for(  i=0;i<13;i++)
    Mes_Assert[i].result=false;
  for(  i=0;i<31;i++)
    Dia_Assert[i].result=false;
  for(  i=0;i<24;i++)
    Hora_Assert[i].result=false;
}


void printDate(void)
{
  now_DATE = rtc.now();
  Serial.print(now_DATE.year(), DEC);
  Serial.print('/');
  Serial.print(now_DATE.month(), DEC);
  Serial.print('/');
  Serial.println(now_DATE.day(), DEC);
  //Serial.print(" (");
  //Serial.print(daysOfTheWeek[now_DATE.dayOfTheWeek()]);
 // Serial.print(") ");
  Serial.print(now_DATE.hour(), DEC);
  Serial.print(':');
  Serial.print(now_DATE.minute(), DEC);
  Serial.print(':');
  Serial.print(now_DATE.second(), DEC);
  Serial.println();
}

void FullDatetochar(char* Cadena)
{
  char charBuf[50];

  now_DATE = rtc.now(); 
  
  String dia_Semana=daysOfTheWeek[now_DATE.DayOfWeek()];

  
  dia_Semana.toCharArray(charBuf, 50);
     
  snprintf(Cadena, 50,"%d/%d/%d (%s) %d:%d:%d",now_DATE.year()
                                              ,now_DATE.month()
                                              ,now_DATE.day()
                                              ,charBuf
                                              ,now_DATE.hour()
                                              ,now_DATE.minute()
                                              ,now_DATE.second());
 
}

void Datetochar(char* Cadena)
{
  now_DATE = rtc.now();  
  snprintf(Cadena,16,"%d/%d/%d",now_DATE.day()
                                ,now_DATE.month()
                                ,now_DATE.year()-2000);
}

void Timetochar(char* Cadena)
{
  char Hour_char[3];
  char Min_char[3];

  now_DATE = rtc.now(); 
  if(now_DATE.hour()<9)
   snprintf(Hour_char,3,"0%d",now_DATE.hour());
  else
   snprintf(Hour_char,3,"%d",now_DATE.hour());
   Hour_char[2]='\0';
  if(now_DATE.minute()<9)
   snprintf(Min_char,3,"0%d",now_DATE.minute());
  else
   snprintf(Min_char,3,"%d",now_DATE.minute()); 
   Min_char[2]='\0';   
  snprintf(Cadena, 16,"%s:%s",Hour_char
                             ,Min_char);
                            
}

bool time_equal(int ME, int DD, int HH)
{
  now_DATE = rtc.now(); 

  if(ME!=-1)
  {
    if(now_DATE.day()!= ME)
    {
       Mes_Assert[ME].result=false;;
       return false;
     }
     else
     {

       if( Mes_Assert[ME].result==false)
       {
         Mes_Assert[ME].result=true;
         return true;
       }  
     }
  }     
  if(DD!=-1)
  {
    if(now_DATE.minute()!= DD)
    {
       Dia_Assert[DD].result=false;
       return false;
     }
     else
     {
       if(Dia_Assert[DD].result==false)
       {
         Dia_Assert[DD].result=true;
         return true;
       }  
     }
  }
  if(HH!=-1)
  {
     if(now_DATE.hour()!= HH)
     {
        Hora_Assert[HH].result=false;
        return false;
     }
     else
     {

        //Serial.print("1.hora igual=>Hora_Assert[HH].result=");
        //Serial.println(Hora_Assert[HH].result);
        if(Hora_Assert[HH].result==false)
        {
          //Serial.print("2.hora igual=Hora_Assert[HH].result=");
          Hora_Assert[HH].result=true;
          //Serial.println(Hora_Assert[HH].result);
          return true;
        }  
     }
  }        
 return false;
}
