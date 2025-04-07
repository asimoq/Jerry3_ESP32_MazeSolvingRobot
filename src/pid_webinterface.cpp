#include "pid_webinterface.h"
#include <EEPROM.h>
#include <vector>

#define DIRECTION_FRONT 0    // 0-egyenesen
#define DIRECTION_LEFT 1     // 1-balra
#define DIRECTION_RIGHT 2    // 2-jobbra
#define DIRECTION_STOP 3     // megállás
#define DIRECTION_START 4    // START
#define DIRECTION_DEAD_END 5 // Zsákutca

#define EEPROM_SIZE 512
#define EEPROM_VALID_FLAG 0xAA // Jelzőbit az érvényes adatokhoz

String generateFormFields();
void saveAllSettingsToEEPROM();
bool loadAllSettingsFromEEPROM();
void handleSaveToEEPROM();

struct WebVariable
{
  String id;          // Egyedi azonosító (formok/HTML elemek számára)
  String name;        // Megjelenített név
  String description; // Opcionális leírás
  double *value;      // Mutató a változóra
  double minValue;    // Minimum érték (opcionális)
  double maxValue;    // Maximum érték (opcionális)
};
std::vector<WebVariable> webVariables;
// Függvény új változó hozzáadására
void addWebVariable(const String &id, const String &name, const String &description, double *value, double minValue = 0, double maxValue = 100)
{
  WebVariable var;
  var.id = id;
  var.name = name;
  var.description = description;
  var.value = value;
  var.minValue = minValue;
  var.maxValue = maxValue;
  webVariables.push_back(var);
}
// Sikeres frissítés jelzése
bool pidValuesUpdated = false;

String generateHtml()
{
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Változó Beállító</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; margin: 20px; }
    .container { max-width: 600px; margin: 0 auto; }
    .form-group { margin-bottom: 15px; }
    label { display: block; margin-bottom: 5px; font-weight: bold; }
    input { width: 100%; padding: 8px; box-sizing: border-box; }
    button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; margin-right: 5px; }
    .current-value { color: #666; margin-bottom: 5px; }
    .success { color: green; margin: 10px 0; }
    .web-button { 
      background-color: rgb(255, 222, 34); 
      color: white; 
      padding: 15px 20px; 
      font-size: 18px; 
      margin-bottom: 20px;
      width: 100%;
      border-radius: 5px;
    }
    .distances {
      display: flex;
      justify-content: space-between;
      margin: 20px 0;
    }
    .distance-box {
      padding: 10px;
      background-color: #f1f1f1;
      border-radius: 5px;
      width: 30%;
      text-align: center;
    }
    .button-group {
      display: flex;
      flex-wrap: wrap;
      gap: 5px;
      margin-top: 15px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 Változó Beállító</h1>
    %SUCCESS_MESSAGE%
    <button onclick="pressButton()" class="web-button">VIRTUÁLIS GOMB</button>
    <div id="distances">
      <h2>Távolságok</h2>
      <p>Elöl: <span id="distance-front"></span> cm</p>
      <p>Bal: <span id="distance-left"></span> cm</p>
      <p>Jobb: <span id="distance-right"></span> cm</p>
    </div>
    <form action="/update" method="post">
)=====" + generateFormFields() +
                R"=====(
      <div class="button-group">
        <button type='button' onclick='saveNormalSettings()' class='button' style='background-color: #FF5722;'>Normál beállítások mentése</button>
        <button type='button' onclick='loadNormalSettings()' class='button'>Normál beállítások betöltése</button>
        <button type='button' onclick='saveSprintSettings()' class='button' style='background-color: #FF5722;'>Sprint beállítások mentése</button>
        <button type='button' onclick='loadSprintSettings()' class='button'>Sprint beállítások betöltése</button>
        <button type='button' onclick='saveToEEPROM()' class='button' style='background-color:rgb(255, 60, 0);'>Mentés EEPROM-ba</button>
      </div>
      <br><br>
      <button type="submit">Értékek frissítése</button>
    </form>
  </div>
  <script>
    function pressButton() {
      fetch('/button')
        .then(response => {
          if(response.ok) {
            alert('Gomb aktiválva!');
          }
        })
        .catch(error => console.error('Hiba:', error));
    }

    function updateDistances() {
      fetch('/distances')
        .then(response => response.json())
        .then(data => {
          document.getElementById('distance-front').textContent = data.front;
          document.getElementById('distance-left').textContent = data.left;
          document.getElementById('distance-right').textContent = data.right;
        });
    }

    setInterval(updateDistances, 100);

    function saveNormalSettings() {
      const settings = {};
      document.querySelectorAll('input[type="number"]').forEach(input => {
        settings[input.id] = parseFloat(input.value);
      });
      localStorage.setItem('normalSettings', JSON.stringify(settings));
      alert('Normál beállítások sikeresen elmentve!');
    }

    function loadNormalSettings() {
      const savedSettings = localStorage.getItem('normalSettings');
      if (savedSettings) {
        const settings = JSON.parse(savedSettings);
        for (const id in settings) {
          const input = document.getElementById(id);
          if (input) {
            input.value = settings[id];
          }
        }
        alert('Normál beállítások sikeresen betöltve!');
      } else {
        alert('Nincsenek mentett normál beállítások!');
      }
    }

    function saveSprintSettings() {
      const settings = {};
      document.querySelectorAll('input[type="number"]').forEach(input => {
        settings[input.id] = parseFloat(input.value);
      });
      localStorage.setItem('sprintSettings', JSON.stringify(settings));
      alert('Sprint beállítások sikeresen elmentve!');
    }

    function loadSprintSettings() {
      const savedSettings = localStorage.getItem('sprintSettings');
      if (savedSettings) {
        const settings = JSON.parse(savedSettings);
        for (const id in settings) {
          const input = document.getElementById(id);
          if (input) {
            input.value = settings[id];
          }
        }
        alert('Sprint beállítások sikeresen betöltve!');
      } else {
        alert('Nincsenek mentett sprint beállítások!');
      }
    }
    function saveToEEPROM() {
    fetch('/save-to-eeprom')
      .then(response => {
        if(response.ok) {
          alert('Beállítások sikeresen mentve EEPROM-ba!');
        }
      })
      .catch(error => console.error('Hiba:', error));
    }
  </script>
</body>
</html>
)=====";

  return html;
}

