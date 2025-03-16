#ifndef PID_WEBINTERFACE_H
#define PID_WEBINTERFACE_H

#include <WiFi.h>
#include <WebServer.h>

// Külső változók deklarálása
extern float Pid_P;
extern float Pid_I;
extern float Pid_D;
extern WebServer server;
extern void beep(int number);

// HTML kód PROGMEM-ben tárolva
const char PID_HTML[] PROGMEM = R"=====(
<!DOCTYPE html><html>
<head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 PID Beallito</title>
<style>
body { font-family: "Lucida Console", "Courier New", monospace; text-align: center; margin: 10px; }
form { max-width: 400px; margin: 0 auto; padding: 10px; border: 1px solid #ddd; border-radius: 5px; }
input[type=number], input[type=text] { width: 100%; padding: 12px 20px; margin: 8px 0; box-sizing: border-box; }
input[type=submit] { background-color:rgb(76, 153, 175); color: white; padding: 5px 10px; margin: 8px 0; border: none; border-radius: 4px; cursor: pointer; width: 100%; }
input[type=submit]:hover { background-color:rgb(69, 116, 160); }
.param-group { margin-bottom: 10px; padding-bottom: 5px; border-bottom: 1px solid #eee; }
.success-message { color: green; font-weight: bold; margin: 5px 0; }
.slider-container { display: flex; align-items: center; margin: 5px 0; }
.slider { flex-grow: 1; margin-right: 5px; }
.slider-value { min-width: 60px; text-align: right; }
</style>
</head>
<body>
<h2>ESP32 PID Beallito</h2>
%SUCCESS_MESSAGE%
<form action="/update" method="post">
  
<div class="param-group">
<h3>P (Proportional)</h3>
<p>Jelenlegi ertek: %PID_P%</p>
<label for="pid_p">Uj ertek:</label>
<!-- Mobilbarát tizedestört beviteli mező -->
<input type="text" inputmode="decimal" id="pid_p" name="pid_p" value="%PID_P%" step="0.01">
<div class="slider-container">
  <input type="range" class="slider" id="pid_p_slider" min="0" max="10" step="0.1" value="%PID_P%">
  <span class="slider-value" id="pid_p_value">%PID_P%</span>
</div>
</div>
  
<div class="param-group">
<h3>I (Integral)</h3>
<p>Jelenlegi ertek: %PID_I%</p>
<label for="pid_i">Uj ertek:</label>
<!-- Mobilbarát tizedestört beviteli mező -->
<input type="text" inputmode="decimal" id="pid_i" name="pid_i" value="%PID_I%" step="0.01">
<div class="slider-container">
  <input type="range" class="slider" id="pid_i_slider" min="0" max="10" step="0.1" value="%PID_I%">
  <span class="slider-value" id="pid_i_value">%PID_I%</span>
</div>
</div>
  
<div class="param-group">
<h3>D (Derivative)</h3>
<p>Jelenlegi ertek: %PID_D%</p>
<label for="pid_d">Uj ertek:</label>
<!-- Mobilbarát tizedestört beviteli mező -->
<input type="text" inputmode="decimal" id="pid_d" name="pid_d" value="%PID_D%" step="0.01">
<div class="slider-container">
  <input type="range" class="slider" id="pid_d_slider" min="0" max="10" step="0.1" value="%PID_D%">
  <span class="slider-value" id="pid_d_value">%PID_D%</span>
</div>
</div>
  
<input type="submit" value="Frissites">
</form>

<script>
// Csúszkák és beviteli mezők összekapcsolása
function setupSlider(sliderId, inputId, valueId) {
  const slider = document.getElementById(sliderId);
  const input = document.getElementById(inputId);
  const valueDisplay = document.getElementById(valueId);
  
  slider.oninput = function() {
    input.value = this.value;
    valueDisplay.textContent = this.value;
  };
  
  input.oninput = function() {
    const value = parseFloat(this.value);
    if (!isNaN(value)) {
      slider.value = value;
      valueDisplay.textContent = value;
    }
  };
}

// Csúszkák inicializálása
setupSlider('pid_p_slider', 'pid_p', 'pid_p_value');
setupSlider('pid_i_slider', 'pid_i', 'pid_i_value');
setupSlider('pid_d_slider', 'pid_d', 'pid_d_value');
</script>
</body></html>
)=====";

// Webszerver kezelő függvények deklarációi
void setupPidWebInterface(const char* ssid, const char* password);
void handlePidRoot();
void handlePidUpdate();
void handlePidWebClient();

// Sikeres frissítés jelzése
extern bool pidValuesUpdated;

#endif
