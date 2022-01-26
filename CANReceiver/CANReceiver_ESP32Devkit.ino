#include <esp_now.h>
#include <WiFi.h>
#include <CAN.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

char canData;
char dataMain[300];
char exportDataA[200];
char exportDataB[200];
String success;
int i;
int j;
int k;

const char* ssid = "KeshTest";
const char* password = "ehhnihao";
// 192.168.109.149


uint8_t broadcastAddress[] = {0xB8, 0xF0, 0x09, 0xC0, 0x1D, 0x00}; //TTGO

typedef struct struct_message {
  char nowData[202];
  int x;
} struct_message;

struct_message canReadings;

// Callback function for sending message via ESP-NOW
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("CAN Receiver");
  CAN.setPins(4, 5);

//-----------------------------------------------------------------------------------------------
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
//-----------------------------------------------------------------------------------------------
  //OTA Parameters
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
//-----------------------------------------------------------------------------------------------
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  // start esp now
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
//-----------------------------------------------------------------------------------------------
   // esp now peer parameters
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_send_cb(OnDataSent);
}
//-----------------------------------------------------------------------------------------------

void loop() {
  ArduinoOTA.handle();
  if (CAN.parsePacket()) {
    // received a packet
    Serial.print("Received ");
    Serial.print("packet 0x");
    Serial.print(CAN.packetId(),DEC);
    Serial.println("");
    
//    Serial.print(" of length ");
//    Serial.println(packetSize);
     }

    // Non-RTR
    while (CAN.available()) {
      canData = (char)CAN.read();
      //Serial.println(canData);
      strcat(dataMain,&canData);
 
      if (canData == '$') {
        Serial.println("Start of Data");
      }
      else if (canData == '#'){
        Serial.println("End of Data");
        //Serial.println(dataMain);
        for ( i=0, j=0; i < k;i++)
        {
          if (i < 200){
            memcpy(&exportDataA[i],&dataMain[i],1);
          }
          else{
            memcpy(&exportDataB[j],&dataMain[i],1);
            j++;
          }
        }
        k = 0;
        dataMain[0] = '\0';
        memcpy(&canReadings.nowData,&exportDataA,sizeof(exportDataA));
        canReadings.x = 1;
        Serial.println(canReadings.x);
        Serial.println(canReadings.nowData);
        if (canReadings.x == 1) {
          esp_err_t resultA = esp_now_send(broadcastAddress, (uint8_t *) &canReadings, sizeof(canReadings));  
          if (resultA == ESP_OK) {
          Serial.println("A sent with success");
        }
          else {
          Serial.println("Error sending A");
        }        
        }
        
        canReadings.nowData[0] = '\0';
        exportDataA[0] = '\0';

        memcpy(&canReadings.nowData,&exportDataB,sizeof(exportDataB));
        canReadings.x = 2; 
        Serial.println(canReadings.x);
        Serial.println(canReadings.nowData);        
        if (canReadings.x == 2) {
          esp_err_t resultB = esp_now_send(broadcastAddress, (uint8_t *) &canReadings, sizeof(canReadings));
          if (resultB == ESP_OK) {
          Serial.println("B sent with success");
        }
          else {
          Serial.println("Error sending B");
        }           
        }
        canReadings.nowData[0] = '\0';
        exportDataB[0] = '\0';                
        
                 
    
        }
        else {
          k++;
        }
     }   
   }
