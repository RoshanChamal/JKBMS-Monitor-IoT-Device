//JK-BMS readings via RS485 : Monitor current, battery voltage, cell voltage and battery capacity
//Refer this for RS485 and ESP8266 connection details: https://www.mischianti.org/2020/05/11/interface-arduino-esp8266-esp32-rs-485/
//Refer this for C# code and debug reponse byte array: https://github.com/dj-nitehawk/jk-bms-test and this has website: https://github.com/dj-nitehawk/Hybrid-Inverter-Monitor

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>

#include <SoftwareSerial.h>

#define REDE D0

#define CONNECT_STS_PIN   D4
#define RELAY_AC_PIN      D7

SoftwareSerial mySerial(D1, D2); // RX, TX //used for JK-BMS - RS485

int battHighLimit       = 30;
int battLowLimit        = 25;
float chargingCurrent   = 0;  // battery current
float batteryVoltage    = 0;   // battery voltage
int batteryCapacity     = 0;    // battery capacity
float batteryCells[16];    //for 8 cell battery

int finalBatteryCapacity = -1;  //to get avergae of capacity to determine reading is correct or not
char dataCmd[] = {0x4E, 0x57, 0x00, 0x13 ,0x00 ,0x00, 0x00 ,0x00 ,0x06 ,0x03 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x68 ,0x00 ,0x00 ,0x01 ,0x29};


#define CLIENT_ID "home/jkbms/01"
char mqtt_server[40] = "broker.hivemq.com";
char mqtt_port[6] = "1883";
WiFiClient espClient;
PubSubClient client(espClient);

long dataInterval = 5000;//in every 5sec
long previousBMSMillis = 0;
long lastReadMillis = 0; //will expire if MCU can't read values from BMS via RS485
long lastResetMillis = millis(); //to avoid starting reboot
long previousHBMillis = 0;

void(*reset) (void) = 0;
void resetArudino(){
  Serial.println(F("Reset MCU"));
  delay(1000);
  reset();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
}

void setup() {
  
  pinMode(REDE,OUTPUT);
  
  pinMode(CONNECT_STS_PIN, OUTPUT);
  digitalWrite(CONNECT_STS_PIN, HIGH); //this to indicate LED is working //D6
  
  pinMode(RELAY_AC_PIN, OUTPUT);
  digitalWrite(RELAY_AC_PIN, LOW); //to relay will be connected to NC to avoid drop inv. out ac supply ready

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  mySerial.begin(115200);
  while (!mySerial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("JK-BMS Monitor Starting...");  
  delay(2000);
  digitalWrite(CONNECT_STS_PIN, LOW); //set to OFF

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

  //init MQTT 
  Serial.println("Connecting to MQTT server");  
  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
  Serial.print("MQTT Broker Server : ");
  Serial.println(mqtt_server);
  Serial.print("MQTT Broker Port : ");
  Serial.println(mqtt_port);
}

void loop() {  
  
  //do you wifi request/ response here or MQTT code here
  //mqtt start
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //mqtt end  
  long currentMillis = millis();    
  if(currentMillis - previousBMSMillis >= dataInterval) // execute once every 5 second, don't flood remote service
  {
    RequestBMSData();

    //send readigs to mqtt server
    if(lastReadMillis > currentMillis)
    {
      // publish temperature to topic
      String outData = "[[JKBMS:100:0^" + String(batteryVoltage)       
      + ",1^" + String(batteryCells[0])
      + ",2^" + String(batteryCells[1])
      + ",3^" + String(batteryCells[2])
      + ",4^" + String(batteryCells[3])
      + ",5^" + String(batteryCells[4])
      + ",6^" + String(batteryCells[5])
      + ",7^" + String(batteryCells[6])
      + ",8^" + String(batteryCells[7])
      + ",9^" + String(batteryCapacity)
      + ",10^" + String(digitalRead(RELAY_AC_PIN)==LOW ? "1" : "0")
      + "]]";

      String topic("/");
      topic += CLIENT_ID;
      topic += "/output";   

      client.publish(topic.c_str(), outData.c_str());
      Serial.print("Send IOT Data:");Serial.println(outData);

      //Controlling Relay Pin
      if(finalBatteryCapacity != -1 && finalBatteryCapacity > battHighLimit)
      {
        digitalWrite(RELAY_AC_PIN, LOW);
        Serial.println(">>>>> RELAY_AC_PIN: LOW - AC OUT READY");
      }
      else if(finalBatteryCapacity != -1 && finalBatteryCapacity <= battLowLimit)
      {
        digitalWrite(RELAY_AC_PIN, HIGH);
        Serial.println("<<<<< RELAY_AC_PIN: HIGH - AC OUT STOPPED");
      }      
    }    
    //update next read time
    previousBMSMillis = currentMillis;
  }
  //heart beat
  if(currentMillis - previousHBMillis >= 60000)
  {    
    String topic("/");
    topic += CLIENT_ID;
    topic += "/status";  
    String valueStr(millis());  
    // publish value to topic
    client.publish(topic.c_str(), valueStr.c_str());
    Serial.print("HB DATA:");Serial.println(valueStr);

    //update next time
    previousHBMillis = currentMillis;
  }
  //restart MCU
  if(currentMillis - lastResetMillis > 1800000)
  {
    resetArudino(); //reset arduino after 30min
  }  
  delay(500);
}

bool initialized = false;
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    digitalWrite(CONNECT_STS_PIN, LOW); //set to OFF
    
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_ID)) {
      if (initialized == false){
        Serial.println("connected");
        initialized = true;
      }
      else{
        Serial.println("reconnected");
      }
      // ... and resubscribe
      //Serial.print("subscribing to: "); Serial.println(inTopic);
      //client.subscribe(inTopic);//, MQTTQOS1);
      previousHBMillis = 0; //send heart beat when mqtt connected
      digitalWrite(CONNECT_STS_PIN, HIGH); //set to ON  
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for (int i = 0; i<5000; i++){
        delay(0);
      }
    }
  }
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
    Serial.println("Requesting ...");
    char buff[268];
    int byteCounter = 0;    
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
    //read data
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

