#include "Relaycmd.h"
#include <Arduino.h>

#define OPEN_RELAY_LED 16
#define OPEN_RELAY_CMD 33
#define CLOSE_RELAY_LED 17
#define CLOSE_RELAY_CMD 32


void Setup_relay(void)
{
  pinMode(OPEN_RELAY_CMD,OUTPUT);
  pinMode(CLOSE_RELAY_CMD,OUTPUT);
  test_relay(1);
}


void Relay_Action(char accion)
{
  digitalWrite(OPEN_RELAY_CMD,LOW);
  digitalWrite(CLOSE_RELAY_CMD, LOW);   
  if(accion==1)
  {
    digitalWrite(CLOSE_RELAY_CMD, HIGH);
  }
  else
   {
    digitalWrite(OPEN_RELAY_CMD, HIGH);
  } 
  delay(150);
  digitalWrite(OPEN_RELAY_CMD,LOW);
  digitalWrite(CLOSE_RELAY_CMD, LOW);    
}

void test_relay(int veces_user)
{
  int veces_test;
  do
  {
    Serial.println("Abro_relay_test:");
    Relay_Action(0);
    delay(1000);
    Serial.println("Cierro_relay_test:");
    Relay_Action(1);
    delay(1000);
    veces_test++;
  }while(veces_test!=veces_user);
}
