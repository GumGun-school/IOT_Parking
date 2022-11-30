#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <NewPing.h>



enum MessageType{
  startSpot,
  registerSpot,
  listSpot,
  adminListSpot,
  getTime
};

struct parkInfo{
  String chatId;
  int64_t arrivalTime;
  bool isOcupied;
};


int32_t parseMsg(enum MessageType *messageType, int32_t *parkingSpot, int32_t isAdmin, const char *message, int32_t messageLen);
int32_t checkAdmin(String chatId);
int checkSpots();

//int forFunction();


#define ADMIN_CHAT_ID "1563792608"
#define BOT_DELAY 1000
#define LIMIT_DIST 8
#define LED 14

// Initialize Telegram BOT
#define BOTtoken "5735675546:AAFZ6zfbzLxWwDwRYd2riYagqaCvYDVvDlo"  // your Bot Token (Get from Botfather)

const char* ssid = "LANLAN";
const char* password = "holahola";

NewPing spots[4] = {
  NewPing(23, 22, LIMIT_DIST),
  NewPing(12, 13, LIMIT_DIST),
  NewPing(33, 32, LIMIT_DIST),
  NewPing(26, 27, LIMIT_DIST)
};

struct parkInfo parkingsSlots[4]={{String(""), 0, 0},{String(""), 0, 0},{String(""), 0, 0},{String(""), 0, 0}};


int32_t start;
int32_t start_1 = 0;



WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Checks for new messages every 1 second.
unsigned long lastTimeBotRan;

String getAdminReadings(){
  int32_t currentTime = millis();
  String message = 
  "Place 0\n  ocupied " + String((parkingsSlots[0].isOcupied)?"Yes":"No") + "\n  Arrived: " + String(currentTime - parkingsSlots[0].arrivalTime)+" seconds\n"+ 
  "Place 1\n  ocupied " + String((parkingsSlots[1].isOcupied)?"Yes":"No") + "\n  Arrived: " + String(currentTime - parkingsSlots[1].arrivalTime)+" seconds\n"+
  "Place 2\n  ocupied " + String((parkingsSlots[2].isOcupied)?"Yes":"No") + "\n  Arrived: " + String(currentTime - parkingsSlots[2].arrivalTime)+" seconds\n"+
  "Place 3\n  ocupied " + String((parkingsSlots[3].isOcupied)?"Yes":"No") + "\n  Arrived: " + String(currentTime - parkingsSlots[3].arrivalTime)+" seconds\n";

  return message;
}


String getReadings(){
   
  int32_t cont = 0;
  String tmp;
  int32_t ping[4];

  
  String message = String("there are:");
  
  for(int iter=0; iter<4; iter++){
    ping[iter] =  spots[iter].ping_cm();
    
    if(ping[iter] == 0){
      Serial.println("Se vacio: " + String(iter));
      cont++;
      message = String(message + "\n   Slot " + String(iter));
    }
    
  }
  
  
  if(ping[0] < 30){
    start_1 =  millis() - start;
    cont++;
  }else if (ping[0] > 30){
    start_1 = 0;
  }  


  return message;
}

int32_t checkAdmin(String chatId){
  if(chatId == ADMIN_CHAT_ID){
    return 1;
  }
  return 0;
}

int32_t checkSpots(){
  int32_t current = millis();
  for(int iter=0; iter<4; iter++){
    int32_t distance = spots[iter].ping_cm();
    if(parkingsSlots[iter].isOcupied){ 
      if(distance==0){
        Serial.println("Se vacio: " + String(iter));
        Serial.println("Tiempo: " + String(parkingsSlots[iter].arrivalTime));
        Serial.println("Lugar: " + parkingsSlots[iter].chatId);
        if(parkingsSlots[iter].chatId != ""){
          bot.sendMessage(parkingsSlots[iter].chatId, "Your car left the parking spot " + String(iter) + " and lasted " + String((millis() - parkingsSlots[iter].arrivalTime)/1000) + " seconds\n"+ String(((millis() - parkingsSlots[iter].arrivalTime)/1000)*.5) + " to pay\nCome back soon", "");
          Serial.println("Se mando un mensaje");
          parkingsSlots[iter].chatId="";
        }
        
        parkingsSlots[iter].isOcupied=0;
      }

    }else{  
      if(distance>0 && distance<50){
        parkingsSlots[iter].isOcupied = 1;
        parkingsSlots[iter].arrivalTime = millis();
        Serial.println("Se Ocupo " + String(iter));
        
      }
      
    }
    
  }
  
  return 1;
}


