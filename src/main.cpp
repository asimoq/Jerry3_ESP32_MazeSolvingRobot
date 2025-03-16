#include "pid_webinterface.h"

const char* ssid = "Bende_iphone";
const char* password = "Pass1234$";

// Webszerver létrehozása a 80-as porton
WebServer server(80);

// A három változó, amit a felhasználó módosíthat
float Pid_P = 0;
float Pid_I = 0;
float Pid_D = 0;

const int buzzerPin = 26;

void beep(int number) {
  // 1000 Hz-es hang generálása
  for (size_t i = 0; i < number; i++)
  {
    tone(buzzerPin, 1000/(i+1));
    delay(250/number);  // Fél másodpercig töredékéig szól amekkora szám annyi részre osztja
    noTone(buzzerPin);
    delay(250/number); // Fél másodpercig töredékéig csönd amekkora szám annyi részre osztja
  }

}
void startupTone(){
  for (size_t i = 5; i > 0; i--)
  {
    tone(buzzerPin, 1000/(i+1));
    delay(250/i);  // Fél másodpercig töredékéig szól amekkora szám annyi részre osztja
    noTone(buzzerPin);
    delay(250/i); // Fél másodpercig töredékéig csönd amekkora szám annyi részre osztja
  }
}


void setup() {
  delay(2000);
  pinMode(buzzerPin, OUTPUT);
  
  // Soros kommunikáció inicializálása
  Serial.begin(115200);
  beep(3);
  startupTone();
  // PID webinterface inicializálása és indítása
  Serial.println("\n\nPID Webinterface indítása...");
  setupPidWebInterface(ssid, password);
  

    
  
}

void loop() {
  // Webszerver kérések kezelése

  handlePidWebClient();
  
  // Itt jöhet a többi kód, ami nem kapcsolódik a webszerverhez
  // ...
  
  // Rövid késleltetés a CPU terhelés csökkentésére
  delay(10);
}