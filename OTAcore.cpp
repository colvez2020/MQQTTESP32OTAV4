#include <Update.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "OTAcore.h"
#include "ESP_MEM_IO.h"

////////////////////////////// S3 Bucket Config  /////////////////////////////////////////////////
//int              port =  80; // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
int              port =  1880;
//String           host = "tecno5.s3-us-west-2.amazonaws.com"; // Host => bucket-name.s3.region.amazonaws.com
String           host = "http://192.168.1.80";
IPAddress server(192,168,1,80);
//String           bin  = "/MQQTTESP32OTA.ino.esp32.bin"; // bin file name with a slash in front.
String           bin  = "/hello-file"; // bin file name with a slash in front.
//////////////////////////////////////////////////////////////////////////////////////////////////

bool             canBegin;
bool             isValidContentType = false;
long             contentLength=0;
size_t           written;
WiFiClient       client;
static OTA_info  Estado_OTA;
Parametros_CFG   ESP32_Parametros_actuales;

///////////////// Utility ////////////////////////////////////// 
String getHeaderValue(String header, String headerName) 
{
  return header.substring(strlen(headerName.c_str()));
}

void progressFunc(unsigned int progress,unsigned int total) 
{
  Serial.printf("Progress: %u of %u\r", progress, total);
  Serial.println("");
  
}

void Almacenar_OTA_Data(OTA_info* pEstado_OTA)
{
  store_flashI2C(TYPE_OTA_INFO,(char*)pEstado_OTA);
}


bool Get_AS3_file(void)
{
  int test_connect=0;

  read_flashESP32(Add_flash(TYPE_PARAMETROS_CFG),
                  Data_size(TYPE_PARAMETROS_CFG),
                  (char*)&ESP32_Parametros_actuales);

  //Serial.print("Connecting Modo_OTA to ");
  //Serial.println(ESP32_Parametros_actuales.SSID_char);
  //Serial.print("Pass ");
  //Serial.println(ESP32_Parametros_actuales.SSID_PASS_char); 
  //WiFi.begin(ESP32_Parametros_actuales.SSID_char, 
   //          ESP32_Parametros_actuales.SSID_PASS_char);
  WiFi.begin("Tecno5","Cuatin05321"); 

  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print("."); // Keep the serial monitor lit!
    delay(500);
    test_connect++;
    if(test_connect==20)
    {
      Serial.println("reboot_wifi_MODO_OTA");
      ESP.restart();
    } 
  }

  // Connection Succeed
  Serial.println("");
  Serial.println("Connected_WIFI_OK ");


  Serial.println("Connecting to: " + String(host));
  // Connect to S3
  delay(500);
  delay(500);
  delay(500);
/*
    HTTPClient http;
 
    http.begin("http://192.168.1.80:1880/hello-file"); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
        Serial.println("fin");
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources*/
  
  //if (client.connect(host.c_str(), port)) 
  if (client.connect(server, port)) 
  {
  
    Serial.println("Fetching Bin: " + String(bin));
    client.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Cache-Control: no-cache\r\n" +
    "Connection: close\r\n\r\n");
  
    // Check what is being sent
    Serial.print(String("GET ") + bin + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "Cache-Control: no-cache\r\n" +
    "Connection: close\r\n\r\n");
  
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      if (millis() - timeout > 5000) 
      {
        Serial.println("Client Timeout !");
        client.stop();
        Estado_OTA.Ino_file_Error=1;
        Almacenar_OTA_Data(&Estado_OTA);
        return false;
      }
    }
    /*
    Response Structure
    HTTP/1.1 200 OK
    x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
    x-amz-request-id: 2D56B47560B764EC
    Date: Wed, 14 Jun 2017 03:33:59 GMT
    Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
    ETag: "d2afebbaaebc38cd669ce36727152af9"
    Accept-Ranges: bytes
    Content-Type: application/octet-stream
    Content-Length: 357280
    Server: AmazonS3
                     
    {{BIN FILE CONTENTS}}
    
    */
    while (client.available()) 
    {
      // read line till /n
      String line = client.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      Serial.println(line);
      line.trim();
  
      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `client` to the
      // Update.writeStream();
      if (!line.length()) 
      {
        //headers ended
        Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));
        if(contentLength && isValidContentType)
        {
          Estado_OTA.Ino_file_Error=0;
          Estado_OTA.Ino_file_size=contentLength;
          return true; // and get the OTA started
        }
      }
    
      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1")) 
      {
        Serial.print("Respuesta=");
        Serial.println(line);
        if (line.indexOf("200") < 0) 
        {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update."); 
          Estado_OTA.Ino_file_Error=1; 
          Estado_OTA.OTA_Activado=0;
          Almacenar_OTA_Data(&Estado_OTA);  
          return false;
        }
      }
    
      // extract headers here
      // Start with content length
      if (line.startsWith("Content-Length: ")) 
      {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }
    
      // Next, the content type
      if (line.startsWith("content-type: ")) 
      {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") 
        {
          isValidContentType = true;
        }
      }
    }//while(cliente.avariable) n°2 
  }
  else //if (client.connect(host.c_str(), port)) 
  {
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
    Estado_OTA.Ino_file_Error=1;
    Almacenar_OTA_Data(&Estado_OTA);
    return false;
  }
  return true;
}

