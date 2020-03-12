#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h> 
#include "MODEM_USER.h"


#define SIM800_RESET_IO 25

#define SerialAT Serial2
#define SerialMon Serial
//#define modem_DEBUG

////////////////////GSM/////////////////////////////
TinyGsm       modemGSM(SerialAT);
const char    apn[]  = "web.colombiamovil.com.co";
const char    gprsUser[] = "";
const char    gprsPass[] = "";
char RSSID_estado[20];


void Hard_reset_GSM(void)
{
  pinMode(SIM800_RESET_IO,OUTPUT);
  digitalWrite(SIM800_RESET_IO, HIGH);
  delay(50);
  digitalWrite(SIM800_RESET_IO, LOW);
  delay(150);
  digitalWrite(SIM800_RESET_IO, HIGH);
  delay(850);

}

bool Setup_GSM(void)
{
  SerialAT.begin(9600);
  ///////////////////////////////////////////////////
  SerialMon.println("Wait 3 seg Modem power_star up");
  Hard_reset_GSM();
  modemGSM.restart();

  //////////////////////////////////////////////////
  //#ifdef modem_DEBUG
  String modemInfo = modemGSM.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  //#endif
  if(modemGSM.getSimStatus()!=SIM_READY)
  {
    SerialMon.print("Error SIMCARD");
    return false;
  }
  //////////////////////////////////////////////////
  SerialMon.println("Waiting for network...");
  if (modemGSM.waitForNetwork()) 
  {
     if (modemGSM.isNetworkConnected()) 
     {
        SerialMon.print("Network connected, SQ=");
        GET_RSSID_20(RSSID_estado);
        SerialMon.println(RSSID_estado);
        return true;
      }
  }
  SerialMon.print("Antena error");
  return false;
}

bool  GSM_set_gprs_link(void)
{
  if(modemGSM.isNetworkConnected())
  {
    #ifdef modem_DEBUG 
    SerialMon.print("Connecting to ");
    SerialMon.println(apn);
    #endif
    modemGSM.gprsConnect(apn, gprsUser, gprsPass);
    if (modemGSM.isGprsConnected()) 
    {
      //#ifdef modem_DEBUG
      SerialMon.println("Gprs_Connected");
      //#endif
      return true;
    }
  }
  SerialMon.println("No_Gprs_Connected");
  return false;
}

bool GSM_gprs_link_up(void)
{
  if(modemGSM.isGprsConnected())
    return true;
  return false;
}

void Ponderar_RSSID_20(int16_t RSSID_CODE, char * ESTADO)
{
    snprintf (ESTADO,19,"INDEFINIDO");
    
    #ifdef modem_DEBUG
    SerialMon.print("RSSID_CODE=");
    SerialMon.println(RSSID_CODE);
    #endif

    if(RSSID_CODE>1 && RSSID_CODE<10) //2-9
      snprintf (ESTADO,19,"MARGINAL!=> %d",RSSID_CODE);
    if(RSSID_CODE>9 && RSSID_CODE<15) //10-14
      snprintf (ESTADO,19,"OK!=> %d",RSSID_CODE);
    if(RSSID_CODE>14 && RSSID_CODE<20) //15-19
      snprintf (ESTADO,19,"GOOD!=> %d",RSSID_CODE);
    if(RSSID_CODE>19 && RSSID_CODE<31) //20-30
      snprintf (ESTADO,19,"EXCELLENT!=> %d",RSSID_CODE);
}

void GET_RSSID_20(char * ESTADO)
{
  Ponderar_RSSID_20(modemGSM.getSignalQuality(),ESTADO);
}