#ifndef WIFIUSER_H
#define WIFIUSER_H

//Modos de configuacion de parametros iniciales.
#define WIFI_ID_SCAN       1   //Busqueda y configuracion de parametros (Numero de caja)
//#define WIFI_ID_RESCAN   2   //Clave errada, o Configuracion fallo
#define WIFI_ID_SET        3   //Se conecto con exito y ID colocado, no es necesario configurar parametros.
#define WIFI_ID_DEFAULT    4   //Usar parametros por defecto

bool Setup_WiFI(void);

void Get_MAC_19(char * MAC);

#endif
