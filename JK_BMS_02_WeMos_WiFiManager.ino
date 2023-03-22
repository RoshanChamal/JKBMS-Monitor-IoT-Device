//JK-BMS readings via RS485 : Monitor current, battery voltage, cell voltage and battery capacity
//Refer this for RS485 and ESP8266 connection details: https://www.mischianti.org/2020/05/11/interface-arduino-esp8266-esp32-rs-485/
//Refer this for C# code and debug reponse byte array: https://github.com/dj-nitehawk/jk-bms-test and this has website: https://github.com/dj-nitehawk/Hybrid-Inverter-Monitor

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <SoftwareSerial.h>

#define REDE D0

//#define _SS_MAX_RX_BUFF 255 // RX buffer size
SoftwareSerial mySerial(D1, D2); // RX, TX //used for JK-BMS - RS485

int battHighLimit = 35;
int battLowLimit = 30;
float chargingCurrent=0;  // battery current
float batteryVoltage=0;   // battery voltage
int batteryCapacity=0;    // battery capacity
float batteryCells[16];    //for 8 cell battery

//JK-BMS reading command
char dataCmd[] = {0x4E, 0x57, 0x00, 0x13 ,0x00 ,0x00, 0x00 ,0x00 ,0x06 ,0x03 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x68 ,0x00 ,0x00 ,0x01 ,0x29};


long lastReadMillis = 0; //will expire if MCU can't read values from BMS via RS485

void setup() {
  
  pinMode(REDE,OUTPUT);

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  mySerial.begin(115200);
  while (!mySerial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("JK-BMS Monitor Starting...");  
  delay(1000);

  //WIFIMANAGER - START
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();    
  wifiManager.autoConnect("Solar.JKBMS");
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  //WIFIMANAGER - END

  //init MQTT variables
}

void loop() {  

  RequestBMSData();
  
  //do you wifi request/ response here or MQTT code here

  delay(5000);
}

// JKBMS functions - Begin
void EnableRx(void)
{
  //DE/RE=LOW Receive Enabled
  digitalWrite(REDE,LOW);  
}

void EnableTx(void)
{
  ///DE/RE=HIGH Transmit Enabled
  digitalWrite(REDE,HIGH);  
}

int reqCount = 0, validCount = 0;
void RequestBMSData()
{
    char buff[268];
    int byteCounter = 0;

    Serial.println("Requesting ...");    
    EnableTx();
    delayMicroseconds(100);
    for(int i=0;i<21;i++)
    {
      mySerial.write(dataCmd[i]);
    }
    
    //mySerial.println(dataCmd, sizeof(dataCmd));
    //delayMicroseconds(100);
    EnableRx();
    delayMicroseconds(2000);

    long startTime = millis();
    //byteCounter = 0;
    int inByte = 0;
    while (millis() - startTime < 5000)
    {
      while(mySerial.available())
      {
        inByte = mySerial.read();
        buff[byteCounter] = inByte;
        //Serial.print(byteCounter);Serial.print(':');Serial.print(inByte, HEX);Serial.println(' ');
        byteCounter++;
        delayMicroseconds(5);
      }
      delayMicroseconds(1);
    }
        
    if(byteCounter > 0)
    {
      ProcessDataPacket(buff);
    }
    reqCount++;
    Serial.print("Data available count:");Serial.print(byteCounter);
    Serial.print(", Request count:");Serial.print(reqCount);
    Serial.print(", Valid count:");Serial.println(validCount);
}
void ProcessDataPacket(char buffData[268])
{
  //int pos = 11;
  //Serial.print(pos);Serial.print(':');Serial.print(buffData[pos], HEX);Serial.println(' ');

  if(buffData[0]==0x4E && buffData[1]==0x57 && buffData[11]==0x79)
  {
    Serial.println("Valid.");   
    
    int pos = 12;
    int cellCount = buffData[pos++]/3;
    Serial.print("cellCount:");Serial.println(cellCount);

    for(int b=1; b <= cellCount; b++)
    {
      int cellNumber =buffData[pos++];
      int read_cellVolt1 = buffData[pos++];
      //Serial.print("pos:");Serial.print(pos);Serial.print(' ');Serial.print(buffData[pos],HEX);Serial.print(' ');Serial.print(read_cellVolt1,HEX);
      int read_cellVolt2 = buffData[pos++];
      //Serial.print("pos:");Serial.print(pos);Serial.print(' ');Serial.print(buffData[pos],HEX);Serial.print(' ');Serial.print(read_cellVolt2,HEX);
      float cellVolt = (read_cellVolt2 + read_cellVolt1*256) * 0.001;
      Serial.print("cell:");Serial.print(cellNumber);Serial.print(' ');Serial.println(cellVolt);
      batteryCells[cellNumber - 1] = cellVolt;
    }

    int batteryCapacityX = (int)buffData[53];
    //if(batteryCapacityX >=0 && batteryCapacityX <=100) //0-100%
    {
      batteryCapacity = batteryCapacityX;
    }
    Serial.print("Capacity:");Serial.println(batteryCapacity);
    chargingCurrent = ((buffData[51] + buffData[50]*256) & 0xfff) * -0.01; 
    Serial.print("Current:");Serial.println(chargingCurrent);
    float batteryVoltageX = ((buffData[48] + buffData[47]*256) & 0xfff) * 0.01; 
    //if(batteryVoltageX > 0 && batteryVoltageX <= 30) //for 24V system
    {
      batteryVoltage = batteryVoltageX;
    }
    Serial.print("Voltage:");Serial.println(batteryVoltage);
    
    Serial.println();

    lastReadMillis = millis() + 30000; //advanced time with 30sec to avoid expire  for successful reads
    validCount++;
  }
  else{
    Serial.println("Not a valid response.");
  }
}
// JKBMS functions - End
