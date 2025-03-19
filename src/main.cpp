#include "pid_webinterface.h"
#include <Arduino.h>
#include <MPU6050_light.h> //gyro könyvtár
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include <stdio.h>
#include "Wire.h"
#include <PID_v1.h>

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
#define IR_PIN_FRONT A1 // Front IR sensor connected to analog pin A1
#define IR_PIN_RIGHT A2 // Right IR sensor connected to analog pin A2
#define IR_PIN_LEFT A0  // Left IR sensor connected to analog pin A0

const char *ssid = "Bende_iphone";
const char *password = "Pass1234$";

// Webszerver létrehozása a 80-as porton
WebServer server(80);

// A három változó, amit a felhasználó módosíthat

const int buzzerPin = 26;

double distances[3];
double lastDistances[3];
bool isFirstMeasurement = true;
int commands[256];
int currentCommand;
double howFareAreWeFromDestinacion;

// motor pinek
#define ENA 5 // bal
#define IN1 3
#define IN2 4
#define IN3 10 // jobb
#define IN4 7
#define ENB 6

// motor speedek
int turnMaxSpeed = 170;
int turnMinSpeed = 75;
int turnProportionalSpeed = turnMaxSpeed - turnMinSpeed;

int forwardMaxSpeed = 90;
int forwardMinSpeed = 78;
int forwardProportionalSpeed = forwardMaxSpeed - forwardMinSpeed;

// PID változók   //100 hoz egsz okes: 0.3 0.3 0.9  //60hoz:
int pidmode = 2;
double setpoint = 0; // Kívánt érték
double input, output;
// double Kp1 = 30, Ki1 = 0, Kd1 = 30; // PID tényezők
double Pid_P = 15, Pid_I = 0.1, Pid_D = 5; // PID tényezők
PID pid(&input, &output, &setpoint, Pid_P, Pid_I, Pid_D, DIRECT);

double distanceFromSingleWall = 10; // hány cm-re van a fal ha csak egyhez igazodik 11.5

