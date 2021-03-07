#include <ThingSpeak.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DQ_pin 4 
#define SENSOR A0  //pH meter Analog output to Arduino Analog Input 0
#define OFFSET 0.00  //deviation compensate
//#define LED 13
#define SAMPLING_INTERVAL 20
#define PRINT_INTERVAL 800
#define ARRAY_LENGTH 40  //times of collection
int PH_ARRAY[ARRAY_LENGTH]; //Store the average value of the sensor feedback
int PH_ARRAY_INDEX=0;

OneWire oneWire(DQ_pin);              //溫度的
DallasTemperature sensors(&oneWire);

#define voltage 5.00    //system voltage     ORP的
#define OFFSET 0        //zero drift voltage
#define LED 13         //operating instructions

double orpValue;

#define ArrayLenth  40    //times of collection
#define orpPin A2          //orp meter output,connect to Arduino controller ADC pin

int orpArray[ArrayLenth];
int orpArrayIndex=0;

#include <SoftwareSerial.h>
SoftwareSerial esp8266(3,2); 
 
//#include "DHT.h"        // 匯入DHT函式庫
//DHT dht(2, DHT22);      // DHT22的訊號接到pin2

String apiKey = "C7O88IHOX1OZSYJA";     // ""內輸入ThingSpeak的Write API Key
String ssid = "Stephen";       // ""內輸入wifi基地台的名稱
String password = "wanlingiscute";   // ""內輸入wifi基地台的密碼
boolean DEBUG = true;

void showResponse(int waitTime){
    long t=millis();
    char c;
    while (t+waitTime>millis()){
      if (esp8266.available()){
        c=esp8266.read();
        if (DEBUG) Serial.print(c);
      }
    }        
}

boolean thingSpeakWrite(float PH_value,float temperature_value,float orpValue){   //自己增加或減少要上傳的數據項目
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149";
  cmd += "\",80";
  esp8266.println(cmd);
  if (DEBUG) Serial.println(cmd);
  if(esp8266.find("Error")){
    if (DEBUG) Serial.println("AT+CIPSTART error");
    return false;
  }

  String getStr = "GET /update?api_key=";
  getStr += apiKey;

  getStr +="&field1=";
  getStr += String(PH_value);

  getStr +="&field2=";
  getStr += String(temperature_value);

  getStr +="&field3=";
  getStr += String(orpValue);
 
  // getStr +="&field3=";                             //自己增加或減少要上傳的數據項目
  // getStr += String(value3);
  // ...
  getStr += "\r\n\r\n";

  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  esp8266.println(cmd);
  if (DEBUG)  Serial.println(cmd);
  delay(100);
  esp8266.print(getStr);
  if (DEBUG)  Serial.print(getStr);
  else{
    esp8266.println("AT+CIPCLOSE");
    if (DEBUG)   Serial.println("AT+CIPCLOSE");
    return false;
    }
  return true;
}


void setup() {     
  // put your setup code here, to run once:
//  pinMode(LED,OUTPUT);
  Serial.begin(9600);
  sensors.begin();
  pinMode(LED,OUTPUT);
  Serial.println("PH SENSOR KIT VOLTAGE TEST!"); //Test the serial monitor
             
  
  Serial.println("Arduino...OK");
 
  esp8266.begin(115200);
  esp8266.write("AT+UART_DEF=9600,8,1,0,0\r\n");
  delay(1500);
  esp8266.begin(9600);
  Serial.println("ESP8266...OK");
  esp8266.println("AT+CWMODE=1");   // set esp8266 as client
  showResponse(1000);  
  esp8266.println("AT+CWJAP=\""+ssid+"\",\""+password+"\"");
  Serial.println("AT+CWJAP=\""+ssid+"\",\""+password+"\"");
  showResponse(10000);  //顯示並等待wifi完成連線

//  dht.begin();
}


