#include "pid_webinterface.h"
#include <Arduino.h>
#include <MPU6050_light.h> //gyro könyvtár
#include <SPI.h>
#include <MFRC522v2.h> // RFID könyvtár
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

#include <string.h>
#include <stdio.h>
#include "Wire.h"
#include <PID_v1.h>
#include <math.h>


#define DIRECTION_FRONT 0    // 0-egyenesen
#define DIRECTION_LEFT 1     // 1-balra
#define DIRECTION_RIGHT 2    // 2-jobbra
#define DIRECTION_STOP 3     // megállás
#define DIRECTION_START 4    // START
#define DIRECTION_DEAD_END 5 // Zsákutca

// gyro
MPU6050 mpu(Wire);
unsigned long timer = 0;
float lastCorrectAngle = 0;

// IR sensor pins (analog inputs)
#define IR_PIN_LEFT 34  // Front IR sensor connected to analog pin A1
#define IR_PIN_RIGHT 36 // Right IR sensor connected to analog pin A2
#define IR_PIN_FRONT 39 // Left IR sensor connected to analog pin A0

struct KalmanFilter {
  double estimate;     // Becsült érték
  double errorCovariance;  // Becslési hiba kovariancia
  double processNoise; // Folyamat zaj (Q)
  double measurementNoise; // Mérési zaj (R)
};

KalmanFilter frontKalman = {0, 1.0, 0.01, 0.1};  // Elülső szenzor
KalmanFilter leftKalman = {0, 1.0, 0.01, 0.1};   // Bal oldali szenzor
KalmanFilter rightKalman = {0, 1.0, 0.01, 0.1};  // Jobb oldali szenzor

double distanceFromSingleWall = 10;   // hány cm-re van a fal ha csak egyhez igazodik 11.5
double distanceFromSingleWallTreshold = distanceFromSingleWall;
double distanceFromFrontWall = 10;  // mennyire van messze az elötte lévő fal

const char *ssid = "Bende_iphone";
const char *password = "Pass1234$";

// Webszerver létrehozása a 80-as porton
WebServer server(80);

// A három változó, amit a felhasználó módosíthat

#define BUZZER_PIN 12

const int buttonPin = 2; // A gomb a GPIO2-höz van csatlakoztatva

double distances[3];
int commands[256];
int currentCommand;

// motor pinek
#define ENB 14 // bal
#define IN4 27
#define IN3 16
#define IN2 17 // jobb
#define IN1 25
#define ENA 26

// motor speedek
int turnMaxSpeed = 190;
int turnMinSpeed = 110;
int turnProportionalSpeed = turnMaxSpeed - turnMinSpeed;

double forwardMaxSpeed = 180;
int forwardMinSpeed = 110;
int forwardProportionalSpeed = forwardMaxSpeed - forwardMinSpeed;

// PID változók   //100 hoz egsz okes: 0.3 0.3 0.9  //60hoz:
int pidmode = 2;
double setpoint = 0; // Kívánt érték
double input, output;
// double Kp1 = 30, Ki1 = 0, Kd1 = 30; // PID tényezők
double Pid_P = 1, Pid_I = 0, Pid_D = 0; // PID tényezők
PID pid(&input, &output, &setpoint, Pid_P, Pid_I, Pid_D, DIRECT);



// RFID CONFIG
#define RST_PIN 13
#define SS_PIN 5
MFRC522DriverPinSimple ss_pin(SS_PIN);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

void beep(int number);
void startupTone();
void checkButton();
void drive(int motorSpeedLeft, int motorSpeedRight);
double measureDistance(int analogPin);
double measureFrontDistance(int analogPin);
void forward();
void backward();
void stop();
void turnLeft(double desiredangle);
void turnRight(double desiredangle);
bool thereIsAWall(int direction);
void PidDrive(double distanceFromMiddle, int maxSpeed, bool isThereAWall);
double measureFrontDistanceWithFilter(int trigerpin);
void measureDistanceAllDirections();
void forwardWithAlignment(int maxSpeed);
int rfidToDirection(int *dirs);
void orientRobot(double desiredAngle);
void handlePidSettings();
double kalmanFilter(double measurement, KalmanFilter* filter);

