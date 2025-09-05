#include <LiquidCrystal_I2C.h>
#include <RTClib.h> 
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"
//#include <husarnet.h>

// Sttrings to join husarnet
/*#define HOSTNAME "esp32-arduino-webserver"
#define JOIN_CODE "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a/cdr74faaMF6HWY2W7RK5Ns"
*/
//HusarnetClient husarnet;
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
/*const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
*/
//Host name

//Variables to save values from HTML form
String ssid;
String pass;




// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
/*const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);
*/
// Timer variables
unsigned int previousMillis = 0;
const int interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

//String ledState;

// LCD inicialization
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 20 column and 4 rows
RTC_DS3231 rtc;
// LCD constants
//Valve outputs
const int open1stvalve = 32;      
const int open2ndvalve = 33;
//Manual buttons to open and close valves 
const int openclose1 = 19;
const int openclose2 = 18;
//Flags for openingclosing
volatile bool is1stopen = false;
volatile bool is2ndopen = false;
//Flags to indicate closing
volatile bool close1st = false;
volatile bool close2nd = false;
//Light output
const int Light = 25;
// Light input
const int turnLight =23;
// flag to turn on or off the light
volatile bool OnOffLight = false;
//Flags to reset what's written on the LCD
volatile bool ResetLCD1 = false;
volatile bool ResetLCD2= false;



// Sensor constants and variables
volatile int pulseCounter1;
volatile int pulseCounter2 ;

//Date variables
String year;
String month;
String day;
String hour;
String minute;
// YF-S201
const float factorK = 7.5;
const int stSensor = 34;
const int ndSensor = 35;
volatile float Liters1;
volatile float Liters2;

//Define max quantity of litters to feed
volatile int MaxToFeed= 10;
//Define hours where the thanks must be openned
volatile int HoursTank1[3] = {18,21,2};
volatile int HoursTank2[3] = {17,20,2};
//Define minutes when the thank must be openned
volatile int MinTank1[3] = {30,0,0};
volatile int MinTank2[3] = {30,0,0};
// Define the second when the tank must be openned
volatile int SecTank1[3] = {30,0,0};
volatile int SecTank2[3] = {30,0,0};
//Light alarm
volatile int HoursLight[2] ={20,0};
volatile int MinLight[2] = {0,0};
volatile int SecLight[2] = {0,0};
//Flags to send alarm value
volatile bool isAlarmchanged = false;




//---INTERRUPTION FUNCTIONS--------------

void CountPulses1()
{ 
  pulseCounter1++;  //incrementamos la variable de pulsos
} 
void CountPulses2()
{ 
  pulseCounter2++;  //incrementamos la variable de pulsos
} 
float GetFrequency1()
{
  int measureInterval = 500;
  pulseCounter1 =0;
  interrupts();
  delay(measureInterval);
  return (float)pulseCounter1 * 1000 / measureInterval;
}
float GetFrequency2()
{
  int measureInterval = 500;
  pulseCounter2 = 0;
  interrupts();
  delay(measureInterval);
  return (float)pulseCounter2 * 1000 / measureInterval;
}
//opens or closes 1st valve
void OpenRead1(){
    is1stopen = !is1stopen;
  }
//opens or closes 2nd valve
void OpenRead2(){
    is2ndopen = !is2ndopen;
  }
