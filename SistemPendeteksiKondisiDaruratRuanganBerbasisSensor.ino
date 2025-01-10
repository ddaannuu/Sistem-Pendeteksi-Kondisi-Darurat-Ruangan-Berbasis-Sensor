#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <UniversalTelegramBot.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define PIR D3
#define DHTPIN D4
#define DHTTYPE DHT11
#define MQ2PIN A0
#define buzz D5
#define rLed D6
#define bLed D7

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

int sensor = 0;
float temp, gasV, hum;

int suhuTerakhir = 0.0;
int batasGas = 300;
int suhuDarurat = 38;
bool buzzeract = false;
int entitas = 0;
t]= 
const char* ssid = "";
const char* password = "";
const char* botToken = "";
const char*  chatID= "";

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

unsigned long lastTelegramTime = 0;
const unsigned long telegramInterval = 1000;

// Timer pengganti delay()
unsigned long prevMillisLCD = 0;
unsigned long prevMillisSensor = 0;
unsigned long prevMillisBuzzer = 0;
unsigned long lcdInterval = 2500;
unsigned long sensorInterval = 500;
unsigned long buzzerInterval = 100;

bool gasAlertSent = false;
bool suhuAlertSent = false;
bool motionAlertSent = false;
bool teleSentMotion = true;
bool monitoringActive = false;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung!");
  client.setInsecure();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting....");
  delay(250);
  lcd.setCursor(0, 1);
  lcd.print("Kelompok 1");
  delay(2000);

  dht.begin();
  pinMode(PIR, INPUT);
  pinMode(MQ2PIN, INPUT);
  pinMode(buzz, OUTPUT);
  pinMode(rLed, OUTPUT);
  pinMode(bLed, OUTPUT);

  bot.sendMessage(chatID, "🤖 Bot siap! Gunakan:\n/monitoring - Mulai monitoring\n/startmotion - Aktifkan sensor gerakan\n/stopmotion - Mematikan sensor gerakan", "");
}

void sendTelegramAlert(String message) {
  if (millis() - lastTelegramTime > telegramInterval) {
    bot.sendMessage(chatID, message, "Markdown");
    lastTelegramTime = millis();
    Serial.println("Telegram Alert Sent: " + message);
  }
}

void bareng(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(buzz, HIGH);
    digitalWrite(rLed, HIGH);
    delay(100);
    digitalWrite(buzz, LOW);
    digitalWrite(rLed, LOW);
    delay(100);
  }
}

void bareng2(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(buzz, HIGH);
    digitalWrite(bLed, HIGH);
    delay(100);
    digitalWrite(buzz, LOW);
    digitalWrite(bLed, LOW);
    delay(100);
  }
}

void sensorData() {
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  gasV = analogRead(MQ2PIN);
  sensor = digitalRead(PIR);
}

void displayDataOnLCD() {
  unsigned long currentMillis = millis();
  if (currentMillis - prevMillisLCD >= lcdInterval) {
    prevMillisLCD = currentMillis;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Suhu: " + String(temp) + "C");
    lcd.setCursor(0, 1);
    lcd.print("Gas : " + String(gasV) + "%");
    delay(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lembab : " + String(hum) + "%");
    lcd.setCursor(0, 1);
    lcd.print("Entitas: " + String(entitas));
  }
}

void checking() {
  if (teleSentMotion == 1) {
    if (sensor == HIGH && !motionAlertSent) {
      sendTelegramAlert("⚠️ Gerakan terdeteksi! Gerakan : " + String(entitas + 1));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("GERAKAN!!!");
      delay(2000);
      entitas++;
      motionAlertSent = true;
      bareng(3);
    } else if (sensor == LOW) {
      motionAlertSent = false;
    }
  }

  if (gasV >= batasGas && !gasAlertSent) {
    gasAlertSent = true;
    sendTelegramAlert("⚠️ Gas berbahaya terdeteksi! Gas: " + String(gasV) + "%");
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print("GAS/ASAP");
    lcd.setCursor(2, 1);
    lcd.print("TERDETEKSI!!!");
    bareng(60);
  } else if (gasV < batasGas) {
    gasAlertSent = false;
  }

  if (temp >= suhuDarurat && !suhuAlertSent) {
    suhuAlertSent = true;
    sendTelegramAlert("⚠️ Suhu tinggi: " + String(temp) + "°C");
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("SUHU PANAS!");
    lcd.setCursor(0, 1);
    lcd.print("TERDETEKSI!");
    bareng(60);
  } else if (temp < suhuDarurat) {
    suhuAlertSent = false;
  }

  // Suhu Naik
  if (temp >= suhuTerakhir + 1) {
    sendTelegramAlert("⬆️ Suhu naik! Suhu: " + String(temp) + "°C");
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("SUHU NAIK");
    bareng2(1);
    suhuTerakhir = temp;
  }

  // Suhu Turun
  if (temp < suhuTerakhir - 1) {
    sendTelegramAlert("⬇️ Suhu turun! Suhu: " + String(temp) + "°C");
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("SUHU TURUN");
    bareng2(2);
    suhuTerakhir = temp;
  }
}

void sendTelegramMessage() {
  String message = "DATA MONITORING :\n";
  message += "Suhu               : " + String(temp) + " C\n";
  message += "Kelembaban  : " + String(hum) + " %\n";
  message += "Gas                 : " + String(gasV) + " %\n";
  message += "Gerakan         : " + String(entitas);

  if (millis() - lastTelegramTime > telegramInterval) {
    bot.sendMessage(chatID, message, "Markdown");
    lastTelegramTime = millis();
    Serial.println("\n"+message +"\n");
  }  
}

void processTelegramCommand() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = bot.messages[i].chat_id;
      String text = bot.messages[i].text;

      if (text == "/monitoring") {
        bot.sendMessage(chat_id, "🔄 Monitoring dimulai!", "");
        sendTelegramMessage();
      } else if (text == "/startmotion") {
        bot.sendMessage(chat_id, "Sensor gerakan aktif!", "");
        teleSentMotion = true;
      } else if (text == "/stopmotion") {
        bot.sendMessage(chat_id, "Sensor gerakan nonaktif!", "");
        teleSentMotion = false;
      } else {
        bot.sendMessage(chat_id, "🤖 Perintah tidak dikenali!", "");
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}

void loop() {
  sensorData();
  displayDataOnLCD();
  checking();
  processTelegramCommand();
}
