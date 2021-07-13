
/* Настройки */
#define WIFI_SSID               "SSID"            //Имя WiFi сети
#define WIFI_PASSWORD           "PASS"            //Пароль этой сети
#define NTP_ADDRESS             "pool.ntp.org"    //Адрес NTP сервера

#define OTA_HOSTNAME            "MeteoBot"        //Название устройства в сети
#define OTA_PASSWORD            ""                //Пароль к устройству для обновления по воздуху

#define DATA_FILE_NAME          "data.bin"        //Файл с сохраненными показаниями

#define BOT_TOKEN               ""                //Токен бота в Telegram

#define USE_NARODMON            1                 //Отправлять ли данные на народный мониторинг
#define NARODMON_ADDRESS        "narodmon.ru"     //Адрес сервера народного мониторинга
#define NARODMON_PORT           8283              //Порт для отправки

#define GEO_LOCATION            ""                //Название города (для бота в Телеграм)
#define GEO_HEIGHT              0                 //Высота над уровнем моря
#define TIME_ZONE               3                 //Временная зона

#define SYNC_PERIOD             600000            //Период синхронизации времени в мс.
#define UPDATE_PERIOD           1000              //Период обновления показаний датчика
#define SAVE_PERIOD             600000            //Период сохранения показаний в память
#define BOT_UPDATE_PERIOD       1000              //Период обновления Телеграм бота
#define MAX_WAIT_RESPONSE_TIME  5000              //Максимальное время ожидания ответа от NTP сервера
/* Настройки */

//Размер файла для сохранения показаний
#define DATA_FILE_SIZE  (sizeof(unsigned long) * 5 + sizeof(WeatherData) * 73)

#include "FS.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

#include <GyverBME280.h>
#include <Forecaster.h>
#include <TimeLib.h>
#include <UniversalTelegramBot.h>

//Объект HTTP веб-сервера
ESP8266WebServer server;

//Создаем Telegram бота
WiFiClientSecure securedClient;
UniversalTelegramBot bot(BOT_TOKEN, securedClient);

//Объект UDP соединения
WiFiUDP udp;

//Объект датчика
GyverBME280 sensor;

//Объект для прогноза погоды
Forecaster cond;

//Погода в виде строк
const char* weatherStrings[] = {
  "Отличная, ясно",
  "Хорошая, ясно",
  "Хорошая, но ухудшается",
  "Достаточно хорошая, но ожидается ливень",
  "Ливни, становится менее устойчивой",
  "Пасмурная, ожидаются дожди",
  "Временами дожди, ухудшение",
  "Временами дожди, очень плохая, пасмурно",
  "Дожди, очень плохая, пасмурно",
  "Штормовая, дожди"
};

//Создаем прототип функции
unsigned long getTime();

//Структура данных о погоде
struct WeatherData {
  float temp = 0.0f;              //Текущая температура
  float pressure = 0.0f;          //Текущее давление воздуха
  float humidity = 0.0f;          //Текущая влажность
  float forecast = 0.0f;          //Предсказание погоды
  unsigned long measureTime = 0;  //Время измерения

  /* Функция чтения показаний с датчика */
  void readFromSensor() {
    temp = sensor.readTemperature();
    pressure = sensor.readPressure();
    humidity = sensor.readHumidity();
    measureTime = getTime();
  }

  /* Функция представления предсказания погоды в виде строки */
  String toString() {
    return String(weatherStrings[constrain((int)round(forecast), 0, 9)]);
  }
};

WeatherData currentWeather;
WeatherData hourWeather[6];
WeatherData dayWeather[24];
WeatherData monthWeather[31];
WeatherData yearWeather[12];

unsigned long lastForecast = 0;
unsigned long lastHourMeasure = 0;
unsigned long lastDayMeasure = 0;
unsigned long lastMonthMeasure = 0;
unsigned long lastYearMeasure = 0;

unsigned long _syncTime = 0;      //UNIX время последней синхронизации
unsigned long _lastSyncTime = 0;  //Время последней синхронизации

