#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h> 
#include "MODEM_USER.h"


#define SerialAT Serial2
#define SerialMon Serial
//#define modem_DEBUG

////////////////////GSM/////////////////////////////
TinyGsm       modemGSM(SerialAT);
const char    apn[]  = "web.colombiamovil.com.co";
const char    gprsUser[] = "";
const char    gprsPass[] = "";
char RSSID_estado[20];


bool Setup_GSM(void)
{

  
  SerialAT.begin(9600);
  ///////////////////////////////////////////////////
  #ifdef modem_DEBUG
  SerialMon.println("Initializing modem...");
  #endif
  modemGSM.restart();
  SerialMon.println("Wait 5 seg Modem power_star up");
  delay(5000);
  //////////////////////////////////////////////////
  #ifdef modem_DEBUG
  String modemInfo = modemGSM.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  #endif
  modemGSM.getSimStatus();
  //////////////////////////////////////////////////
  SerialMon.println("Waiting for network...");
  if (modemGSM.waitForNetwork()) 
  {
     if (modemGSM.isNetworkConnected()) 
     {
        SerialMon.print("Network connected, SQ=");
        Ponderar_RSSID_20(modemGSM.getSignalQuality(),RSSID_estado);
        SerialMon.println(RSSID_estado);
        return true;
      }
  }
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