//turns on or of the light
void TurnLIGHT(){
  OnOffLight =!OnOffLight;
}
//-------------------------------MAIN FUNCTION-----------------------------------  
void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();// Configuramos las filas y las columnas del LCD en este caso 16 columnas y 2 filas
  //initializes outputs and inputs
  pinMode(open1stvalve, OUTPUT);
  pinMode(open2ndvalve, OUTPUT);
  pinMode(Light,OUTPUT);
  pinMode(openclose1, INPUT);
  pinMode(openclose2, INPUT);
  pinMode(stSensor, INPUT);
  pinMode(ndSensor, INPUT);
  pinMode(turnLight,INPUT);

  digitalWrite(open1stvalve,LOW);
  digitalWrite(open2ndvalve,LOW);
  digitalWrite(Light,LOW);

  //Initializes interruptions for buttons;
  attachInterrupt(digitalPinToInterrupt(openclose1), OpenRead1, FALLING);
  attachInterrupt(digitalPinToInterrupt(openclose2), OpenRead2, FALLING);
  attachInterrupt(digitalPinToInterrupt(turnLight), TurnLIGHT, FALLING);
  attachInterrupt(digitalPinToInterrupt(stSensor), CountPulses1, RISING);
  attachInterrupt(digitalPinToInterrupt(ndSensor), CountPulses2, RISING);
  //Serial to debug dont forget to take it out on the final version
  Serial.begin(115200);
  // variable asignment to perform later calculations on the amount of food fed

  Liters1 = 0;
  Liters2=0;
  Wire.begin();
  //-----------RTC module setup----------------------
   if (! rtc.begin()) {
    Serial.println("RTC module is NOT found");
    Serial.flush();
    while (1);
  }    
         if(rtc.lostPower()) {
        // this will adjust to the date and time at compilation
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    //we don't need the 32K Pin, so disable it
    rtc.disable32K(); 
  //-----------------------Web server initialization------------------------------------
   initLittleFS();

  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  // ip = readFile(LittleFS, ipPath);
  // gateway = readFile (LittleFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  // Serial.println(ip);
  // Serial.println(gateway);

  if(initWiFi()) {
    
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    server.serveStatic("/", LittleFS, "/");
    
    // Route to set first valve state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      is1stopen = true;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    // Route to set first valve state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      is1stopen = false;
     request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    // Route to set second valve state to HIGH
    server.on("/on2", HTTP_GET, [](AsyncWebServerRequest *request) {
      is2ndopen = true;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    // Route to set second valve state to LOW
    server.on("/off2", HTTP_GET, [](AsyncWebServerRequest *request) {
      is2ndopen = false;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    //Route to set light on
    server.on("/on3", HTTP_GET, [](AsyncWebServerRequest *request) {
      OnOffLight = true;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
     //Route to set light of
    server.on("/off3", HTTP_GET, [](AsyncWebServerRequest *request) {
      OnOffLight = false;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    // Handle Web Server Events
    events.onConnect([](AsyncEventSourceClient *client){
      if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      // send event with message "hello!", id current millis
      // and set reconnect delay to 1 second
      String ipadd = WiFi.localIP().toString(); 
      events.send(ipadd.c_str(),"getipaddress",millis());
      isAlarmchanged = true;
    });
   
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {

      int params = request->params();
      for(int i=0;i<params;i++){

        const AsyncWebParameter* p = request->getParam(i);
        Serial.println(p->name());
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
           
          // HTTP POST year value
          if (p->name() == "year") {
            year=p->value().c_str();
          }
          // HTTP POST pass value
          if (p->name() == "month") {
            month=p->value();
            
          }
          if (p->name() == "day") {
            day= p->value();
           
          }
          if (p->name() == "hour") {
            hour=p->value();
            
          }
          if (p->name() == "min") {
            minute= p->value();
            rtc.adjust(DateTime(atoi(year.c_str()),atoi(month.c_str()),atoi(day.c_str()),atoi(hour.c_str()),atoi(minute.c_str()),0));
            Serial.println("Time adjusted to:");
            Serial.println(year);Serial.println(month);Serial.println(day);Serial.println(hour);Serial.println(minute);
          }
          if (p->name()== "changealarm1.1"){
            HoursTank1[0] = p->value().toInt();
            Serial.println("Time adjusted to:");
            Serial.println(HoursTank1[0]);

          }
          
        }
      }
      request->send(200, "/index.html", "text/html");
    });
   
    server.addHandler(&events);
    server.begin();
    String ipadd = WiFi.localIP().toString(); 
    events.send(ipadd.c_str(),"getipaddress",millis());
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          /*if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }*/
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: ");
      delay(3000);
      ESP.restart();
    });
     // Join the Husarnet network
  /*husarnet.join(HOSTNAME, JOIN_CODE);

  while(!husarnet.isJoined()) {
    Serial.println("Waiting for Husarnet network...");
    delay(1000);
  }
  Serial.println("Husarnet network joined");

  Serial.print("Husarnet IP: ");
  Serial.println(husarnet.getIpAddress().c_str());*/

    server.begin();
  }
}

 
void loop() {
  
  float frequency1;
  float frequency2;
  float caudal_L_m1;
  float caudal_L_m2;
  float dt1;
  float dt2;
  float t01;
  float t02;
  t01 = millis();
  t02 = millis();
  //Serial.println("OK");
  //Reads the time from the RTC module
  DateTime now = rtc.now();
  char buf1[] = "YYYY-MM-DDThh:mm:ss";
  events.send(String(now.toString(buf1)).c_str(),"time",millis());
  if (isAlarmchanged){
    char buf2 [5];

    
      sprintf(buf2, "%02d:%02d", HoursTank1[0],MinTank1[0]);
      events.send(buf2,"alarm1_1",millis());
      Serial.println("alarm1_1");
      sprintf(buf2, "%02d:%02d", HoursTank1[1],MinTank1[1]);
      events.send(buf2,"alarm1_2",millis());
      Serial.println("alarm1_2");
      sprintf(buf2, "%02d:%02d", HoursTank1[2],MinTank1[2]);
      events.send(buf2,"alarm1_2",millis());
      Serial.println("alarm1_2");
    //Alarm 2
      sprintf(buf2, "%02d:%02d", HoursTank2[0],MinTank2[0]);
      events.send(buf2,"alarm2_1",millis());
      sprintf(buf2, "%02d:%02d", HoursTank2[1],MinTank2[1]);
      events.send(buf2,"alarm2_2",millis());
      sprintf(buf2, "%02d:%02d", HoursTank2[2],MinTank2[2]);
      events.send(buf2,"alarm2_3",millis());

     //Alarm 3
      sprintf(buf2, "%02d:%02d", HoursLight[0],MinLight[0]);
      events.send(buf2,"alarm3_1",millis());
      sprintf(buf2, "%02d:%02d", HoursLight[1],MinLight[1]);
      events.send(buf2,"alarm3_2",millis());
      
    isAlarmchanged = false;
  }

  //checks light flag to turn it off or on
  if(OnOffLight||now.hour() == HoursLight[0]&& now.minute()==MinLight[0]&& now.second()==SecLight[0]||now.hour() == HoursLight[1]&& now.minute()==MinLight[1]&& now.second()==SecLight[1]){
    digitalWrite(ledPin,HIGH);
    digitalWrite(Light,HIGH);
    events.send("Encendida","LightState",millis());
  }else if(!OnOffLight){
    digitalWrite(ledPin,LOW);
    digitalWrite(Light,LOW);
    events.send("Apagada","LightState",millis());
  }
  //checks the time and sets the flag to open the especific thank
  //First tank schedule
  if(now.hour() == HoursTank1[0] && now.minute()== MinTank1[0] && now.second() == SecTank1[0]||now.hour() == HoursTank1[1] && now.minute()== MinTank1[1] && now.second() == SecTank1[1]||now.hour() == HoursTank1[2] && now.minute()== MinTank1[2] && now.second() == SecTank1[2] ){
      is1stopen = true;
      //Sends the event to change the page label
      events.send(String("Abierta").c_str(),"valve1",millis());
    }
  //Second tank schedule
  if(now.hour() == HoursTank2[0] && now.minute()== MinTank2[0] && now.second() == SecTank2[0]||now.hour() == HoursTank2[1] && now.minute()== MinTank2[1] && now.second() == SecTank2[1]||now.hour() == HoursTank2[2] && now.minute()== MinTank2[2] && now.second() == SecTank2[2] ){
      is2ndopen = true;
      //Sends the event to change the page label
      events.send(String("Abierta").c_str(),"valve2",millis());
    }
    
  //Idle mode
 if(!is1stopen && !is2ndopen && !close1st && !close2nd){
    //Sends the event to change the page label
    events.send(String("Cerrada").c_str(),"valve1",millis());
    events.send(String("Cerrada").c_str(),"valve2",millis());
    //Sends the event to monitor the depposited food
    events.send(String(Liters1).c_str(),"deposited1",millis());
    events.send(String(Liters2).c_str(),"deposited2",millis());
    digitalWrite(open1stvalve,LOW);
    digitalWrite(open2ndvalve,LOW);
    lcd.setCursor(0,0);
    lcd.print("Tanques cerrados");
    lcd.setCursor(0,1); 
    char buf1[] = "YYYY-MM-DDThh:mm:ss";
    
    lcd.print(String(now.toString(buf1))+String("           "));
    ResetLCD1 = true;
    ResetLCD2 = true;
    
    Liters1 =0;
    Liters2 =0;
      //Serial.println(is1stopen);
   }
    //closing first tank mode
     else if (close1st && !is1stopen ||Liters1>= MaxToFeed){
      //Sends the event to change the page label
      events.send(String("Cerrada").c_str(),"valve1",millis());
      digitalWrite(open1stvalve,LOW);
      close1st = false;   //Enters here when Feed Limit is reached or the button to close is pressed and reset the corresponding flags
      is1stopen = false;
      lcd.setCursor(0,0);
      lcd.print("Cerrando T1     ");
      ResetLCD2 = true;
      delay(500);
      Liters1 =0;
      
    }
    //closing second tank mode
     else if (close2nd && !is2ndopen||Liters2>= MaxToFeed){
      //Sends the event to change the page label
      events.send(String("Cerrada").c_str(),"valve2",millis());
      digitalWrite(open2ndvalve,LOW);
      //Sends the event to monitor the depposited food
      events.send(String(Liters2).c_str(),"deposited2",millis());
      close2nd = false; //Enters here when Feed Limit is reached or the button to close is pressed and reset the corresponding flags
      is2ndopen= false;
      ResetLCD1 = true;
      lcd.setCursor(0,1);
      lcd.print("Cerrando T2    ");
      delay(500);
      Liters2 =0;
      
    }
    //Both tanks openned simultaneously
    else if(is1stopen && is2ndopen&& Liters1<= MaxToFeed&& Liters2<= MaxToFeed){
//    Cleans LCD
    if(ResetLCD1&&ResetLCD2){
        clearLCD();
        ResetLCD1 = false;
        ResetLCD2 = false;
    }
        //Sends the event to change the page label
    events.send(String("Abierta").c_str(),"valve1",millis());
        //Sends the event to change the page label
    events.send(String("Abierta").c_str(),"valve2",millis());
    //Sends the event to monitor the depposited food
    events.send(String(Liters1).c_str(),"deposited1",millis());
    events.send(String(Liters2).c_str(),"deposited2",millis());
    //opens the first valve
     digitalWrite(open1stvalve,HIGH);
     //opens the second valve
     digitalWrite(open2ndvalve,HIGH);
     //displays on the LCD the first and second valve fed ammount
     lcd.setCursor(0,0);
     lcd.print("T1:  ");
     lcd.setCursor(0,1);
     lcd.print("T2: ");
     close1st = true;
     close2nd = true;
    //Starts reading the first flux sensor value
    
      frequency1 = GetFrequency1();
      caudal_L_m1=frequency1/factorK; //calculates the caudal
      dt1 = millis()-t01;
      t01 = millis();
      Liters1 = Liters1+(caudal_L_m1/60)*(dt1/1000); //calculates the volume of the liquid
      lcd.setCursor(5,0);
      String result = String(Liters1);
      lcd.print(result+"L");  
    //Starts reading the second flux sensor value
      frequency2 = GetFrequency2();
      caudal_L_m2=frequency2/factorK; //calculamos el caudal en L/m
      dt2 = millis()-t02;
      t02 = millis();
      Liters2 = Liters2+(caudal_L_m2/60)*(dt2/1000); 
      lcd.setCursor(5,1);
      result = String(Liters2);
      lcd.print(result+"L");   
    }
    //only the first tank is open
  else if(is1stopen&& Liters1<= MaxToFeed){
    //Serial.println("¡Botón1 presionado!");
    //Sends the event to change the page label
    events.send(String("Abierta").c_str(),"valve1",millis());
     //Sends the event to monitor the depposited food
    events.send(String(Liters1).c_str(),"deposited1",millis());
    //opens the first valve
    digitalWrite(open1stvalve,HIGH);
    if(!close1st||ResetLCD1){
    //cleans what was previously written on the LCD only once 
     clearLCD();
     lcd.setCursor(0,0);
     lcd.print("T1: ");
     close1st = true;
     ResetLCD1 = false;
     //set the flag in case it entered the condition with the time scheduled
     is1stopen = true;
    }
    //Starts reading the flux sensor value
      frequency1 = GetFrequency1();
      caudal_L_m1=frequency1/factorK; //calculamos el caudal en L/m
      dt1 = millis()-t01;
      t01 = millis();
      //calculates the deposited volume
      Liters1 = Liters1+(caudal_L_m1/60)*(dt1/1000);
      lcd.setCursor(5,0);
      String result = String(Liters1);
      lcd.print(result+"L");
         
    }
   //Only the second tank is open
   else if(is2ndopen&& Liters2<= MaxToFeed){
    //Serial.println("¡Botón2 presionado!");
    //opens the second valve
        //Sends the event to change the page label
    events.send(String("Abierta").c_str(),"valve2",millis());
     //Sends the event to monitor the depposited food
    events.send(String(Liters2).c_str(),"deposited2",millis());
    digitalWrite(open2ndvalve,HIGH);
    if(!close2nd||ResetLCD2){
      //cleans what was previously written on the LCD only once
      clearLCD();
      lcd.setCursor(0,1);
      lcd.print("T2: ");
      close2nd = true;
      ResetLCD2 = false;
      //set the flag in case it entered the condition with the time scheduled
      is2ndopen =true;
    }
    //Starts reading the flux sensor value

      frequency2 = GetFrequency2();
     caudal_L_m2=frequency2/factorK; //calculamos el caudal en L/m
      dt2 = millis()-t02;
      t02 = millis();
      //calculates the deposited volume
      Liters2 = Liters2+(caudal_L_m2/60)*(dt2/1000); 
      lcd.setCursor(5,1);
      String result = String(Liters2);
      lcd.print(result+"L");
     
    }
}

void clearLCD (){
  
     lcd.setCursor(0,0);
     lcd.print("                ");
     lcd.setCursor(0,1);
     lcd.print("                ");
  }

//----------------WIFI functions---------------------
// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid==""){
    Serial.println("Undefined SSID ");
    return false;
  }

  /*WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }*/
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());

  return true;
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  /*if(var == "STATE") {
    if(digitalRead(open1stvalve)) {
      ledState = "Abierta";
    }
    else {
      ledState = "Cerrada";
    }
    
  }
  else if(var == "STATE2") {
      if(digitalRead(open2ndvalve)) {
        ledState = "Abierta";
      }
      else {
        ledState = "Cerrada";
      }
      
  }*/
  //return ledState;
  return String();
}
