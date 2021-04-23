#include <Crypto.h>
#include <base64.hpp>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); 


// ========================== Methods and all things to encrypt and decrypt data ===============================
#define BLOCK_SIZE 16

uint8_t key[BLOCK_SIZE] = { 0x1C,0x3E,0x4B,0xAF,0x13,0x4A,0x89,0xC3,0xF3,0x87,0x4F,0xBC,0xD7,0xF3, 0x31, 0x31 };
uint8_t iv[BLOCK_SIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void bufferSize(char* text, int &length)
{
  int i = strlen(text);
  int buf = round(i / BLOCK_SIZE) * BLOCK_SIZE;
  length = (buf <= i) ? buf + BLOCK_SIZE :  buf;
}
    
void encrypt(char* plain_text, char* output, int length)
{
  byte enciphered[length];
  RNG::fill(iv, BLOCK_SIZE); 
  AES aesEncryptor(key, iv, AES::AES_MODE_128, AES::CIPHER_ENCRYPT);
  aesEncryptor.process((uint8_t*)plain_text, enciphered, length);
  int encrypted_size = sizeof(enciphered);
  char encoded[encrypted_size];
  encode_base64(enciphered, encrypted_size, (unsigned char*)encoded);
  strcpy(output, encoded);
}

void decrypt(char* enciphered, char* output, int length)
{
  length = length + 1; //re-adjust
  char decoded[length];
  decode_base64((unsigned char*)enciphered, (unsigned char*)decoded);
  bufferSize(enciphered, length);
  byte deciphered[length];
  AES aesDecryptor(key, iv, AES::AES_MODE_128, AES::CIPHER_DECRYPT);
  aesDecryptor.process((uint8_t*)decoded, deciphered, length);
  strcpy(output, (char*)deciphered);
}

//==================================================================== END ========================================================
 


// REPLACE WITH THE MAC Address of your receiver 
// 0x9C,0x9C,0x1F,0xCB,0x0F,0xAC second node
// 9C:9C:1F:C9:79:60 second node
// 10:52:1C:02:16:EF esp8266 node
uint8_t broadcastAddress[] = {0x10,0x52,0x1C,0x02,0x16,0xEF};

String user_input;

// Define variables to store incoming readings
//char incoming_data_need_to_decrypt[100];


// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
//typedef struct struct_message {
//    char encrypted[];
//} struct_message;

// Create a struct_message called ExternalInput to hold sensor readings
//struct_message ExternalInput;
//
//// Create a struct_message to hold incoming sensor readings
char incomingReadings[24];

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
  }
}

void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  
  // Init Serial Monitor
  Serial.begin(115200);
  while (!Serial) {
    ; //wait
  }

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
        if(sizeof(incomingReadings)!=0){ 
           updateDisplay();
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
   // ============================================== TO Encrypt Data ==================================================
   // Length (with one extra character for the null terminator)
   int str_len = user_input.length() + 1; 
   // Prepare the character array (the buffer) 
   char plain_text[str_len];
   //char plain_text[] = "ABCD";
   // Copy it over 
   user_input.toCharArray(plain_text, str_len);
   Serial.println("data in plain text char array:");
   Serial.println(plain_text);
   // encrypt
   int length = 0;
   bufferSize(plain_text, length);
   char encrypted[length];
   encrypt(plain_text, encrypted, length);
   Serial.println("");
   Serial.print("Encrypted: ");
   Serial.println(encrypted); 
   
   length = strlen(encrypted);
   char decrypted[length];
   decrypt(encrypted, decrypted, length);
   
   Serial.println(user_input);
   Serial.print(user_input);
   user_input.trim();
   lcd.setCursor(0,1);
   lcd.print(user_input);
   delay(3000);
  // ============================================== TO Display encrypted data on lcd screen =================================
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Encrypted Data:");
   lcd.setCursor(0,1);
   lcd.print(encrypted);
   delay(3000);
   //======================== ==============================To send data to other node=============================================
   //ExternalInput.encrypted = encrypted;
   //memcpy(ExternalInput.encrypted, encrypted, sizeof(encrypted));
   // Send message via ESP-NOW
   esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &decrypted, sizeof(decrypted));
   
   if (result == ESP_OK) {
    Serial.println("Sent with success");
   }
   else {
    Serial.println("Error sending the data");
  }
}

void updateDisplay(){
//  Serial.println("Incoming Data:");
//  Serial.print(incomingReadings);
//  lcd.setCursor(0,0);
//  lcd.print("Incoming Data:");
//  lcd.setCursor(0,1);
//  incomingReadings.trim();
//  lcd.println(incomingReadings);
//  delay(4000);
  // ========================================== CODE TO Decrypt data =============================
//  int length = strlen(incomingReadings);
//  char decrypted[length];
//  decrypt(incomingReadings, decrypted, length);

  Serial.println("Decrypted data: ");
  Serial.println(incomingReadings);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Decrypted Data:");
  lcd.setCursor(0,1);
  lcd.print(incomingReadings);
  delay(10000);
}