int lastBatteryCapacity = -1;  //to get avergae of capacity to determine reading is correct or not

void ProcessDataPacket(char buffData[268])
{
  //int pos = 11;
  //Serial.print(pos);Serial.print(':');Serial.print(buffData[pos], HEX);Serial.println(' ');

  
  if(buffData[0]==0x4E && buffData[1]==0x57 && buffData[11]==0x79)
  {    
    Serial.println("Valid.");   
    bool isValid = true;    
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
    Serial.print("Capacity(raw):");Serial.println(batteryCapacityX);
    if(batteryCapacityX > 0 && batteryCapacityX <=100) //0-100%
    {      
      batteryCapacity = batteryCapacityX;
      //cross check reading      
      if(lastBatteryCapacity == -1)
      {
        lastBatteryCapacity = batteryCapacity;
      }
      else if(lastBatteryCapacity != -1)
      {
        if(abs(lastBatteryCapacity - batteryCapacity) != 0 && abs(lastBatteryCapacity - batteryCapacity) > 5) //difference should be less than with previous reading
        {
          lastBatteryCapacity = -1;
        }
        else if(abs(lastBatteryCapacity - batteryCapacity) == 0)
        {
          finalBatteryCapacity = batteryCapacity; //used this value for Relay on/off
          lastBatteryCapacity = batteryCapacity; //update last value for next check
        }
      }      
    }
    else
    {
      isValid = false;
    }
    Serial.print("Capacity(Modified):");Serial.println(batteryCapacity);    
    chargingCurrent = ((buffData[51] + buffData[50]*256) & 0xfff) * -0.01; 
    Serial.print("Current:");Serial.println(chargingCurrent);
    float batteryVoltageX = ((buffData[48] + buffData[47]*256) & 0xfff) * 0.01; 
    Serial.print("Voltage(raw):");Serial.println(batteryVoltageX);    
    if(batteryVoltageX > 0 && batteryVoltageX <= 30) //for 24V system
    {
      batteryVoltage = batteryVoltageX;
    }
    else
    {
      isValid = false;
    }
    Serial.print("Voltage(Modified):");Serial.println(batteryVoltage);    
    Serial.println();

    if(isValid)
    {
      lastReadMillis = millis() + 10000; //advanced time with 30sec to avoid expire  for successful reads
    }
    else
    {
      lastReadMillis = 0;//reset to  avoid values via mqtt
    }
    validCount++;
  }
  else{
    Serial.println("Not a valid response.");
  }
}
// JKBMS functions - End
