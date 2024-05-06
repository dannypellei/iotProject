#include "LoRaWan_APP.h"
#include <Arduino.h>
#include <TinyGPS++.h>


// Constants for GPS (Updated Pin Assignments)
const int RX_PIN = 6;  //was 44
const int TX_PIN = 5;  //was 43 
TinyGPSPlus gps;
HardwareSerial SerialGPS(1);

uint8_t devEui[] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0x64, 0x2D}; 
bool overTheAirActivation = true;
uint8_t appEui[] = {0x42, 0x04, 0x20, 0x42, 0x04, 0x20, 0x42, 0x04}; 
uint8_t appKey[] = {0xEB, 0x92, 0xD2, 0xA6, 0x7C, 0xEE, 0x0F, 0x25, 0x38, 0xDC, 0x7B, 0xD2, 0x98, 0x61, 0x9C, 0xE6};


//These are only used for ABP, for OTAA, these values are generated on the Nwk Server, you should not have to change these values
uint8_t nwkSKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t appSKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint32_t devAddr =  (uint32_t)0x00000000; 


/*LoraWan channelsmask*/
uint16_t userChannelsMask[6]={ 0xFF00,0x0000,0x0000,0x0000,0x0000,0x0000 };


// LoRaWAN operational settings
uint32_t appTxDutyCycle = 15000;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 1;
uint8_t confirmedNbTrials = 8;
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;

// RTC and first run detection
RTC_DATA_ATTR bool firstrun = true;





//----------------------------------------ADDED-------------------------------------------


void setupGPS() {
  SerialGPS.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  Serial.println("GPS Module Initialized");
}


static void prepareTxFrame(uint8_t port) {
  if (gps.location.isValid()) {
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

    // Print the latitude and longitude to the Serial monitor
    Serial.print("Valid GPS Data: Latitude = ");
    Serial.print(latitude, 6); // Print with 6 decimal places for precision
    Serial.print(", Longitude = ");
    Serial.println(longitude, 6);

    uint8_t latBytes[sizeof(float)], lonBytes[sizeof(float)];
    memcpy(latBytes, &latitude, sizeof(float));
    memcpy(lonBytes, &longitude, sizeof(float));

    appDataSize = 1 + 2 * sizeof(float);
    appData[0] = 0x04; // Group number 4

    for (int i = 0; i < sizeof(float); i++) {
      appData[i + 1] = latBytes[i];
      appData[i + 1 + sizeof(float)] = lonBytes[i];
    }
  } else {
    Serial.println("GPS data not valid or waiting for fix");
  }
}









void setup() {
  Serial.begin(115200);
  setupGPS();
  Mcu.begin();

  if (firstrun) {
    LoRaWAN.displayMcuInit();
    firstrun = false;
  }
  deviceState = DEVICE_STATE_INIT;
}










void loop()
{
   switch( deviceState )
   {
       case DEVICE_STATE_INIT:
       {
#if(LORAWAN_DEVEUI_AUTO)
           LoRaWAN.generateDeveuiByChipID();
#endif
           LoRaWAN.init(loraWanClass,loraWanRegion);
           break;
       }
       case DEVICE_STATE_JOIN:
       {
     LoRaWAN.displayJoining();
           LoRaWAN.join();
           if (deviceState == DEVICE_STATE_SEND){
               LoRaWAN.displayJoined();
           }
           break;
       }
       case DEVICE_STATE_SEND:
      LoRaWAN.displaySending();
      if (SerialGPS.available()) {
        while (SerialGPS.available() > 0) {
          gps.encode(SerialGPS.read());
        } 
        if (gps.location.isValid()) { 
          prepareTxFrame(appPort); 
          LoRaWAN.send();
        } else {
          Serial.println("GPS data not valid or waiting for fix");
        }        
      }
      deviceState = DEVICE_STATE_CYCLE;
      break;
       case DEVICE_STATE_CYCLE:
       {
           // Schedule next packet transmission
           txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
           LoRaWAN.cycle(txDutyCycleTime);
           deviceState = DEVICE_STATE_SLEEP;
           break;
       }
       case DEVICE_STATE_SLEEP:
       {
           LoRaWAN.displayAck();
           LoRaWAN.sleep(loraWanClass);
           break;
       }
       default:
       {
           deviceState = DEVICE_STATE_INIT;
           break;
       }
   }
}