/* Функция синхронизации времени по NTP */
void syncTime() {
  static const uint16_t NTP_PACKET_SIZE = 48;   //Размер NTP пакета
  IPAddress ntpServerIp;                        //IP адрес NTP сервера

  //Получаем IP адрес сервера
  WiFi.hostByName(NTP_ADDRESS, ntpServerIp);

  //Буфер, хранящий текущий пакет
  byte packet[NTP_PACKET_SIZE];

  //Формируем запрос
  packet[0] = 0b00100011; //Индикатор коррекции, номер версии и режим

  //Отправляем пакет
  udp.beginPacket(ntpServerIp, 123);
  udp.write(packet, NTP_PACKET_SIZE);
  udp.endPacket();

  //Ждем ответа
  unsigned long waitBeginTime = millis(); //Время начала ожидания
  while (millis() - waitBeginTime < MAX_WAIT_RESPONSE_TIME) {
    int size = udp.parsePacket(); //Получаем размер полученного пакета
    if (size >= NTP_PACKET_SIZE) { //Если был получен пакет с допустимым размером
      udp.read(packet, NTP_PACKET_SIZE);  //Получаем пакет
      _syncTime = (unsigned long)packet[40] << 24;
      _syncTime |= (unsigned long)packet[41] << 16;
      _syncTime |= (unsigned long)packet[42] << 8;
      _syncTime |= (unsigned long)packet[43];
      _syncTime -= 2208988800UL;
      _lastSyncTime = millis();
      udp.flush();

      Serial.println(F("NTP response was received!"));
      Serial.print(F("Current time: "));
      Serial.println(_syncTime);
      return;
    }
  }

  Serial.println(F("Error! There is no NTP response"));
}

/* Функция для нахождения текущего времени */
unsigned long getTime() {
  if (millis() - _lastSyncTime > SYNC_PERIOD)
    syncTime();
  return _syncTime + (millis() - _lastSyncTime) / 1000;
}

/* Получить время в виде строки */
String getTimeAsString() {
  unsigned long time = getTime() + TIME_ZONE * SECS_PER_HOUR;
  
  char* buff = new char[17];
  sprintf_P(buff, (const char*)F("%02i.%02i.%04i %02i:%02i"), day(time), month(time), year(time), hour(time), minute(time));
  
  String str = String(buff);
  delete[] buff;
  
  return str;
}

/* Функция отправки текущих показаний на народный мониторинг */
void sendToNarodmon() {
  //IP адрес сервера
  IPAddress serverIp;

  //Получаем IP сервера
  WiFi.hostByName(NARODMON_ADDRESS, serverIp);
  
  WiFiClient client;

  //Устанавливаем соединение
  if (!client.connect(serverIp, NARODMON_PORT)) {
    Serial.print(F("Error! Connection to "));
    Serial.print(NARODMON_ADDRESS);
    Serial.println(" failed");
    return;
  }

  Serial.print("Sending data to ");
  Serial.println(NARODMON_ADDRESS);

  client.print("#");
  client.println(WiFi.macAddress());
  client.print("#TEMPC#");
  client.println(currentWeather.temp, 2);
  client.print("#PRESS#");
  client.println(currentWeather.pressure, 2);
  client.print("#HUMID#");
  client.println(currentWeather.humidity, 2);
  client.print("##");

  client.stop();
}

/* Отправить сообщение с текущим состоянием */
void sendStateMessage(String chatId) {
  bot.sendMessage(chatId,
    String(F("Погода в ")) + GEO_LOCATION +
    String(F("\n\n🌡 Температура: ")) + String(currentWeather.temp, 1) + 
    String(F("°C\n💧 Влажность: ")) + String(currentWeather.humidity, 1) + 
    String(F("%\n⏲ Давление: ")) + String(currentWeather.pressure * 0.01f, 1) +
    String(F(" гПа\n⛅ Погода: ")) + currentWeather.toString() +
    String(F("\n🗓 Дата и время: ")) + getTimeAsString() +
    String(F("\n\nЛокальный IP: ")) + WiFi.localIP().toString()
  );
}

/* Функция обработчик входящих сообщений */
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chatId = bot.messages[i].chat_id;
    String message = bot.messages[i].text;
    if (message == "/start" || message == "/get_weather") {
      sendStateMessage(chatId);
    } else {
      bot.sendMessage(chatId, F("Неизвестная команда!"));
    }
  }
}

/* Найти средне-арифметическое значение погоды из массива */
WeatherData averageWeather(WeatherData* a, int length) {
  WeatherData out;
  for (int i = 0; i < length; i++) {
    out.temp += a[i].temp;
    out.pressure += a[i].pressure;
    out.humidity += a[i].humidity;
    out.forecast += a[i].forecast;
  }
  out.temp /= length;
  out.pressure /= length;
  out.humidity /= length;
  out.forecast /= length;
  return out;
}

