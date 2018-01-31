#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <TelegramBot.h>
#include <ESP8266Ping.h>

char wifiSsid[] = "<youWifiId>";
char wifiPassword[] = "<youWifiPassword>";

#define BOT_TOKEN "<telegramBotToken>"
#define REGISTRATED_USERNAME "<userName>"
#define REGISTRATED_CHAT_ID "<chatId>"
#define BOT_MTBS 500
#define PING_MTBS 30000
#define LIMIT_ERROR_PING 6
#define SMART_REBOOT_DELAY 10000
#define REMOTE_IP_ADDRESS 123

#define RELAY_1_PIN D0
#define RELAY_2_PIN D1

#define SHOW_ALL_CMD "/cmd"
#define ALL_RELAY_ON_CMD "/onAll"
#define ALL_RELAY_ON_MSG "Relays ON"
#define ALL_RELAY_OFF_CMD "/offAll"
#define ALL_RELAY_OFF_MSG "Relays OFF"
#define STATUS_CMD "/status"
#define PING_STATUS_ONLINE_MSG "Computer is online =)"
#define PING_STATUS_OFFLINE_MSG "Computer is offline =("
#define PING_STATUS_OFFLINE_SMART_WORK_MSG "Computer is offline!!!"
#define FIRST_PING_SUCCESS_MSG "Computer is running!"
#define START_SMART_WORK_CMD "/start"
#define START_SMART_WORK_MSG "WatchDog function started!"
#define STOP_SMART_WORK_CMD "/stop"
#define STOP_SMART_WORK_MSG "WatchDog function stopped!"
#define CMD_NOT_FOUND "Command not found!"
#define AUTO_RESET_MSG "Auto reset!"
#define UNKNOW_USER_MSG "Unknow user!"

#define ALL_CMD_MSG "On all relays: " ALL_RELAY_ON_CMD "\r\nOff all relays: " ALL_RELAY_OFF_CMD "\r\nCurrent status: " STATUS_CMD "\r\nStart WatchDog function: " START_SMART_WORK_CMD "\r\nStop WatchDog function: " STOP_SMART_WORK_CMD "\r\n"

WiFiClientSecure client;
TelegramBot bot (BOT_TOKEN, client);
const IPAddress remoteIp(192, 168, 0, REMOTE_IP_ADDRESS);

long botCheckLastTime;
long pingLastTime;
String currentChatId = REGISTRATED_CHAT_ID;
bool relaysStatus = true;
bool startAfterPowerOn = false;
bool smartWork = true;
int countFailPing = 0;

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Connecting Wifi: ");
  Serial.println(wifiSsid);
  WiFi.begin(wifiSsid, wifiPassword);

  while (wiFiConnected()) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  initRelaysPins();
  delay(10);
  onAllRelays();

  bot.begin();
}

void loop() {
  if (startAfterPowerOn) {
    // Checking and processing messages
    if (millis() > botCheckLastTime + BOT_MTBS) {
      message msg = bot.getUpdates();
      if (msg.chat_id != 0) {
        handleNewMessages(msg);
      }
      botCheckLastTime = millis();
    }
  
    // Ping
    if (smartWork && (millis() > pingLastTime + PING_MTBS)){
      if(!Ping.ping(remoteIp)) {
        countFailPing++;
        bot.sendMessage(currentChatId, PING_STATUS_OFFLINE_SMART_WORK_MSG);
      } else {
        countFailPing = 0;
      }
      pingLastTime = millis();
    }
  
    // Smart relays reboot after fail ping
    if (smartWork && (countFailPing >= LIMIT_ERROR_PING)) {
      bot.sendMessage(currentChatId, AUTO_RESET_MSG);
      offAllRelays();
      delay(SMART_REBOOT_DELAY);
      onAllRelays();
      countFailPing = 0;
    }
  } else {
    // First ping
    if (smartWork && (millis() > pingLastTime + PING_MTBS)){
      if(Ping.ping(remoteIp)) {
        bot.sendMessage(currentChatId, FIRST_PING_SUCCESS_MSG);
        startAfterPowerOn = true;
      }
      pingLastTime = millis();
    }
  }
}

void handleNewMessages(message msg) {
  String chatId = String(msg.chat_id);
  String text = msg.text;
  String senderName = msg.sender;
  if (senderName == REGISTRATED_USERNAME) {
    currentChatId = chatId;
    if (text == ALL_RELAY_ON_CMD) {
      onAllRelays();
      bot.sendMessage(chatId, ALL_RELAY_ON_MSG);
    } else
    if (text == ALL_RELAY_OFF_CMD) {
      offAllRelays();
      bot.sendMessage(chatId, ALL_RELAY_OFF_MSG);
    } else
    if (text == STATUS_CMD) {
      bot.sendMessage(chatId, relaysStatus ? ALL_RELAY_ON_MSG : ALL_RELAY_OFF_MSG);
      bot.sendMessage(chatId, Ping.ping(remoteIp) ? PING_STATUS_ONLINE_MSG : PING_STATUS_OFFLINE_MSG);      
    } else
    if (text == START_SMART_WORK_CMD) {
      smartWork = true;
      bot.sendMessage(chatId, START_SMART_WORK_MSG);
    } else
    if (text == STOP_SMART_WORK_CMD) {
      smartWork = false;
      bot.sendMessage(chatId, STOP_SMART_WORK_MSG);
    } else
    if (text == SHOW_ALL_CMD) {
      bot.sendMessage(chatId, ALL_CMD_MSG);
    } 
  else bot.sendMessage(chatId, CMD_NOT_FOUND);
  } else {
    bot.sendMessage(chatId, UNKNOW_USER_MSG);
  }
}

void initRelaysPins() {
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
}

void offAllRelays() {
  relaysStatus = false;
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, HIGH);
}

void onAllRelays() {
  relaysStatus = true;
  digitalWrite(RELAY_1_PIN, HIGH);
  digitalWrite(RELAY_2_PIN, LOW);
}
 
bool wiFiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

