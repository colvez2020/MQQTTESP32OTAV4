#ifndef BTUSER_H
#define BTUSER_H

bool BT_pair(void);
void Estatus_ESP32CFG(void);
void Setup_BT(void);
char BT_CMD_incomming(String messageBT);
void BT_CMD_RUM(char CMD);
//void BT_send_data_32(char * mensaje);

#endif