void loop() {
   // put your main code here, to run repeatedly:
  static unsigned long SAMPLING_TIME = millis();
  static unsigned long PRINT_TIME = millis();
  static float VOLTAGE;
  static float PH_value;
  static float temperature_value;

  static unsigned long orpTimer=millis();   //analog sampling interval     Orp顯示部分
  static unsigned long printTime=millis();
  if(millis() >= orpTimer)
  {
    orpTimer=millis()+20;
    orpArray[orpArrayIndex++]=analogRead(orpPin);    //read an analog value every 20ms
    if (orpArrayIndex==ArrayLenth) {
      orpArrayIndex=0;
    }   
    orpValue=((30*(double)voltage*1000)-(75*avergearray(orpArray, ArrayLenth)*voltage*1000/1024))/75-OFFSET;   //convert the analog value to orp according the circuit
  }
  if(millis() >= printTime)   //Every 800 milliseconds, print a numerical, convert the state of the LED indicator
  {
    printTime=millis()+800;
    Serial.print("ORP: ");
    Serial.print((int)orpValue);
        Serial.println("mV");
        digitalWrite(LED,1-digitalRead(LED));
  }


  if(millis()-SAMPLING_TIME > SAMPLING_INTERVAL) //PH值部分
  {
    PH_ARRAY[PH_ARRAY_INDEX++]=analogRead(SENSOR);
    if(PH_ARRAY_INDEX==ARRAY_LENGTH)PH_ARRAY_INDEX=0;
    VOLTAGE=AVERAGE_ARRAY(PH_ARRAY,ARRAY_LENGTH)*5.0/1024;
    PH_value=7-VOLTAGE/59.2;
    SAMPLING_TIME=millis();
  }
  if(millis() - PRINT_TIME >PRINT_INTERVAL) //Every 800 milliseconds, print a numerical,convert the state of the LED indicator
  {
    
    Serial.print("PH OUTPUT:");
    Serial.println(PH_value,4);
    //digitalWrite(LED,digitalRead(LED)^1);
    PRINT_TIME=millis();
  }

   Serial.print("Temperatures->");            //溫度顯示部分
   sensors.requestTemperatures();
   temperature_value= sensors.getTempCByIndex(0);
   Serial.println(temperature_value);
   //Serial.println(sensors.getTempCByIndex(0));

  
   thingSpeakWrite(PH_value,temperature_value,orpValue);                      //自己增加或減少要上傳的數據項目
   
   
   delay(60000);    // thingspeak需要15秒以上才能更新

     
}


double AVERAGE_ARRAY(int* ARR, int NUMBER) //PH值的
{
  int i;
  int max,min;
  double AVG;
  long AMOUNT=0;
  if(NUMBER<=0)
  {
    Serial.println("ERROR!/n");
    return 0;
  }
  if(NUMBER<5)  //less than 5,calculated directly statistics
  {
    for(i=0;i<NUMBER;i++)
    {
      AMOUNT+=ARR[i];
    }
    AVG = AMOUNT/NUMBER;
    return AVG;
  }
  else
  {
    if(ARR[0]<ARR[1])
    {
       min = ARR[0];
       max = ARR[1];
    }
    else
    {
      min = ARR[1];
      max = ARR[0];
    }
    for(i=2;i<NUMBER;i++)
    {
      if(ARR[i]<min)
      {
        AMOUNT+=min;  //arr<min
        min=ARR[i];
      }
      else
      {
        if(ARR[i]>max)
        {
          AMOUNT+=max;  //arr>max
          max=AMOUNT+=ARR[i];
        }
        else
        {
          AMOUNT+=ARR[i];  //min<=arr<=max
        }
      }//if
    }//for
    AVG = (double)AMOUNT/(NUMBER-2);
  }//if
  return AVG;

}

double avergearray(int* arr, int number){   //ORP的
  int i;
  int max,min;
  double avg;
  long amount=0;
  if(number<=0){
    printf("Error number for the array to avraging!/n");
    return 0;
  }
  if(number<5){   //less than 5, calculated directly statistics
    for(i=0;i<number;i++){
      amount+=arr[i];
    }
    avg = amount/number;
    return avg;
  }else{
    if(arr[0]<arr[1]){
      min = arr[0];max=arr[1];
    }
    else{
      min=arr[1];max=arr[0];
    }
    for(i=2;i<number;i++){
      if(arr[i]<min){
        amount+=min;        //arr<min
        min=arr[i];
      }else {
        if(arr[i]>max){
          amount+=max;    //arr>max
          max=arr[i];
        }else{
          amount+=arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount/(number-2);
  }//if
  return avg;
}
