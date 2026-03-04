// Настройки WiFi
const int rows = 3,
    columns = 2;
char * link[rows][columns]
{
    {
        "ChongQiu",
        "Tori-man2021"
    }, // WiFi сеть София
    {
        "Polygon-502",
        "Admin344"
    }, // WiFi сеть школа
    {
        "Project-1557",
        "rO0fe9di"
    } // WiFi сеть проекта
};
WiFiClient wifiClient;
HTTPClient http;
void setup() {
    // Подключение к WiFi
    WiFi.begin(link[0]); // попытка подключения к первой известной сети
    while (WiFi.status() = !WL_CONNECTED) {
        for (int i = 0; i < rows; i++) {
            WiFi.begin(link[i]); // подключение к известной сети
            String linked = "link: " + String(link[i][0]); // имя сети
            myOLED.print(linked, OLED_C, 2);
            if (WiFi.status() = WL_CONNECTED) {
                break;
            }
            else {
                delay(1000);
                myOLED.print("                ", OLED_C, 2); // очистка строки с именем сети
            }
        }
    }
    long rssi = WiFi.RSSI();
    String status = "Сигнал WiFi: " + String(rssi) + " dB";
    myOLED.print(status, OLED_C, 3);
}
