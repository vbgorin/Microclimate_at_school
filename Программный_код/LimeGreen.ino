#include <Wire.h>                           // Подключаем библиотеку для работы по I2C
#include <SparkFun_Qwiic_Humidity_AHT20.h>  // ПОдключаем библиотеку от SparkFun
#include <SparkFun_ENS160.h>                // ПОдключаем библиотеку от SparkFun
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <iarduino_OLED_txt.h>  // Подключаем библиотеку iarduino_OLED_txt.
#include <SDS011.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>

// Настройки WiFi
const char * ssid = "ChongQiu"; // Название WiFi сети
const char * password = "Tori-man2021"; // Пароль WiFi сети
//const char * ssid = "Polygon-502"; // Название WiFi сети
//const char * password = "Admin344"; // Пароль WiFi сети

//Облако VizIoT
const String VizIoT_Device_key = "TF4SXW26JMOFN3OE"; //Ключ VizIoT устройства на лоджии
const String VizIoT_Device_pass = "SWT5NYUWVDB72IU3LEYU"; //Пароль VizIoT устройства на лоджии
const String serverURL = "http://VizIoT.com/update";
WiFiClient wifiClient;
HTTPClient http;
// Создаие объектов для работы с датчиками
SparkFun_ENS160 myENS;
AHT20 humiditySensor;
SDS011 my_sds;
iarduino_OLED_txt myOLED(0x3C);
// Объявление переменные и константы
int ensStatus, AQI, TVOC, CO2, getFlags, error;
float temperature, humidity, hi, p10, p25;
int light = 255; // яркость светодиода
float tk = 5.2; //калибровочный коэффицент датчика температуры
int hk = 10; //калибровочный коэффициент датчика влажности
#define PIN D5 // Светодиод подключен к контакту D5
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    pixels.begin(); // Инициализируем библиотеку NeoPixel
    pixels.show(); // Светодиод выкл
    myOLED.begin( & Wire); // Инициируем работу с дисплеем
    myOLED.clrScr();
    myOLED.setFont(MediumFontRus);
    myOLED.print("Школа 1557", OLED_C, 3);
    delay(3000);
    myOLED.clrScr();
    myOLED.print("ЗАПУСК", OLED_C, 1);
    myOLED.setFont(SmallFontRus);
    // Подключение к WiFi
    WiFi.begin(ssid, password);
    myOLED.print("Подключение к WiFi", OLED_C, 2);
    while (WiFi.status() != WL_CONNECTED) delay(2000);
    long rssi = WiFi.RSSI();
    String status = "Сигнал WiFi: " + String(rssi) + " dB";
    myOLED.print(status, OLED_C, 3);
    delay(1000);
    Wire.begin(); // Инициализация связи I2C
    //инициализация метеодатчика ATH21
    if (!humiditySensor.begin()) {
        myOLED.print("Метеодатчик ОШИБКА!", OLED_C, 4);
        while (1);
    }
    myOLED.print("Метеодатчик готов", OLED_C, 4);
    delay(1000);
    //инициализация датчика газа ENS160
    if (!myENS.begin() || !myENS.setOperatingMode(SFE_ENS160_RESET)) {
        myOLED.print("Датчик газа ОШИБКА!", OLED_C, 5);
        while (1);
    }
    delay(100);
    myENS.setOperatingMode(SFE_ENS160_STANDARD); // Установка стандартного режима
    ensStatus = myENS.getFlags(); //получение статуса датчика газа
    while (ensStatus > 0) {
        myOLED.print("Датчик газа нагрев", OLED_C, 5);
        delay(1000);
    }
    myOLED.print("Датчик газа готов ", OLED_C, 5);
    delay(1000);
    //ициализация датчика пыли SDS011
    my_sds.begin(0, 3); //RX, TX
    myOLED.print("Датчик пыли готов", OLED_C, 6);
}
void loop() {
    myOLED.clrScr();
    myOLED.setFont(MediumFontRus);
    if (!myENS.checkDataStatus()) {
        myOLED.print("AQI:ОШИБКА!", OLED_C, 1);
        myOLED.print("СБРОС", OLED_C, 4);
        delay(2000);
        ESP.restart(); //перезапуск контроллера
    }
    int AQI = myENS.getAQI(); //индекс качества воздуха
    int TVOC = myENS.getTVOC(); //количество летучих органических веществ 
    int CO2 = myENS.getECO2(); //количество углекислого газа
    int getFlags = myENS.getFlags(); //статус датчика газа
    if (AQI < 3) { //ФОРМИРОВАНИЕ СТРОКИ С ДАННЫМИ КАЧЕСТВА ВОЗДУХА
        String str = "AQI:" + String(AQI) + " OK";
        myOLED.print(str, OLED_C, 1);
    } else {
        String str = "AQI:" + String(AQI) + " BAD";
        myOLED.print(str, OLED_C, 1);
    }
    myOLED.setFont(SmallFontRus);
    myOLED.print("CO2:", 0, 5);
    myOLED.print(CO2, 25, 5);
    myOLED.print("| TVOC:", 50, 5);
    myOLED.print(TVOC, 95, 5);
    long rssi = WiFi.RSSI(); // считываем уровень сигнала WiFi, он должен быть не меньше 80 dB
    // Считываем показания температуры и влажности
    while (!humiditySensor.available()) delay(100);
    float temperature = humiditySensor.getTemperature() - tk;
    float humidity = humiditySensor.getHumidity() - hk;
    float hi = calcHI(temperature, humidity); // вычисление теплового индекса
    myOLED.print("Температура:", 0, 2);
    myOLED.print(temperature, 80, 2);
    myOLED.print("С", 115, 2);
    myOLED.print("Влажность:", 0, 3);
    myOLED.print(humidity, 80, 3);
    myOLED.print("%", 115, 3);
    myOLED.print("Heat Index:", 0, 4);
    myOLED.print(hi, 80, 4);
    myOLED.print("C", 115, 4);
    // Считываем показания датчика пыли
    myOLED.print("ПЫЛЬ, мг/м3", OLED_C, 6);
    error = my_sds.read( & p25, & p10);
    if (error) myOLED.print("ОШИБКА!", OLED_C, 7);
    myOLED.print("2.5:", 0, 7);
    myOLED.print(p25, 25, 7);
    myOLED.print("| 10:", 50, 7);
    myOLED.print(p10, 82, 7);
    /*-------------ЦВЕТОВАЯ ИНДИКАЦИЯ КАЧЕСТВА ВОЗДУХА-------------
1	Действий не требуется - ЗЕЛЕНЫЙ
2	Рекомендуется проветрить помещение - ГОЛУБОЙ
3	Рекомендуется усиленная вентиляция - ЖЕЛТЫЙ
4	Необходима усиленная вентиляция - РОЗОВЫЙ
5	В таком помещении находится можно только при необходимости - КРАСНЫЙ
*/
    if (p25 < 160 || p10 < 300) { //проверка уровня запыленности
        if (AQI < 2 && hi < 33) setColor(0, light, 0);              // GREEN! микроклимат в норме: AQI 1-2 и Тепловой индекс меньше 32 С
        else if (AQI == 2) setColor(0, 0, light);                   // BLUE! синяя тревога: AQI 2
        else if (AQI == 3) setColor(light, light, 0);               // YELLOW! желтая тревога: AQI 3
        else if (AQI == 4 || hi > 32) setColor(light, 0, light);    // ROZE! розовая тревога: AQI 4 или Тепловой индекс больше 32 С
        else if (AQI == 5 || hi > 40) setColor(light, 0, 0);        //RED! красная тревога: AQI больше 3 или Тепловой индекс больше 40 С
    } else setColor(light, 0, 0); //ОПАСНОСТЬ! красная тревога: запыленность выше допустимой
    VizIoT(serverURL, VizIoT_Device_key, VizIoT_Device_pass, temperature, hi, humidity, AQI, TVOC, CO2, getFlags, rssi, p25, p10); // Отправка данных в облако VizIoT
    delay(10000);
}
/*-------------БЛОК ФУНКЦИЙ-----------------------------------*/
// Функция вычисления теплового индекса
float calcHI(float temperature, float humidity) {
    float tf = temperature * 1.8 + 32; // конвертируем C in F
    float hi = 0.5 * (tf + 61.0 + ((tf - 68.0) * 1.2) + (humidity * 0.094));
    if (hi > 79) {
        hi = -42.379 + 2.04901523 * tf + 10.14333127 * humidity + -0.22475541 * tf * humidity + -0.00683783 * pow(tf, 2) + -0.05481717 * pow(humidity, 2) + 0.00122874 * pow(tf, 2) * humidity + 0.00085282 * tf * pow(humidity, 2) + -0.00000199 * pow(tf, 2) * pow(humidity, 2);

        if ((humidity < 13) && (tf >= 80.0) && (tf <= 112.0))
            hi -= ((13.0 - humidity) * 0.25) * sqrt((17.0 - abs(tf - 95.0)) * 0.05882);

        else if ((humidity > 85.0) && (tf >= 80.0) && (tf <= 87.0))
            hi += ((humidity - 85.0) * 0.1) * ((87.0 - tf) * 0.2);
    }
    hi = (hi - 32) * 0.55555; // конвертируем F in C
    return hi;
}
// Функция отправки данных в облако VizIoT
void VizIoT(String serverURL, String VizIoT_Device_key, String VizIoT_Device_pass, float temperature, float hi, float humidity, int AQI, int TVOC, int CO2, int getFlags, long rssi, float p25, float p10) {
    String url = serverURL + "?key=" + VizIoT_Device_key + "&pass=" + VizIoT_Device_pass + "&Temp=" + String(temperature) + "&Hic=" + String(hi) + "&Hum=" + String(humidity) + "&AQI=" + String(AQI) + "&TVOC=" + String(TVOC) + "&CO2=" + String(CO2) + "&Flag=" + String(getFlags) + "&RSSI=" + String(rssi) + "&p2.5=" + String(p25) + "&p10=" + String(p10);
    http.begin(wifiClient, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String response = http.getString();
        if (response != "OK") {
            myOLED.clrScr();
            myOLED.print("ОШИБКА!", OLED_C, 3);
            myOLED.print("Ответ сервера: " + response, OLED_C, 4);
        }
    } else {
        myOLED.clrScr();
        myOLED.print("ОШИБКА!", OLED_C, 3);
        myOLED.print("Код ошибки: " + String(httpCode), OLED_C, 4);
    }
    http.end();
}
// функция управления светодиодом
void setColor(int greenValue, int redValue, int blueValue) {
    pixels.setPixelColor(0, pixels.Color(greenValue, redValue, blueValue));
    pixels.show();
}