// RFID CONFIG
#define RST_PIN 8
#define SS_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void beep(int number)
{
  // 1000 Hz-es hang generálása
  for (size_t i = 0; i < number; i++)
  {
    tone(buzzerPin, 1000 / (i + 1));
    delay(250 / number); // Fél másodpercig töredékéig szól amekkora szám annyi részre osztja
    noTone(buzzerPin);
    delay(250 / number); // Fél másodpercig töredékéig csönd amekkora szám annyi részre osztja
  }
}
void startupTone()
{
  for (size_t i = 5; i > 0; i--)
  {
    tone(buzzerPin, 1000 / (i + 1));
    delay(250 / i); // Fél másodpercig töredékéig szól amekkora szám annyi részre osztja
    noTone(buzzerPin);
    delay(250 / i); // Fél másodpercig töredékéig csönd amekkora szám annyi részre osztja
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
}

// TÁVOLSÁGMÉRÉS CM-BEN
//  TÁVOLSÁGMÉRÉS CM-BEN IR szenzorral
double measureDistance(int analogPin)
{

  int sensorValue = analogRead(analogPin); // IR szenzor analóg értékének beolvasása

  // Az alábbiakban egy példa konverziót használunk, ami az IR szenzor jellegzetességeitől függ.
  // A gyártó specifikációja szerint egy konkrét érzékelési görbe van, amit ki kell számolni.
  // Egy egyszerű példa alapján (lineáris közelítés, de a szenzorra specifikus görbét kell használni):

  double voltage = sensorValue * (5.0 / 1023.0); // Feszültség számítása (5V a referencia feszültség)

  // A szenzor adatlapján található feszültség-távolság görbe alapján kell meghatározni a képletet.
  // Például egy egyszerű konverzió 4 cm és 30 cm közötti távolságra:

  if (voltage == 0)
    return 0; // Ha a feszültség 0, nincs mért távolság

  // A távolság kiszámítása a feszültség alapján (ez egy példaképlet, pontos görbe kell)
  double distanceCm = (4 / (voltage - 0.042)) - 0.42;

  // Az érzékelési tartományon kívül eső értékek szűrése
  if (distanceCm < 2)
  {
    return 0; // Érzékelési tartományon kívül
  }

  if (distanceCm > 45.0)
  {
    return 45; // Érzékelési tartományon kívül
  }

  return distanceCm;
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
void turnRight(double desiredangle)
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
void turnLeft(double desiredangle)
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
bool thereIsAWall(int direction, double distances[])
{
  double singleDistance;
  singleDistance = distances[direction];
  if (singleDistance >= 1 && singleDistance <= 22)
    return true; // 22 kb jó
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

double measureFrontDistanceWithFilter(int trigerpin)
{
  unsigned int numberOfMeasurements = 4;
  double treshold = 10;
  unsigned int NumberOfMatchesNeeded = 3;
  double frontdistances[numberOfMeasurements];
  for (size_t i = 0; i < numberOfMeasurements; i++)
  {
    frontdistances[i] = measureDistance(trigerpin);
    delay(10);
  }
  for (size_t i = 0; i < numberOfMeasurements; i++)
  {
    int matched = 0;
    for (size_t j = 0; j < numberOfMeasurements; j++)
    {
      if (abs(frontdistances[i] - frontdistances[j]) < treshold)
        matched++;
    }
    if (matched - 1 > NumberOfMatchesNeeded)
      return frontdistances[i];
  }
  double sum, avg;
  sum = 0;
  avg = 0;

  for (int i = 0; i < numberOfMeasurements; i++)
  {
    sum += frontdistances[i];
  }
  avg = sum / numberOfMeasurements;
  return avg;
}
// feltölt egy double tömböt távolságokkal - előre, balra és jobbra mér
void measureDistanceAllDirections()
{
  distances[DIRECTION_FRONT] = measureDistance(IR_PIN_FRONT);
  distances[DIRECTION_RIGHT] = measureDistance(IR_PIN_RIGHT);
  distances[DIRECTION_LEFT] = measureDistance(IR_PIN_LEFT);
}

// összetett függvény ami a körülötte lévő falak számától függően középre rendezi a robotot miközben előrefele halad.
void forwardWithAlignment(int maxSpeed)
{

  // mindkét oldalt van fal
  if (thereIsAWall(DIRECTION_LEFT, distances) && thereIsAWall(DIRECTION_RIGHT, distances))
  {

    // Számold ki a középső távolságot a jobb és bal oldali távolságok alapján
    double distanceFromMiddle = (distances[DIRECTION_RIGHT] - distances[DIRECTION_LEFT]) / 2.0;

    PidDrive(distanceFromMiddle, maxSpeed, true);
  }
  // balra van csak fal
  if (thereIsAWall(DIRECTION_LEFT, distances) && !thereIsAWall(DIRECTION_RIGHT, distances))
  {

    // Számold ki a középső távolságot a jobb és bal oldali távolságok alapján
    double distanceFromMiddle = (distanceFromSingleWall - distances[DIRECTION_LEFT]) / 2.0;

    PidDrive(distanceFromMiddle, maxSpeed, true);
  }
  // jobbra van csak fal
  if (!thereIsAWall(DIRECTION_LEFT, distances) && thereIsAWall(DIRECTION_RIGHT, distances))
  {

    // Számold ki a középső távolságot a jobb és bal oldali távolságok alapján
    double distanceFromMiddle = (distances[DIRECTION_RIGHT] - distanceFromSingleWall) / 2.0;

    PidDrive(distanceFromMiddle, maxSpeed, true);
  }
  // Nincs fal mellette
  if (!thereIsAWall(DIRECTION_LEFT, distances) && !thereIsAWall(DIRECTION_RIGHT, distances))
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
  int retVal[4] = {0};
  if (dirs == nullptr)
  {
    dirs = retVal;
  }

  if (mfrc522.PICC_IsNewCardPresent())
  {
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
      return dirs[0];
    }
  }
  return -2;
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

void setup()
{
  delay(2000);
  pinMode(buzzerPin, OUTPUT);

  // Soros kommunikáció inicializálása
  Serial.begin(115200);
  beep(3);
  startupTone();
  // PID webinterface inicializálása és indítása
  Serial.println("\n\nPID Webinterface indítása...");
  setupPidWebInterface(ssid, password);

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

  // RFID kártyaolvasó inicializálása
  SPI.begin();
  mfrc522.PCD_Init();
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  pid.SetMode(AUTOMATIC);
  pid.SetOutputLimits(-255, 255);
  pid.SetSampleTime(25);

  mpu.update();
  lastCorrectAngle = mpu.getAngleZ();

  commands[0] = 0;
  currentCommand = 0;
}

void loop()
{
  // Webszerver kérések kezelése

  handlePidWebClient();

  // Itt jöhet a többi kód, ami nem kapcsolódik a webszerverhezwhile (true)
  while (true)
  {
    /* code */
  }
  
  {
    measureDistanceAllDirections();
    while (distances[DIRECTION_FRONT] > 10 || distances[DIRECTION_FRONT] < 2)
    {
      measureDistanceAllDirections();

      forwardWithAlignment(100);
      mpu.update();
      switch (rfidToDirection())
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
    }
    else
    {
      turnRight(85);
    }
  }

  // while (true)
}