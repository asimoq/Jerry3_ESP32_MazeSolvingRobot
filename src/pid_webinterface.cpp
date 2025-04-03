#include "pid_webinterface.h"
#include <vector>

#define DIRECTION_FRONT 0    // 0-egyenesen
#define DIRECTION_LEFT 1     // 1-balra
#define DIRECTION_RIGHT 2    // 2-jobbra
#define DIRECTION_STOP 3     // megállás
#define DIRECTION_START 4    // START
#define DIRECTION_DEAD_END 5 // Zsákutca

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
      button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }
      .current-value { color: #666; margin-bottom: 5px; }
      .success { color: green; margin: 10px 0; }
      .web-button { 
        background-color: #ff5722; 
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
)=====";

  // Minden változóhoz HTML elem generálása
  for (const auto &var : webVariables)
  {
    html += "<div class=\"form-group\">";
    html += "<label for=\"" + var.id + "\">" + var.name + "</label>";
    if (var.description.length() > 0)
    {
      html += "<p>" + var.description + "</p>";
    }
    html += "<p class=\"current-value\">Jelenlegi érték: " + String(*var.value) + "</p>";
    html += "<input type=\"number\" step=\"any\" id=\"" + var.id + "\" name=\"" + var.id +
            "\" value=\"" + String(*var.value) + "\" min=\"" + String(var.minValue) +
            "\" max=\"" + String(var.maxValue) + "\">";
    html += "</div>";
  }

  html += R"=====(
          <button type="submit">Értékek frissítése</button>
      </form>
  </div>
</body>
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
    

setInterval(updateDistances, 100); // Frissítés másodpercenként
</script>
</html>
)=====";

  return html;
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

void handleDistances() {
  String json = "{\"front\":" + String(distances[DIRECTION_FRONT]) + 
                ",\"left\":" + String(distances[DIRECTION_LEFT]) + 
                ",\"right\":" + String(distances[DIRECTION_RIGHT]) + "}";
  server.send(200, "application/json", json);
}

void handleButton() {
  webButtonPressed = true;
  server.send(200, "text/plain", "Gomb aktiválva");
}

void setupPidWebInterface(const char *ssid, const char *password)
{
  // Csatlakozás a WiFi hálózathoz
  Serial.print("Csatlakozas a WiFi halozathoz: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Várakozás a kapcsolat létrejöttére
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  beep(1);
  Serial.println("");
  Serial.println("WiFi kapcsolat letrejott");
  Serial.print("IP cim: ");
  Serial.println(WiFi.localIP());

  // Útvonalak beállítása a webszerverhez
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, handleUpdate);
  server.on("/distances", HTTP_GET, handleDistances);
  server.on("/button", HTTP_GET, handleButton);

  // Webszerver indítása
  server.begin();
  Serial.println("PID webinterface elindult");

  addWebVariable("pid_p", "P (Proportional)", "Arányos tag", &Pid_P, 0, 200);
  addWebVariable("pid_i", "I (Integral)", "Integráló tag", &Pid_I, 0, 200);
  addWebVariable("pid_d", "D (Derivative)", "Differenciáló tag", &Pid_D, 0, 200);
  addWebVariable("single_wall", "Faltól tartott távolság", "Távolság(cm)", &distanceFromSingleWall, 0, 50);
  addWebVariable("forward_max_speed", "Maximális sebesség", "Sebesség(0-255)", &forwardMaxSpeed, 0, 255);
  addWebVariable("distanceFromFrontWall", "Előre tartott távolság", "Távolság(cm)", &distanceFromFrontWall, 0, 50);
}

void handlePidWebClient()
{
  server.handleClient();
}