void execOTA(void)
{
  canBegin = Update.begin(contentLength);
  Serial.print("canBegin=");
  Serial.println(canBegin);
  
  Serial.print("canBegin=");
  Serial.println(canBegin);
  Serial.print("canBegin=");
  Serial.println(canBegin);
  Serial.print("canBegin=");
  Serial.println(canBegin);
  Serial.print("canBegin=");
  Serial.println(canBegin);
  // If yes, begin
  if (canBegin) 
  {
    Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
    // No activity would appear on the Serial monitor
    // So be patient. This may take 2 - 5mins to complete
    written = Update.writeStream(client);
  
    if (written == contentLength) 
    {
      Serial.println("Written : " + String(written) + " successfully");
       
    } 
    else 
    {
      Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
      
    }
    
    if (Update.end()) 
    {
      Serial.println("OTA done!");
      if(Update.isFinished()) 
      {
          Serial.println("Update successfully completed. Rebooting.");
          Estado_OTA.Last_OTA_OK=1;
      } 
      else 
      {
        Serial.println("Update not finished? Something went wrong!");
         Estado_OTA.Last_OTA_OK=0;
      }
    }
    else  // (Update.end()) 
    {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      
    }
  } 
  else //(canBegin) 
  {
    // not enough space to begin OTA
    // Understand the partitions and
    // space availability
    Serial.println("Not enough space to begin OTA");
    Estado_OTA.No_ESP32_espace=1;
  }
  Estado_OTA.Last_OTA_Error=Update.getError();
  Estado_OTA.Ino_file_size=contentLength;
  Estado_OTA.Ino_file_complete_size=written;
  Almacenar_OTA_Data(&Estado_OTA);

  ESP.restart();  //Siempre hay que reiniciar, ya sea para ejecutar el codigo nuevo o para restaurar en algun error.
}

void OTAsetup(void)
{
  Update.onProgress(progressFunc);
  Estado_OTA.OTA_Activado=0;            //Desactivo_Modo_OTA para no caer en bluque de reflacheo.
  Estado_OTA.Ino_file_Error=0;          //Sin Error de descarga o conexcion al AS3
  Estado_OTA.No_ESP32_espace=0;         //Sin Error Archivo demasiado grande para particion
  Estado_OTA.Last_OTA_OK=0;             //Sin Exito, porque no se ha realizado el OTA
  Estado_OTA.Last_OTA_Error=0;          //Sin Error de quemado FW, porque no se ha realizado el OTA
  Estado_OTA.Ino_file_size=0;           //En cero, porque no se leido el archivo.
  Estado_OTA.Ino_file_complete_size=0;  //En cero porque no se a Tamaño cargado al FW.
}

unsigned char OTA_Activado(void)
{
  Serial.println("Leo_bandera_OTA");
  read_flashI2C(TYPE_OTA_INFO ,(char*)&Estado_OTA); 
  
  return Estado_OTA.OTA_Activado;
}

void Estatus_OTA_50(char* Status_info)
{
      snprintf (Status_info,49,"%u, %u, %u, %u, %u, %ld, %ld",Estado_OTA.OTA_Activado     //En Modo
                                                                  ,Estado_OTA.Ino_file_Error   //Error de descarga o conexcion al AS3
                                                                  ,Estado_OTA.No_ESP32_espace  //Archivo demasiado grande para particion
                                                                  ,Estado_OTA.Last_OTA_OK      //Exito en la ultima OTA
                                                                  ,Estado_OTA.Last_OTA_Error   //Error en la ultima OTA
                                                                  ,Estado_OTA.Ino_file_size             //Tamaño del archivo a cargar.
                                                                  ,Estado_OTA.Ino_file_complete_size    //Tamaño cargado.
                                                                   );
}
