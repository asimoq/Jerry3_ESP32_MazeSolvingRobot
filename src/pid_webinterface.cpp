#include "pid_webinterface.h"

// Sikeres frissítés jelzése
bool pidValuesUpdated = false;

// Webszerver kezelő függvények implementációja
void handlePidRoot() {
  Serial.println("Főoldal lekérése");
  
  // HTML sablon beolvasása és a változók behelyettesítése
  String html = FPSTR(PID_HTML);
  html.replace("%PID_P%", String(Pid_P));
  html.replace("%PID_I%", String(Pid_I));
  html.replace("%PID_D%", String(Pid_D));
  
  // Sikeres frissítés üzenet megjelenítése, ha volt frissítés
  if (pidValuesUpdated) {
    html.replace("%SUCCESS_MESSAGE%", "<div class=\"success-message\">Értékek sikeresen frissítve!</div>");
    pidValuesUpdated = false; // Reset flag
  } else {
    html.replace("%SUCCESS_MESSAGE%", ""); // Nincs üzenet
  }
  
  server.send(200, "text/html", html);
}

void handlePidUpdate() {
  Serial.println("Update kérés érkezett");
  
  bool valtozottErtek = false;
  
  // Debug: kiírjuk az összes beérkező paramétert
  Serial.println("Beérkezett paraméterek:");
  for (int i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print(": ");
    Serial.println(server.arg(i));
  }
  
  if (server.hasArg("pid_p")) {
    double ujErtek = server.arg("pid_p").toDouble();
    Serial.print("Új P érték: ");
    Serial.println(ujErtek);
    
    
    if (Pid_P != ujErtek) {
      Pid_P = ujErtek;
      valtozottErtek = true;
      Serial.print("Pid_P frissitve: ");
      Serial.println(Pid_P);
    }
  }
  
  if (server.hasArg("pid_i")) {
    double ujErtek = server.arg("pid_i").toDouble();
    Serial.print("Új I érték: ");
    Serial.println(ujErtek);
    
    
    if (Pid_I != ujErtek) {
      Pid_I = ujErtek;
      valtozottErtek = true;
      Serial.print("Pid_I frissitve: ");
      Serial.println(Pid_I);
    }
  }
  
  if (server.hasArg("pid_d")) {
    double ujErtek = server.arg("pid_d").toDouble();
    Serial.print("Új D érték: ");
    Serial.println(ujErtek);
    
    
    if (Pid_D != ujErtek) {
      Pid_D = ujErtek;
      valtozottErtek = true;
      Serial.print("Pid_D frissitve: ");
      Serial.println(Pid_D);
    }
  }
  
  if (valtozottErtek) {
    Serial.println("PID ertekek frissitve:");
    Serial.print("P: ");
    Serial.print(Pid_P);
    Serial.print(", I: ");
    Serial.print(Pid_I);
    Serial.print(", D: ");
    Serial.println(Pid_D);
    beep(5);
    // Jelezzük, hogy volt frissítés
    pidValuesUpdated = true;
  } else {
    Serial.println("Nem történt értékváltozás");
  }
  
  // Átirányítás vissza a főoldalra
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setupPidWebInterface(const char* ssid, const char* password) {
  // Csatlakozás a WiFi hálózathoz
  Serial.print("Csatlakozas a WiFi halozathoz: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Várakozás a kapcsolat létrejöttére
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  beep(1);
  Serial.println("");
  Serial.println("WiFi kapcsolat letrejott");
  Serial.print("IP cim: ");
  Serial.println(WiFi.localIP());
  
  // Útvonalak beállítása a webszerverhez
  server.on("/", HTTP_GET, handlePidRoot);
  server.on("/update", HTTP_POST, handlePidUpdate);
  
  // Webszerver indítása
  server.begin();
  Serial.println("PID webinterface elindult");
}

void handlePidWebClient() {
  server.handleClient();
}