/* Метод для получения информации о боте */
void handleGetInfo() {
  server.send(200, "application/json", String(F("{\"botName\":\"")) + bot.userName + String(F("\",\"geoLocation\":\"")) + GEO_LOCATION + String(F("\"}")));
}

/* Метод для получения текущей информации о погоде */
void handleGetCurrentWeather() {
  server.send(200, "application/octet-stream", reinterpret_cast<uint8_t*>(&currentWeather), sizeof(currentWeather));
}

/* Метод для получения информации о погоде за текущий час */
void handleGetHourWeather() {
  server.send(200, "application/octet-stream", reinterpret_cast<uint8_t*>(&hourWeather), sizeof(hourWeather));
}

/* Метод для получения информации о погоде за текущий день */
void handleGetDayWeather() {
  server.send(200, "application/octet-stream", reinterpret_cast<uint8_t*>(&dayWeather), sizeof(dayWeather));
}

/* Метод для получения информации о погоде за текущий месяц */
void handleGetMonthWeather() {
  server.send(200, "application/octet-stream", reinterpret_cast<uint8_t*>(&monthWeather), sizeof(monthWeather));
}

/* Метод для получения информации о погоде за текущий год */
void handleGetYearWeather() {
  server.send(200, "application/octet-stream", reinterpret_cast<uint8_t*>(&yearWeather), sizeof(yearWeather));
}

/* Функция загрузки показаний */
void loadWeatherData() {
  //Если файл не существует
  if (!SPIFFS.exists(DATA_FILE_NAME))
    return;

  //Открываем файл
  File file = SPIFFS.open(DATA_FILE_NAME, "r");
  if (!file)  //Если произошла ошибка
    return;

  //Читаем все данные
  char buffer[DATA_FILE_SIZE];
  file.readBytes(buffer, DATA_FILE_SIZE);
  file.close();

  //Считываем все переменные
  lastForecast = reinterpret_cast<unsigned long>(buffer);
  lastHourMeasure = reinterpret_cast<unsigned long>(buffer + sizeof(unsigned long));
  lastDayMeasure = reinterpret_cast<unsigned long>(buffer + sizeof(unsigned long) * 2);
  lastMonthMeasure = reinterpret_cast<unsigned long>(buffer + sizeof(unsigned long) * 3);
  lastYearMeasure = reinterpret_cast<unsigned long>(buffer + sizeof(unsigned long) * 4);

  //Находим указатель на начало первого массива
  void* currPtr = buffer + sizeof(unsigned long) * 5;

  //Копируем все данные из буфера в массив
  memcpy(hourWeather, currPtr, sizeof(WeatherData) * 6);
  currPtr += sizeof(WeatherData) * 6;

  memcpy(dayWeather, currPtr, sizeof(WeatherData) * 24);
  currPtr += sizeof(WeatherData) * 24;

  memcpy(monthWeather, currPtr, sizeof(WeatherData) * 31);
  currPtr += sizeof(WeatherData) * 31;

  memcpy(yearWeather, currPtr, sizeof(WeatherData) * 12);
}

