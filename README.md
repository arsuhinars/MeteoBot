# MeteoBot
DIY погодная метеостанция под управлением ESP8266.

## Основные функции
* Построение графиков показаний через веб-интерфейс
* Прогноз погоды
* Telegram бот
* Отправка показаний на [Народный мониторинг](https://narodmon.ru)

# Настройка
Все доступные настройки вы сможете найти в файле скетча.
Ниже перечисленны некоторые из них:
```
#define WIFI_SSID               "SSID"            //Имя WiFi сети
#define WIFI_PASSWORD           "PASS"            //Пароль этой сети
#define BOT_TOKEN               ""                //Токен бота в Telegram
#define USE_NARODMON            1                 //Отправлять ли данные на народный мониторинг
#define GEO_LOCATION            ""                //Название города (для бота в Телеграм)
#define GEO_HEIGHT              0                 //Высота над уровнем моря
#define TIME_ZONE               3                 //Временная зона
```

# Использованные библиотеки
* [GyverBME280](https://github.com/GyverLibs/GyverBME280)
* [Forecaster](https://github.com/GyverLibs/Forecaster)
* [TimeLib](https://github.com/PaulStoffregen/Time)
* [UniversalTelegramBot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
