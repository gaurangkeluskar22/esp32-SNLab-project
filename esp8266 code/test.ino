#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); 

typedef struct struct_message {
    String user_input;
} struct_message;

uint8_t key[] =  {0x9C,0x9C,0x1F,0xC9,0x79,0x60};
// create a structure to hold data comming from esp32
struct_message data_comming_myMessage;
// create a structure to hold data comming from user esp8266
struct_message user_data;
//crate varible to store string comming from esp32
String incoming_data_from_esp32;
//create varible to store string comming from user esp8266
String data_from_esp8266;

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void onDataReceiver(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
   Serial.println("Message received.");
   // We don't use mac to verify the sender
   // Let us transform the incomingData into our message structure
  memcpy(&data_comming_myMessage, incomingData, sizeof(data_comming_myMessage));
  if(len>0){
    Serial.print("Bytes Received:");
    Serial.println(data_comming_myMessage.user_input);
    incoming_data_from_esp32 = data_comming_myMessage.user_input;
    Serial.println(incoming_data_from_esp32);
  }
}
void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  
  Serial.begin(115200);
  WiFi.disconnect();
  ESP.eraseConfig();
  // Wifi STA Mode
  WiFi.mode(WIFI_STA);
  // Get Mac Add
  Serial.print("Mac Address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("\nESP-Now Receiver:");
  
  // Initializing the ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Problem during ESP-NOW init");
    return;
  }
  
  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(key, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  // We can register the receiver callback function
  esp_now_register_recv_cb(onDataReceiver);
}
void loop() {
    int choice=0;
    Serial.println("Enter 1 to Send data:");
    Serial.println("Enter 2 to Decrypt:");
    lcd.setCursor(0,0);
    lcd.print("1) Send Data:");
    lcd.setCursor(0,1);
    lcd.print("2) Decrypt Data:");
    while(Serial.available()==0){}
    choice = Serial.parseInt();

    switch(choice){
      case 1:
        lcd.clear();
        getReadings();
        break;
      case 2:
        lcd.clear();
        incoming_data_from_esp32.trim();
        if(incoming_data_from_esp32.length()!=0){ 
          updateDisplay();
          incoming_data_from_esp32=' ';
        }
        else{
          lcd.setCursor(0,0);
          lcd.print("NO Data");    
          delay(5000);
        }
    }
}


void getReadings(){
   Serial.println("Enter String:");
   lcd.setCursor(0, 0);
   lcd.print("Enter Text:");
   String t;
   while (Serial.available() > 0){
       t = Serial.read(); 
   }
   while(Serial.available()==0){}
   data_from_esp8266=' ';
   data_from_esp8266 = Serial.readString();
   data_from_esp8266.trim();
   lcd.setCursor(0,1);
   lcd.print(data_from_esp8266);
   user_data.user_input = data_from_esp8266;
   data_from_esp8266=' ';
   Serial.println("data in enum:");
   Serial.println(user_data.user_input);
   delay(3000);
   // Send message via ESP-NOW
   esp_now_send(key, (uint8_t *) &user_data, sizeof(user_data));
}

void updateDisplay(){
  Serial.println("Incoming Data:");
  Serial.print(data_comming_myMessage.user_input);
  lcd.setCursor(0,0);
  lcd.print("Incoming Data:");
  lcd.setCursor(0,1);
  lcd.print(data_comming_myMessage.user_input);
  delay(10000);
}