void setup()
{
  delay(1000);
  pinMode(BUZZER_PIN, OUTPUT);

  // Soros kommunikáció inicializálása
  Serial.begin(115200);
  startupTone();
  // PID webinterface inicializálása és indítása
  Serial.println("\n\nPID Webinterface indítása...");
  setupPidWebInterface(ssid, password);
  Serial.println("PID Webinterface elindult");
  Serial.println("PID Webinterface elérhető a következő címen: ");
  Serial.println(WiFi.localIP());

  // gyro beállítása
  Wire.begin();
  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while (status != 0)
  {
  } // stop everything if could not connect to MPU6050

  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets(); // gyro and accelero
  Serial.println("Done!\n");

  // Motorvezérlő pin-ek beállítása
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(buttonPin, OUTPUT); // Gomb bemenetének beállítása

  // RFID kártyaolvasó inicializálása

  SPI.begin();
  mfrc522.PCD_Init();
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);
  mfrc522.PCD_AntennaOn();
  mfrc522.PCD_SetAntennaGain(0x07); // 0x07 = 48dB

  pid.SetMode(AUTOMATIC);
  pid.SetOutputLimits(-255, 255);
  pid.SetSampleTime(25);

  mpu.update();
  lastCorrectAngle = mpu.getAngleZ();

  commands[0] = 0;
  currentCommand = 0;

  delay(1000);
  beep(3);
  // várunk a gomb megnyomására
  while (digitalRead(buttonPin) == LOW)
  {
    delay(100);
  }
  {
    beep(3);
    delay(500);
  }
}

void loop()
{
  // PID webinterface kezelése
  measureDistanceAllDirections();
  handlePidSettings();
  pid.SetTunings(Pid_P, Pid_I, Pid_D);
  while (distances[DIRECTION_FRONT] > distanceFromFrontWall)
  {
    measureDistanceAllDirections();
    handlePidSettings();
    forwardWithAlignment(forwardMaxSpeed);
    mpu.update();
    switch (rfidToDirection(commands))
    {
    case DIRECTION_LEFT:
      delay(1000);
      turnLeft(85);
      break;
    case DIRECTION_RIGHT:
      delay(1000);
      turnRight(85);
      break;
    case DIRECTION_STOP:
      stop();
      drive(90, -90);
      delay(2000);
      while (true)
      {
        stop();
      }
      break;

    default:
      break;
    }
  }
  stop();
  if (distances[DIRECTION_LEFT] >= distances[DIRECTION_RIGHT])
  {
    turnLeft(85);
    measureDistanceAllDirections();
  }
  else
  {
    turnRight(85);
    measureDistanceAllDirections();
  }

}

void handlePidSettings(){
  handlePidWebClient();
  pid.SetTunings(Pid_P, Pid_I, Pid_D);
}

void beep(int number)
{
  // 1000 Hz-es hang generálása
  for (size_t i = 0; i < number; i++)
  {
    tone(BUZZER_PIN, 1000 / (i + 1));
    delay(250 / number); // Fél másodpercig töredékéig szól amekkora szám annyi részre osztja
    noTone(BUZZER_PIN);
    delay(250 / number); // Fél másodpercig töredékéig csönd amekkora szám annyi részre osztja
  }
}

void startupTone()
{
  for (size_t i = 5; i > 0; i--)
  {
    tone(BUZZER_PIN, 1000 / (i + 1));
    delay(250 / i); // Fél másodpercig töredékéig szól amekkora szám annyi részre osztja
    noTone(BUZZER_PIN);
    delay(250 / i); // Fél másodpercig töredékéig csönd amekkora szám annyi részre osztja
  }
}

void checkButton()
{
  if (digitalRead(buttonPin) == HIGH)
  {
    stop();
    beep(3);
    delay(500);
    while (digitalRead(buttonPin) == LOW)
    {
      delay(100);
      handlePidSettings();
      measureDistanceAllDirections();
    }
    {
      beep(3);
      delay(500);
    }
  }
}