String generateFormFields()
{
  String fields;
  for (const auto &var : webVariables)
  {
    fields += "<div class=\"form-group\">";
    fields += "<label for=\"" + var.id + "\">" + var.name + "</label>";
    if (var.description.length() > 0)
    {
      fields += "<p>" + var.description + "</p>";
    }
    fields += "<p class=\"current-value\">Jelenlegi érték: " + String(*var.value) + "</p>";
    fields += "<input type=\"number\" step=\"any\" id=\"" + var.id + "\" name=\"" + var.id +
              "\" value=\"" + String(*var.value) + "\" min=\"" + String(var.minValue) +
              "\" max=\"" + String(var.maxValue) + "\">";
    fields += "</div>";
  }
  return fields;
}

bool variablesUpdated = false;

void handleRoot()
{
  Serial.println("Főoldal lekérése");

  String html = generateHtml();

  if (variablesUpdated)
  {
    html.replace("%SUCCESS_MESSAGE%", "<p class=\"success\">Az értékek sikeresen frissítve!</p>");
    variablesUpdated = false;
  }
  else
  {
    html.replace("%SUCCESS_MESSAGE%", "");
  }

  server.send(200, "text/html", html);
}

void handleUpdate()
{
  Serial.println("Update kérés érkezett");
  bool valtozottErtek = false;

  // Összes paraméter kiírása debug célokból
  Serial.println("Beérkezett paraméterek:");
  for (int i = 0; i < server.args(); i++)
  {
    Serial.print(server.argName(i));
    Serial.print(": ");
    Serial.println(server.arg(i));
  }

  // Minden változó ellenőrzése
  for (auto &var : webVariables)
  {
    if (server.hasArg(var.id))
    {
      double ujErtek = server.arg(var.id).toDouble();
      Serial.print("Új érték a(z) ");
      Serial.print(var.name);
      Serial.print(" számára: ");
      Serial.println(ujErtek);

      // Érték frissítése, ha változott
      if (*var.value != ujErtek)
      {
        *var.value = ujErtek;
        valtozottErtek = true;
        Serial.print(var.id);
        Serial.print(" frissítve: ");
        Serial.println(*var.value);
      }
    }
  }

  if (valtozottErtek)
  {
    Serial.println("Változók frissítve:");
    for (const auto &var : webVariables)
    {
      Serial.print(var.name);
      Serial.print(": ");
      Serial.println(*var.value);
    }
    beep(5);
    variablesUpdated = true;
  }
  else
  {
    Serial.println("Nem történt értékváltozás");
  }

  // Átirányítás vissza a főoldalra
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void handleDistances()
{
  String json = "{\"front\":" + String(distances[DIRECTION_FRONT]) +
                ",\"left\":" + String(distances[DIRECTION_LEFT]) +
                ",\"right\":" + String(distances[DIRECTION_RIGHT]) + "}";
  server.send(200, "application/json", json);
}

void handleButton()
{
  webButtonPressed = true;
  server.send(200, "text/plain", "Gomb aktiválva");
}

void onWiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_AP_STACONNECTED:
    Serial.println("Egy eszköz csatlakozott az AP-hez");
    beep(1); // Egyszeri csipogás csatlakozáskor
    break;
  case SYSTEM_EVENT_AP_STADISCONNECTED:
    Serial.println("Egy eszköz lecsatlakozott az AP-ről");
    beep(2); // Kétszeri csipogás lecsatlakozáskor
    break;
  default:
    break;
  }
}

