#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
#include "images.h"
#include <TinyGPS++.h>
#include <axp20x.h>
#include <string.h>
#include <stdlib.h>


#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;

SSD1306 display(0x3c, 21, 22);
String rssi = "RSSI --";
String packSize = "--";
String packet ;

#define LORA_MESSAGE_FORMAT_COUNTER_LINE_INDEX 0
#define LORA_MESSAGE_FORMAT_DATETIME_LINE_INDEX 1
#define LORA_MESSAGE_FORMAT_LATITUDE_LINE_INDEX 2
#define LORA_MESSAGE_FORMAT_LONGITUDE_LINE_INDEX 3
#define LORA_MESSAGE_FORMAT_LINES_MAX 4

void loraData(){

  Serial.println("LoRa received:");
  Serial.print(packet);

  //we receive one chunk of data per line: packet counter|date-time (ISO-8601)|lat|lng
  char copy[packet.length() + 1];
  strcpy(copy, packet.c_str());

  char* lines[LORA_MESSAGE_FORMAT_LINES_MAX];
  int i = 0;
  char* line = strtok(copy, "\r\n");
  while(line != NULL && ++i < LORA_MESSAGE_FORMAT_LINES_MAX)
    line = strtok(NULL, "\r\n");
  }

  if(i != LORA_MESSAGE_FORMAT_LINES_MAX) {
    Serial.print("Invalid packet format, not enough lines: ");
    Serial.println(i);
    return;
  }

  int counter = strtol(lines[LORA_MESSAGE_FORMAT_COUNTER_LINE_INDEX], NULL, 10);
  double latitude = strod(lines[LORA_MESSAGE_FORMAT_LATITUDE_LINE_INDEX], NULL);
  double longitude = strod(lines[LORA_MESSAGE_FORMAT_LONGITUDE_LINE_INDEX], NULL);

  //calculate distance in meters
  double distance = -1;
  if(gps.location.isValid()) {
    distance = TinyGPSPlus.distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      latitude,
      longitude
    );
  }

  String strDistance = String(distance, 1) + "m";

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  //display.drawString(64 , 20 , "Received "+ packSize + " bytes");
  display.drawStringMaxWidth(64 , 40 , 128, strDistance);
  display.drawString(64, 0, rssi);

  display.display();
  Serial.println(rssi);
  Serial.println("******************");
  Serial.println("");
}

void cbk(int packetSize) {
  packet ="";
  packSize = String(packetSize,DEC);
  for (int i = 0; i < packetSize; i++) { packet += (char) LoRa.read(); }
  rssi = "RSSI " + String(LoRa.packetRssi(), DEC) ;
  loraData();
}

void setup() {
  pinMode(16,OUTPUT);
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high、

Serial.begin(115200);
  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  GPS.begin(9600, SERIAL_8N1, 12, 34);   //17-TX 18-RX


  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("LoRa Receiver Callback");
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);  
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  //LoRa.onReceive(cbk);
  // LoRa.receive();
  Serial.println("init ok");
  display.init();
  display.flipScreenVertically();  
  display.setFont(ArialMT_Plain_10);
   
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_24);
  display.drawString(64 , 0 , "CTL");
  display.drawString(64 , 20 , "Receiver");
  display.drawString(64 , 40 , "Ready!");
  display.display();
  
  delay(1500);

}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) { cbk(packetSize);  }
  delay(10);
Serial.print("Latitude  : ");
  Serial.println(gps.location.lat(), 5);
  Serial.print("Longitude : ");
  Serial.println(gps.location.lng(), 4);
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("Altitude  : ");
  Serial.print(gps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Time      : ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  Serial.print("Speed     : ");
  Serial.println(gps.speed.kmph()); 
  Serial.println("**********************");

  smartDelay(1000);

  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      gps.encode(GPS.read());
  } while (millis() - start < ms);

  
}
