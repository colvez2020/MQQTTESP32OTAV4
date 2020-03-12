#ifndef MODEMUSER_H
#define MODEMUSER_H

bool Setup_GSM(void);
void Hard_reset_GSM(void);
bool GSM_set_gprs_link(void);
bool GSM_gprs_link_up(void);
void Ponderar_RSSID_20(int16_t RSSID_CODE, char * ESTADO);
void GET_RSSID_20(char * ESTADO);

#endif