/* Функция сохрания показаний */
void saveWeatherData() {
  //Открываем файл для записи
  File file = SPIFFS.open(DATA_FILE_NAME, "w");
  if (!file) {
    Serial.println(F("Error! Failed to open file to writing"));
    return;
  }

  //Записываем все данные
  file.write(reinterpret_cast<byte*>(&lastForecast), sizeof(unsigned long));
  file.write(reinterpret_cast<byte*>(&lastHourMeasure), sizeof(unsigned long));
  file.write(reinterpret_cast<byte*>(&lastDayMeasure), sizeof(unsigned long));
  file.write(reinterpret_cast<byte*>(&lastMonthMeasure), sizeof(unsigned long));
  file.write(reinterpret_cast<byte*>(&lastYearMeasure), sizeof(unsigned long));
  file.write(reinterpret_cast<byte*>(hourWeather), sizeof(hourWeather));
  file.write(reinterpret_cast<byte*>(dayWeather), sizeof(dayWeather));
  file.write(reinterpret_cast<byte*>(monthWeather), sizeof(monthWeather));
  file.write(reinterpret_cast<byte*>(yearWeather), sizeof(yearWeather));

  //Закрываем и сохраняем файл
  file.close();
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  //Инитилизируем датчик
  if (!sensor.begin()) {
    Serial.println(F("Can not access to the BME280 sensor. Please check wirings"));
    abort();
  }

  //Устанавливаем высоту над уровнем моря
  cond.setH(GEO_HEIGHT);

  //Инитилизируем файловую систему
  SPIFFS.begin();
  
  //Загружаем показания
  loadWeatherData();

  //Подключаемся к WiFi
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  //Ожидаем подключения
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  //Запускаем Web-сервер
  server.on("/getInfo", handleGetInfo);
  server.on("/getCurrentWeather", handleGetCurrentWeather);
  server.on("/getHourWeather", handleGetHourWeather);
  server.on("/getDayWeather", handleGetDayWeather);
  server.on("/getMonthWeather", handleGetMonthWeather);
  server.on("/getYearWeather", handleGetYearWeather);
  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/script.js", SPIFFS, "/script.js");
  server.begin();

  //Синхронизируем время
  udp.begin(123);
  syncTime();

  //Получаем данные о боте
  securedClient.setInsecure();
  if (bot.getMe()) {
    Serial.print("Telegram bot started @");
    Serial.println(bot.userName);
  } else {
    Serial.println("Error was caused while starting Telegram bot");
  }

  //Указываем боту его команды
  bot.setMyCommands(F("[{\"command\":\"get_weather\", \"description\":\"Получить текущую погоду\"}]"));

  //Сразу же считываем погоду
  currentWeather.readFromSensor();
  
  //Инитилизируем обновление по воздуху
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
    
    SPIFFS.end();
    server.close();
    udp.stop();
    
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    ESP.restart();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  //Обновляем бота
  static unsigned long botTimer = 0;
  if (millis() - botTimer > BOT_UPDATE_PERIOD) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    botTimer = millis();
  }

  static unsigned long saveTime = 0;
  if (millis() - saveTime > SAVE_PERIOD) {
    //Сохраняем показания
    saveWeatherData();
    saveTime = millis();
  }

  unsigned long currentTime = getTime();
  
  //Обновляем показания датчика
  static unsigned long updateTimer = 0;
  if (millis() - updateTimer > UPDATE_PERIOD) {
    updateTimer = millis();
    currentWeather.readFromSensor();
    
    //Обновляем предсказания раз в 30 минут
    if (currentTime - lastForecast > 1800) {
      cond.setMonth(month(currentTime));
      cond.addP(currentWeather.pressure, currentWeather.temp);
      currentWeather.forecast = cond.getCast();
      lastForecast = currentTime;
    }
  }
  
  //Обновляем показания за час
  if (currentTime - lastHourMeasure > 600) {
    //Смещаем показания
    for (byte i = 5; i >= 1; i--)
      hourWeather[i] = hourWeather[i - 1];

    //Записываем текущие показания
    hourWeather[0] = currentWeather;

    lastHourMeasure = currentTime;

    //Отправляем показания на народный мониторинг
    sendToNarodmon();
  }

  //..за сутки
  if (hour(lastDayMeasure) != hour(currentTime)) {
    //Смещаем показания
    for (byte i = 23; i >= 1; i--)
      dayWeather[i] = dayWeather[i - 1];

    //Записываем текущие показания
    dayWeather[0] = averageWeather(hourWeather, 6);
    dayWeather[0].measureTime = currentTime;

    lastDayMeasure = currentTime;
  }

  //..за месяц
  if (day(lastMonthMeasure) != day(currentTime)) {
    //Смещаем показания
    for (byte i = 30; i >= 1; i--)
      monthWeather[i] = monthWeather[i - 1];

    //Записываем текущие показания
    monthWeather[0] = averageWeather(dayWeather, 24);
    monthWeather[0].measureTime = currentTime;

    lastMonthMeasure = currentTime;
  }

  //..за год
  if (month(lastYearMeasure) != month(currentTime)) {
    //Смещаем показания
    for (byte i = 11; i >= 1; i--)
      yearWeather[i] = yearWeather[i - 1];

    //Записываем текущие показания
    yearWeather[0] = averageWeather(monthWeather, 31);
    yearWeather[0].measureTime = currentTime;

    lastYearMeasure = currentTime;
  }
}