// motorbeállítás
void drive(int motorSpeedLeft, int motorSpeedRight)
{
  // Motor A irányának beállítása
  if (motorSpeedLeft >= 0)
  {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  else
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    motorSpeedLeft = -motorSpeedLeft;
  }

  // Motor B irányának beállítása
  if (motorSpeedRight >= 0)
  {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  else
  {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    motorSpeedRight = -motorSpeedRight;
  }

  // Motorok sebességének beállítása
  analogWrite(ENA, motorSpeedLeft);
  analogWrite(ENB, motorSpeedRight);
  checkButton();
}

// TÁVOLSÁGMÉRÉS CM-BEN
//  TÁVOLSÁGMÉRÉS CM-BEN IR szenzorral
double measureDistance(int analogPin, KalmanFilter* kalmanFilterInstance = nullptr) {
  int rawValue = analogRead(analogPin);
  
  // Lookup tábla a megadott értékpárokkal (analóg érték -> cm)
  const int lookupSize = 11;
  const int analogValues[lookupSize] = {2500, 1500, 880, 600, 450, 405, 370, 330, 300, 150, 100};
  const double distances[lookupSize] = {1.0, 2.5, 5.0, 7.5, 10.0, 12.5, 15.0, 17.5, 20.0, 35.0, 45.0};
  
  double rawDistance = 0;
  
  // Ha kisebb az érték mint a legkisebb a táblában (100), akkor 45cm-nél nagyobb
  if (rawValue < 100) {
    rawDistance = 50.0;
  }
  // Ha nagyobb az érték mint a legnagyobb a táblában (2500), akkor 1cm
  else if (rawValue > 2500) {
    rawDistance = 1.0;
  }
  else {
    // Keressük meg a két szomszédos értéket és interpoláljunk
    for (int i = 0; i < lookupSize - 1; i++) {
      if (rawValue <= analogValues[i] && rawValue > analogValues[i+1]) {
        // Lineáris interpoláció
        double proportion = (double)(rawValue - analogValues[i+1]) / (analogValues[i] - analogValues[i+1]);
        rawDistance = distances[i] + (1.0 - proportion) * (distances[i+1] - distances[i]);
        break;
      }
    }
    
    // Fallback, ha nem találtunk megfelelő értéket
    if (rawDistance == 0) {
      rawDistance = 15.0;
    }
  }
  
  // Ha van megadva Kalman-szűrő, alkalmazzuk
  if (kalmanFilterInstance != nullptr) {
    return kalmanFilter(rawDistance, kalmanFilterInstance);
  }
  
  return rawDistance;
}

// Előre néző távolságmérő függvény Kalman-szűrővel
double measureFrontDistance(int analogPin, KalmanFilter* kalmanFilterInstance = nullptr) {
  int rawValue = analogRead(analogPin);
  
  // Lookup tábla a megadott értékpárokkal (analóg érték -> cm)
  const int lookupSize = 8;
  const int analogValues[lookupSize] = {2000, 1200, 750, 500, 375, 300, 200, 150};
  const double distances[lookupSize] = {5, 10, 15, 20, 25, 30, 35, 40};
  
  double rawDistance = 0;
  
  // Ha kisebb az érték mint a legkisebb a táblában (150), akkor 70cm
  if (rawValue < 150) {
    rawDistance = 70.0;
  }
  // Ha nagyobb az érték mint a legnagyobb a táblában (2000), akkor 5cm
  else if (rawValue > 2000) {
    rawDistance = 5.0;
  }
  else {
    // Keressük meg a két szomszédos értéket és interpoláljunk
    for (int i = 0; i < lookupSize - 1; i++) {
      if (rawValue <= analogValues[i] && rawValue > analogValues[i+1]) {
        // Lineáris interpoláció
        double proportion = (double)(rawValue - analogValues[i+1]) / (analogValues[i] - analogValues[i+1]);
        rawDistance = distances[i+1] + proportion * (distances[i] - distances[i+1]);
        break;
      }
    }
    
    // Fallback, ha nem találtunk megfelelő értéket
    if (rawDistance == 0) {
      rawDistance = 60000.0 * pow(rawValue, -1.05) + 3.0;
    }
  }
  
  // Ha van megadva Kalman-szűrő, alkalmazzuk
  if (kalmanFilterInstance != nullptr) {
    return kalmanFilter(rawDistance, kalmanFilterInstance);
  }
  
  return rawDistance;
}

// Előre haladás
void forward()
{
  drive(80, 80);
}

// Hátramenet
void backward()
{
  drive(-80, -80);
}

// Megállás
void stop()
{
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// Balra fordulás 90 fok
void turnLeft(double desiredangle)
{

  mpu.update();                       // gyro frissítése
  float startAngle = mpu.getAngleZ(); // gyro mérése és aktuális állapot mentése
  float currentAngle = mpu.getAngleZ();

  drive(-90, 90); // fordulás megkezdése

  while (currentAngle <= startAngle + desiredangle * 2)
  {               // várakozás amíg el nem értük a kívánt fokot. lehet több vagy kevesebb a kívánt fok.
    mpu.update(); // gyro frissítés
    currentAngle = mpu.getAngleZ();
    howFareAreWeFromDestinacion = ((startAngle + desiredangle) - currentAngle) / desiredangle;
    drive(-constrain((turnMinSpeed + (turnProportionalSpeed * howFareAreWeFromDestinacion)), turnMinSpeed, turnMaxSpeed), constrain((turnMinSpeed + (turnProportionalSpeed * howFareAreWeFromDestinacion)), turnMinSpeed, turnMaxSpeed));
  }
  drive(80, -80);
  delay(60);
  stop(); // leállítás mert elértük a kívánt fokot

  // jelenlegi helyzet elmentése egy globális változóba. Ezt a helyzetet használjuk egyenesen haladáshoz amennyiben nincsenek falak.
  mpu.update();
  lastCorrectAngle = mpu.getAngleZ();
}

// Jobbra fordulás 90 fok. Magyarázatért look up turnLeft()
void turnRight(double desiredangle)
{
  mpu.update();
  float startAngle = mpu.getAngleZ(); // gyro mérése és aktuális állapot mentése
  float currentAngle = mpu.getAngleZ();

  drive(90, -90);
  while (currentAngle >= startAngle - desiredangle * 2)
  {               // lehet több vagy kevesebb a kívánt fok
    mpu.update(); // gyro frissítés
    currentAngle = mpu.getAngleZ();
    howFareAreWeFromDestinacion = (currentAngle - (startAngle - desiredangle)) / desiredangle;
    drive(constrain((turnMinSpeed + (turnProportionalSpeed * howFareAreWeFromDestinacion)), turnMinSpeed, turnMaxSpeed), -constrain((turnMinSpeed + (turnProportionalSpeed * howFareAreWeFromDestinacion)), turnMinSpeed, turnMaxSpeed));
  }

  drive(-80, 80);
  delay(60);

  stop();
  mpu.update();
  lastCorrectAngle = mpu.getAngleZ();
}

// Eldönti hogy az adott irányban van e fal 0cm és 15cm között. Irányt és egy távoságok tömböt vár
bool thereIsAWall(int direction)
{
  if (distances[direction] >= 1 && distances[direction] <= (distanceFromSingleWall + distanceFromSingleWallTreshold))
    return true;
  else
    return false;
}

// PID alapján beállítja a motorok sebességét
void PidDrive(double distanceFromMiddle, int maxSpeed, bool isThereAWall)
{
  input = distanceFromMiddle;
  pid.Compute();
  // Motorok vezérlése a PID kimenet alapján
  int motorSpeedLeft = constrain(maxSpeed - output, -50, 255);  // Bal motor sebessége
  int motorSpeedRight = constrain(maxSpeed + output, -50, 255); // Jobb motor sebessége

  // Motorok mozgatása
  drive(motorSpeedLeft, motorSpeedRight);

  // jelenlegi helyzet elmentése egy globális változóba. Ezt a helyzetet használjuk egyenesen haladáshoz amennyiben nincsenek falak.
  if (isThereAWall)
  {
    lastCorrectAngle = mpu.getAngleZ();
  }
}


// feltölt egy double tömböt távolságokkal - előre, balra és jobbra mér
void measureDistanceAllDirections() {
  distances[DIRECTION_FRONT] = measureFrontDistance(IR_PIN_FRONT, &frontKalman);
  distances[DIRECTION_RIGHT] = measureDistance(IR_PIN_RIGHT, &rightKalman);
  distances[DIRECTION_LEFT] = measureDistance(IR_PIN_LEFT, &leftKalman);
}

// összetett függvény ami a körülötte lévő falak számától függően középre rendezi a robotot miközben előrefele halad.
void forwardWithAlignment(int maxSpeed)
{

  // mindkét oldalt van fal
  if (thereIsAWall(DIRECTION_LEFT) && thereIsAWall(DIRECTION_RIGHT))
  {

    // Számold ki a középső távolságot a jobb és bal oldali távolságok alapján
    double distanceFromMiddle = (distances[DIRECTION_RIGHT] - distances[DIRECTION_LEFT]);

    PidDrive(distanceFromMiddle, maxSpeed, true);
  }
  // balra van csak fal
  if (thereIsAWall(DIRECTION_LEFT) && !thereIsAWall(DIRECTION_RIGHT))
  {

    // Számold ki a középső távolságot a jobb és bal oldali távolságok alapján
    double distanceFromMiddle = (distanceFromSingleWall - distances[DIRECTION_LEFT]);

    PidDrive(distanceFromMiddle, maxSpeed, true);
  }
  // jobbra van csak fal
  if (!thereIsAWall(DIRECTION_LEFT) && thereIsAWall(DIRECTION_RIGHT))
  {

    // Számold ki a középső távolságot a jobb és bal oldali távolságok alapján
    double distanceFromMiddle = (distances[DIRECTION_RIGHT] - distanceFromSingleWall);

    PidDrive(distanceFromMiddle, maxSpeed, true);
  }
  // Nincs fal mellette
  if (!thereIsAWall(DIRECTION_LEFT) && !thereIsAWall(DIRECTION_RIGHT))
  {
    float angle = mpu.getAngleZ();
    double error = (angle - lastCorrectAngle) * 0.07; // gyro alapján egyenesen a legutóbbi helyezkedéstől(falhoz igazítás vagy fordulás) számolva tartja a szöget elvileg :D
    PidDrive(error, maxSpeed, false);
  }
}

// RFID kártya direction outputtal. -1 értéket ad vissza, ha rossz az olvasás.
//  dirs pointer opcionális az első fordulóban.
int rfidToDirection(int *dirs = nullptr)
{
  delay(50);
  int retVal[4] = {0};
  if (dirs == nullptr)
  {
    dirs = retVal;
  }

  if (mfrc522.PICC_IsNewCardPresent())
  {
    beep(1);
    if (mfrc522.PICC_ReadCardSerial())
    {
      if ((mfrc522.uid.uidByte[1] == 0xBC))
      {
        switch (mfrc522.uid.uidByte[2] & 0xF0)
        {
        case 0xC0:
          /* START */
          dirs[0] = DIRECTION_START;
          break;
        case 0x50:
          /* STOP */
          dirs[0] = DIRECTION_STOP;
          break;
        case 0xF0:
          /* JOBBRA */
          dirs[0] = DIRECTION_RIGHT;
          break;
        case 0x00:
          /* BALRA */
          dirs[0] = DIRECTION_LEFT;
          break;
        case 0x90:
        case 0xA0:
          /* EJJB */
          dirs[0] = DIRECTION_FRONT;
          dirs[1] = DIRECTION_RIGHT;
          dirs[2] = DIRECTION_RIGHT;
          dirs[3] = DIRECTION_LEFT;
        default:
          /* bro baj van */
          dirs[0] = -1;
          break;
        }
      }
      else if (mfrc522.uid.uidByte[1] == 0xBD)
      {
        switch (mfrc522.uid.uidByte[2] & 0xF0)
        {
        case 0xD0:
        case 0xE0:
        case 0xF0:
          /* ZSÁK UTCA */
          dirs[0] = DIRECTION_DEAD_END;
          break;
        case 0x00:
          /* BJJ */
          dirs[0] = DIRECTION_LEFT;
          dirs[1] = DIRECTION_RIGHT;
          dirs[2] = DIRECTION_RIGHT;
          break;
        case 0x60:
          /* JBB */
          dirs[0] = DIRECTION_RIGHT;
          dirs[1] = DIRECTION_LEFT;
          dirs[2] = DIRECTION_LEFT;
          break;
        default:
          /* BRO BAJ VAN */
          dirs[0] = -1;
          break;
        }
      }
      mfrc522.PICC_HaltA();
      return dirs[0];
    }
  }
  return -2;
}

double kalmanFilter(double measurement, KalmanFilter* filter) {
  // Előrejelzési lépés
  double predictedEstimate = filter->estimate;
  double predictedErrorCovariance = filter->errorCovariance + filter->processNoise;
  
  // Frissítési lépés
  double kalmanGain = predictedErrorCovariance / (predictedErrorCovariance + filter->measurementNoise);
  filter->estimate = predictedEstimate + kalmanGain * (measurement - predictedEstimate);
  filter->errorCovariance = (1 - kalmanGain) * predictedErrorCovariance;
  
  return filter->estimate;
}

void orientRobot(double desiredAngle)
{
  mpu.update();
  if (abs(mpu.getAngleZ() - desiredAngle) > 10)
  {
    if (mpu.getAngleZ() > desiredAngle)
    {
      turnRight((mpu.getAngleZ() - desiredAngle) / 2);
    }
    else
    {
      turnLeft(abs(mpu.getAngleZ() - desiredAngle) / 2);
    }
  }
}