#ifndef PID_WEBINTERFACE_H
#define PID_WEBINTERFACE_H

#include <WiFi.h>
#include <WebServer.h>

// Külső változók deklarálása
extern double Pid_P;
extern double Pid_I;
extern double Pid_D;
extern double distanceFromSingleWall;
extern double distances[3];
extern double forwardMaxSpeed;
extern double distanceFromFrontWall;
extern double delayBeforeTurn;
extern double turnMinSpeed;
extern double turnMaxSpeed;
extern bool webButtonPressed;
extern WebServer server;
extern void beep(int number);

// Webszerver kezelő függvények deklarációi
void setupPidWebInterface(const char* ssid, const char* password);
void handleRoot();
void handleUpdate();
void handlePidWebClient();
void handleButton();
void handleDistances();

// Sikeres frissítés jelzése
extern bool pidValuesUpdated;

#endif
