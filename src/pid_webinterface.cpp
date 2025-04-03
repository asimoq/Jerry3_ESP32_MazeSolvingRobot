#include "pid_webinterface.h"
#include <vector>

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
  </style>
</head>
<body>
  <div class="container">
      <h1>ESP32 Változó Beállító</h1>
      %SUCCESS_MESSAGE%
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

  // Webszerver indítása
  server.begin();
  Serial.println("PID webinterface elindult");

  addWebVariable("pid_p", "P (Proportional)", "Arányos tag", &Pid_P, 0, 200);
  addWebVariable("pid_i", "I (Integral)", "Integráló tag", &Pid_I, 0, 200);
  addWebVariable("pid_d", "D (Derivative)", "Differenciáló tag", &Pid_D, 0, 200);
  addWebVariable("single_wall", "Faltól tartott távolság", "Távolság(cm)", &distanceFromSingleWall, 0, 50);
}

void handlePidWebClient()
{
  server.handleClient();
}
