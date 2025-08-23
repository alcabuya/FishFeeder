#include <LiquidCrystal_I2C.h>
#include <RTClib.h> 
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"

AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
/*const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
*/
//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

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
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;

// LCD inicialization
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 20 column and 4 rows
RTC_DS3231 rtc;

 
// LCD constants
#define COLS 16 // LCD columns
#define ROWS 2 // LCD rows
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

//Flags to reset what's written on the LCD
volatile bool ResetLCD1 = false;
volatile bool ResetLCD2= false;

// Sensor constants and variables
const int measureInterval = 500;
volatile int pulseConter1;
volatile int pulseCounter2;

// YF-S201
const float factorK = 7.5;
const int stSensor = 34;
const int ndSensor = 35;
volatile float frequency1;
volatile float frequency2;
volatile float caudal_L_m1;
volatile float caudal_L_m2;
volatile float Liters1;
volatile float Liters2;
volatile float dt1;
volatile float dt2;
volatile float t01;
volatile float t02;
const int MaxToFeed= 10;
//Define hours where the thanks must be openned
const int HoursTank1[2] = {18,21};
const int HoursTank2[2] = {17,20};
//Define minutes where the thank must be openned
const int MinTank1[2] = {30,0};
const int MinTank2[2] = {30,0};
// Define the second where the tank must be openned
const int SecTank1[2] = {30,0};
const int SecTank2[2] = {30,0};

//---INTERRUPTION FUNCTIONS--------------

void CountPulses1()
{ 
  pulseConter1++;  //incrementamos la variable de pulsos
} 
void CountPulses2()
{ 
  pulseCounter2++;  //incrementamos la variable de pulsos
} 
float GetFrequency1()
{
  pulseConter1 = 0;
  interrupts();
  delay(measureInterval);
  return (float)pulseConter1 * 1000 / measureInterval;
}
float GetFrequency2()
{
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

//-------------------------------MAIN FUNCTION-----------------------------------  
void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();// Configuramos las filas y las columnas del LCD en este caso 16 columnas y 2 filas
  //initializes outputs and inputs
  pinMode(open1stvalve, OUTPUT);
  pinMode(open2ndvalve, OUTPUT);
  pinMode(openclose1, INPUT);
  pinMode(openclose2, INPUT);
  pinMode(stSensor, INPUT);
  pinMode(ndSensor, INPUT);
  digitalWrite(open1stvalve,LOW);
  digitalWrite(open2ndvalve,LOW);
  //Initializes interruptions for buttons;
  attachInterrupt(digitalPinToInterrupt(openclose1), OpenRead1, FALLING);
  attachInterrupt(digitalPinToInterrupt(openclose2), OpenRead2, FALLING);
  attachInterrupt(digitalPinToInterrupt(stSensor), CountPulses1, RISING);
  attachInterrupt(digitalPinToInterrupt(ndSensor), CountPulses2, RISING);
  //Serial to debug dont forget to take it out on the final version
  Serial.begin(115200);
  // variable asignment to perform later calculations on the amount of food fed
  t01 = millis();
  t02 = millis();
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
    
    // Route to set GPIO state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      is1stopen = true;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      is1stopen = false;
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    server.begin();
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
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
}

 
void loop() {
  //Serial.println("OK");
  //Reads the time from the RTC module
  DateTime now = rtc.now();
  /*Serial.print("ESP32 RTC Date Time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(now.dayOfTheWeek());
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);*/
  //checks the time and sets the flag to open the especific thank
  //First tank schedule
  if(now.hour() == HoursTank1[0] && now.minute()== MinTank1[0] && now.second() == SecTank1[1]||now.hour() == HoursTank1[1] && now.minute()== MinTank1[1] && now.second() == SecTank1[1] ){
      is1stopen = true;
    }
  //Second tank schedule
  if(now.hour() == HoursTank2[0] && now.minute()== MinTank2[0] && now.second() == SecTank2[1]||now.hour() == HoursTank2[1] && now.minute()== MinTank2[1] && now.second() == SecTank2[1] ){
      is2ndopen = true;
    }
    
  //Idle mode
 if(!is1stopen && !is2ndopen && !close1st && !close2nd){
    digitalWrite(open1stvalve,LOW);
    digitalWrite(open2ndvalve,LOW);
    lcd.setCursor(0,0);
    lcd.print("Tanques cerrados");
    lcd.setCursor(0,1); 
    char buf1[] = "hh:mm";
    
    lcd.print(String(now.toString(buf1))+String("           "));
    ResetLCD1 = true;
    ResetLCD2 = true;
    
    Liters1 =0;
    Liters2 =0;
      //Serial.println(is1stopen);
   }
    //closing first tank mode
     else if (close1st && !is1stopen ||Liters1>= MaxToFeed){
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
      digitalWrite(open2ndvalve,LOW);
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
    //Create loop to meassure the ammount of food depposited on the thank
//Replace delay with that and don't forget to reset the open flag
     // Serial.println("¡Botón1 presionado!");
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
  if(var == "STATE") {
    if(digitalRead(open1stvalve)) {
      ledState = "ON";
    }
    else {
      ledState = "OFF";
    }
    return ledState;
  }
  return String();
}