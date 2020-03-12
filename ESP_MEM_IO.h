#ifndef ESP_MEM_IO_H
#define ESP_MEM_IO_H

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////Bandera INI EEPROM/////////////////////////////////////////////////////
#define EE_INI_VAL 0X54
//#define MODO_COMUNICACION_DEFAULT MODO_GSM
#define MODO_COMUNICACION_DEFAULT MODO_WIFI
//#define WIFI_MODO_USER WIFI_ID_SCAN
#define WIFI_MODO_USER WIFI_ID_DEFAULT
#define ID_CAJA_DEFAULT 243

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define SW_VER "1.0.2"
#define MQQT_LED_IO 13  //IO13 (LABEL) 16 (PIN)
//////////////////////////////////////////////////////EEPROM I2C//////////////////////////////////////////////////////// 
//Se reservan 20 bancos de datos, para cada variable.
//La siguiente posicion vacia es la 21.
#define DATA_INDEX_OVERDRIVE 21       

//8 tipos de datos, la siguiente posicion vacia es la 9.  
#define DATA_ADD_OFFSET sizeof(int)*9  

#define TYPE_FLAGS          1
#define TYPE_MEDIDOR_INFO   2
#define TYPE_LIMITADOR_INFO 3
#define TYPE_PREPAGO_INFO   4
#define TYPE_DOFI_INFO      5
#define TYPE_USER_INFO      6
#define TYPE_GPS_INFO       7
#define TYPE_OTA_INFO       8

#define TAM_FLAGS           sizeof( All_flag)
#define TAM_MEDIDOR_INFO    sizeof( Medidor_info)
#define TAM_LIMITADOR_INFO  sizeof( Limitador_info)
#define TAM_PREPAGO_INFO    sizeof( Prepago_info)
#define TAM_DOFI_INFO       sizeof( Dosifi_info)
#define TAM_USER_INFO       sizeof( int)
#define TAM_GPS_INFO        sizeof( GPS_info)
#define TAM_OTA_INFO        sizeof( OTA_info)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 

//////////////////////////////////////////////////////EEPROM ESP32 512///////////////////////////////////////////////
#define TYPE_ID_CFG_MODO    20
#define TYPE_PARAMETROS_CFG 21
#define TYPE_FECHA_FAB      22
#define TYPE_FECHA_CORTE_KW 23
#define TYPE_FECHA_CORTE_PR 24


#define TAM_ID_CFG_MODO     sizeof ( char)
#define TAM_PARAMETROS_CFG  sizeof( Parametros_CFG)  
#define TAM_FECHA_FAB       sizeof ( char)*8  
#define TAM_FECHA_CORTE_KW  sizeof ( char)
#define TAM_FECHA_CORTE_PR  sizeof ( char)     



struct Parametros_CFG
{
  char SSID_char[20];        //NOMBRE WIFI
  char SSID_PASS_char[20];   //CLAVE WIFI
  char ORCOM_char[20];       //OPERADOR RED
  char CDTRAFO_ID_char[12];  //CENTRO DE DISTRIBUCION
  char IMYCO_ID_char[6];     //NUMERO DE CAJA
  char CTOSUS_char[12];      //NUMERO DE CONTRATO
  char USERCAJA_char[30];    //NOMBRE DEL USUARIO
  char IMEI_char[16];        //IMEI GSM
  char N_telefono[12];       //NUMERO DE TELEFONO
  char GSM_char[21];         //IMEI GPS
  char DIRECCION_char[30];   //DIRECCION USER
 // char MEN_LOCK_ID;
};
//=20+20+20+12+6+12+30+16+11+20+1=168

struct All_flag
{
   char Memoria_ini_flag;             //Indica si la memoria no se ha inicializado a los valores por defecto
   char Actualizar_ref_KW_flag;       //Solo se graba la primera vez, marca el inicio el consumo del usuario
};

struct Medidor_info  
{
    float KW_Inicio_Medida_ref;    //Valor de refencia de inicio del consumo usuario
    float KW_Inicio_Corte_ref;     //Acumulado a la fecha de corte
    float KM_ref_error;
    float KM_delta_error;          //Error por corriente parasita
    float KM_delta_error_anterior; //Error por corriente parasita
};

struct Limitador_info
{
   float  A_limitacion;           //Valor de limitacion
   long   Tiempo_reconeccion_seg;
};

struct Prepago_info
{
    char  Fecha_recarga[10];
    float KW_Pre_Inicio_ref;
    float KM_Consumo_total;
    float KM_Saldo_anterior;
    float KM_Saldo;
    float KM_Recarga;
};

struct Dosifi_info
{
    float    KW_Cuota_Dia;
    float    KM_Disponible;
    float    KW_Disponible_dia_anterior;
    float    KW_Dofi_Inicio_ref;
    float    KW_Consumo_total;
    boolean  Indicador_ref_hora;

};

struct GPS_info
{
    unsigned int Satelites_found;
    double Latitud_leida;
    double Longitud_leida;
    double Latitud_ultima_conocida;
    double Longitud_ultima_conocida;
    long   failedChecksum_data;
    long   passedChecksum_data;
    long   charsProcessed_data;
    int    Fix_data;
};

struct OTA_info
{
  unsigned char OTA_Activado;     //En Modo
  unsigned char Ino_file_Error;   //Error de descarga o conexcion al AS3
  unsigned char No_ESP32_espace;  //Archivo demasiado grande para particion
  unsigned char Last_OTA_OK;      //Exito en la ultima OTA
  unsigned char Last_OTA_Error;   //Error en la ultima OTA
  long Ino_file_size;             //Tamaño del archivo a cargar.
  long Ino_file_complete_size;    //Tamaño cargado.
};

void Setup_ESP_flash(void);
void  ini_I2C_Data(char            flag_var_randon);

int  Data_size(int type);
void store_flashESP32(int add_start,int size_data, char *byte_data);
void read_flashESP32(int add_start,int size_data, char *byte_data);

void read_flashI2C(int Data_type,char* Info);
void store_flashI2C(int Data_type,char* Info);
void Read_index_reinicio(int* pReinicio_index);
void Store_index_reinicio(int Reinicio_index);
int  Add_flash(int type);

void LED_Setup(void);
void MQTT_LED(char action);
void MQTT_LED_CONNECTED_OK(void);


#endif