void setupPidWebInterface(const char *ssid, const char *password)
{
  // Csatlakozás a WiFi hálózathoz
  Serial.print("WiFi halozat letrehozasa: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_AP);

  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  // Soft AP konfigurálása fix IP címmel
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);

  WiFi.onEvent(onWiFiEvent);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP cím: ");
  beep(1);
  Serial.println("");

  // Útvonalak beállítása a webszerverhez
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.on("/distances", HTTP_GET, handleDistances);
  server.on("/button", HTTP_GET, handleButton);
  server.on("/save-to-eeprom", HTTP_GET, handleSaveToEEPROM);

  // Webszerver indítása
  server.begin();
  Serial.println("PID webinterface elindult");

  addWebVariable("pid_p", "P (Proportional)", "Arányos tag", &Pid_P, 0, 200);
  addWebVariable("pid_i", "I (Integral)", "Integráló tag", &Pid_I, 0, 200);
  addWebVariable("pid_d", "D (Derivative)", "Differenciáló tag", &Pid_D, 0, 200);
  addWebVariable("single_wall", "Faltól tartott távolság", "Távolság(cm)", &distanceFromSingleWall, 0, 50);
  addWebVariable("forward_max_speed", "Maximális sebesség", "Sebesség(0-255)", &forwardMaxSpeed, 0, 255);
  addWebVariable("distanceFromFrontWall", "Előre tartott távolság", "Távolság(cm)", &distanceFromFrontWall, 0, 50);
  addWebVariable("delay_time", "Időzítés", "Idő(ms)", &delayBeforeTurn, 0, 5000);

  // EEPROM inicializálása
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("Hiba az EEPROM inicializálásakor!");
    delay(1000);
    beep(10);
    ESP.restart();
  }

  // Adatok betöltése EEPROM-ból, ha van
  if (loadAllSettingsFromEEPROM())
  {
    Serial.println("Beállítások sikeresen betöltve az EEPROM-ból!");
    beep(2);
  }
  else
  {
    Serial.println("Nincsenek mentett beállítások az EEPROM-ban.");
  }
}

void handlePidWebClient()
{
  server.handleClient();
}

void handleSaveToEEPROM()
{
  saveAllSettingsToEEPROM();
  server.send(200, "text/plain", "Beállítások sikeresen mentve EEPROM-ba!");
  beep(3);
}
// Függvény az összes változó mentéséhez az EEPROM-ba
void saveAllSettingsToEEPROM()
{
  int address = 1; // Az első byte a jelzőbit

  // Jelöljük, hogy van mentett adat
  EEPROM.write(0, EEPROM_VALID_FLAG);

  // Minden webVariable mentése
  for (const auto &var : webVariables)
  {
    EEPROM.put(address, *var.value);
    address += sizeof(double); // Minden változó double típusú
  }

  // Commit a változtatásokat
  EEPROM.commit();
  Serial.println("Beállítások mentve az EEPROM-ba!");
}

// Függvény az összes változó betöltéséhez az EEPROM-ból
bool loadAllSettingsFromEEPROM()
{
  // Ellenőrizzük, hogy van-e érvényes adat
  if (EEPROM.read(0) != EEPROM_VALID_FLAG)
  {
    return false; // Nincs érvényes adat
  }

  int address = 1; // Az első byte a jelzőbit

  // Minden webVariable betöltése
  for (const auto &var : webVariables)
  {
    EEPROM.get(address, *var.value);
    address += sizeof(double);
  }
  return true; // Sikeres betöltés
}
