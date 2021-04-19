
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// REPLACE WITH THE MAC Address of your receiver 
// 0x9C,0x9C,0x1F,0xCB,0x0F,0xAC second node
// 9C:9C:1F:C9:79:60 second node
// 10:52:1C:02:16:EF esp8266 node
uint8_t broadcastAddress[] = {0x10,0x52,0x1C,0x02,0x16,0xEF};

String user_input;

// Define variables to store incoming readings
String incoming_data;


// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    String user_input;
} struct_message;

// Create a struct_message called ExternalInput to hold sensor readings
struct_message ExternalInput;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  if (len>0){
    Serial.print("Bytes received: ");
    Serial.println(len);
    incoming_data = incomingReadings.user_input;
    Serial.println(incomingReadings.user_input);
  }
}
 
void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  Serial.print("Mac Address: ");
  Serial.print(WiFi.macAddress());
  Serial.println("ESP32 ESP-Now Broadcast");

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = 0;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
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
        incoming_data.trim();
        if(incoming_data.length()!=0){ 
           updateDisplay();
           incoming_data=' ';
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
      user_input = Serial.readString();
  
   Serial.println(user_input);
   Serial.print(user_input);
   user_input.trim();
   lcd.setCursor(0,1);
   lcd.print(user_input);
   delay(3000);
   ExternalInput.user_input = user_input;
   // Send message via ESP-NOW
   esp_err_t result = esp_now_send(NULL, (uint8_t *) &ExternalInput, sizeof(ExternalInput));
   
   if (result == ESP_OK) {
    Serial.println("Sent with success");
   }
   else {
    Serial.println("Error sending the data");
  }
}

void updateDisplay(){
  Serial.println("Incoming Data:");
  Serial.print(incomingReadings.user_input);
  lcd.setCursor(0,0);
  lcd.print("Incoming Data:");
  lcd.setCursor(0,1);
  lcd.print(incomingReadings.user_input);
  delay(10000);
}