//Handle what happens when you receive new messages
int32_t handleNewMessages(int numNewMessages) {

  Serial.println("handleNewMessages");

  for (int32_t i=0; i<numNewMessages; i++) {
    Serial.println(bot.messages[i].text);
    String chatId = String(bot.messages[i].chat_id);
    
    // Print the received message
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;

    enum MessageType messageType;
    int32_t parkingSpot = 0;
    Serial.println("parsing MSG");
    int32_t isAdmin = checkAdmin(bot.messages[i].chat_id);
    
    int32_t error = parseMsg(&messageType, &parkingSpot, isAdmin, text.c_str(), text.length());
    if(error){
      Serial.println("Space not available");
      return 1;
    }
    if(parkingSpot>4){
      bot.sendMessage(chatId, "Incorrect parking spot", "");
      Serial.println("Incorrect Place");
      return 1;
    }
    switch(messageType){
      case startSpot:{
        bot.sendMessage(chatId, "You can send:\n  /spots for a list of the available spots\n  /register <number you parked in> once you are parked to get notification from telegram", "");
        
        break;
      }
      case registerSpot:{
        Serial.println("Reg a spot: " + String(parkingSpot));
        Serial.println(parkingsSlots[parkingSpot].chatId + chatId);
        if(parkingsSlots[parkingSpot].isOcupied){
          if(parkingsSlots[parkingSpot].chatId == ""){
            parkingsSlots[parkingSpot].chatId = chatId;
            Serial.println("Parking assigned to " + parkingsSlots[parkingSpot].chatId);
          }else{
            Serial.println("Parking already taken");
          }
          bot.sendMessage(chatId, "The parking spot number " + String(parkingSpot) + " was assigned to you.", "");
        }else{
          bot.sendMessage(chatId, "There is no one parked", "");
        }
        break;
      }
      
      case listSpot:{
        String readings = getReadings();
        bot.sendMessage(chatId, readings, "");
        break;
      }
      case adminListSpot:{
        String readings = getAdminReadings();
        bot.sendMessage(chatId, readings, "");
        break;
      }
      case getTime:{
        
        if(parkingsSlots[parkingSpot].chatId == chatId){
          
          bot.sendMessage(chatId, "your car has been parked " + String((millis() - parkingsSlots[parkingSpot].arrivalTime)/1000) + " seconds \n" + String(((millis() - parkingsSlots[parkingSpot].arrivalTime)/1000)*.5) + " to pay", "");
          
        }else{
          
          bot.sendMessage(chatId, "thats not your car", "");
        }
        
        break;
      }
      
    }   
    
  }
  return 0;
  
}


int32_t 
parseMsg(enum MessageType *messageType, int32_t *parkingSpot, int32_t isAdmin, const char *message, int32_t messageLen){
  if(isAdmin){
    Serial.println("el Admin");
    if(!memcmp(message, "/list", 5)){
      *messageType = adminListSpot;
      
    }
    
  }
  if(!memcmp(message, "/start", 5)){
    
    *messageType = startSpot;
    
  }else if(!memcmp(message, "/register", 9)){
    
    if(messageLen < 10){
      return 1;
    }
    char *parsed = (char*)malloc(messageLen-5);
    snprintf(parsed, messageLen-5, "%s", message+10);
    *messageType=registerSpot;
    *parkingSpot=atoi(parsed);
    
  }else if(!memcmp(message, "/spot", 5)){
  
    *messageType=listSpot;
        
  }else if(!memcmp(message, "/time", 5)){
    
    
    if(messageLen < 7){
      return 1;
    }
    
    char *parsed = (char*)malloc(messageLen-5);
    snprintf(parsed, messageLen, "%s", message + 6);
    
    *messageType=getTime;
    *parkingSpot=atoi(parsed);
    
    Serial.println(String(*parkingSpot));
    Serial.println("Place " + String (parsed));
  }
  
  return 0;

}


void setup() {
  // Connect to Wi-Fi
  // Serial.begin(115200);
  delay(1000);
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");
  Serial.println(" Connected!");

  WiFi.mode(WIFI_STA); //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(100);
  } 

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
#endif

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  
  
  /*
  checkSpots();
  sleep(3);
  checkSpots();
  */
}

void loop() {
 
  if (millis() > lastTimeBotRan + BOT_DELAY)  {
    checkSpots();
    
    int32_t numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    
    while(numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      
    }
    
    lastTimeBotRan = millis();
    
  }
}
