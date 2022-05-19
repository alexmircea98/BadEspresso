/*
  Keyboard logout

  This sketch demonstrates the Keyboard library.

  When you connect pin 2 to ground, it performs a logout.
  It uses keyboard combinations to do this, as follows:

  On Windows, CTRL-ALT-DEL followed by ALT-l
  On Ubuntu, CTRL-ALT-DEL, and ENTER
  On OSX, CMD-SHIFT-q

  To wake: Spacebar.

  Circuit:
  - Arduino Leonardo or Micro
  - wire to connect D2 to ground

  created 6 Mar 2012
  modified 27 Mar 2012
  by Tom Igoe

  This example is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/KeyboardLogout
*/

#include "USB.h"
#include "USBHIDKeyboard.h"
USBHIDKeyboard Keyboard;

#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;

#define OSX 0
#define WINDOWS 1
#define UBUNTU 2

//how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 1

WiFiServer server(1337);
WiFiClient serverClients[MAX_SRV_CLIENTS];

int platform = 1; //DEFAULT WINDOWS


void lock(int platform){
  switch (platform) {
    case OSX:
      Keyboard.press(KEY_LEFT_GUI);
      // Shift-Q logs out:
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.press('Q');
      delay(100);
      Keyboard.releaseAll();
      // enter:
      Keyboard.write(KEY_RETURN);
      break;
    case WINDOWS:
      // CTRL-ALT-DEL:
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_DELETE);
      delay(100);
      Keyboard.releaseAll();
      // ALT + k:
      delay(2000);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.releaseAll();
      Keyboard.press('k');
      Keyboard.releaseAll();
      break;
    case UBUNTU:
      // CTRL-ALT-DEL:
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_DELETE);
      delay(1000);
      Keyboard.releaseAll();
      // Enter to confirm logout:
      Keyboard.write(KEY_RETURN);
      break;
  }
}

void terminal(){
  switch (platform) {
    case OSX:
      break;
    case WINDOWS:
      // WIN+R(run):
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('r');
      delay(100);
      Keyboard.releaseAll();
      delay(500);
      Keyboard.print("cmd");
      delay(100);
      Keyboard.write(KEY_RETURN);
      break;
    case UBUNTU:
      // CTRL-ALT-DEL:
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_DELETE);
      delay(1000);
      Keyboard.releaseAll();
      // Enter to confirm logout:
      Keyboard.write(KEY_RETURN);
      break;
  }
}

void connect_to_wifi(){
//  const char* ssid = ;
//  const char* password = ;

  wifiMulti.addAP("BTAIM", "pnxJrrfbtm6c");
//  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
//  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }
}

void setup() {
  
  Keyboard.begin();
  USB.begin();

  Serial.begin(115200);
  Serial.println("\nConnecting");

  connect_to_wifi();

  //start UART and the server
  server.begin();
  server.setNoDelay(true);

//  Serial.print("Ready! Use 'nc ");
//  Serial.print(WiFi.localIP());
//  Serial.println(" 1337' to connect");
}

void loop() {

 uint8_t i;
 String recv;
 char buf[30];
  if (wifiMulti.run() == WL_CONNECTED) {
    //check if there are any new clients
    if (server.hasClient()){
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        //find free/disconnected spot
        if (!serverClients[i] || !serverClients[i].connected()){
          if(serverClients[i]) serverClients[i].stop();
          serverClients[i] = server.available();
          if (!serverClients[i]) Serial.println("available broken");
          serverClients[i].print("New client: ");
          serverClients[i].print(i); serverClients[i].print(' ');
          serverClients[i].println(serverClients[i].remoteIP());
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        //no free/disconnected spot so reject
        server.available().stop();
      }
    }
    //check clients for data
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        if(serverClients[i].available()){
          //get data from the telnet client and push it to the UART
          while(serverClients[i].available()){
            recv = serverClients[i].readStringUntil('\r');
            
            if (recv == "exit") {
              serverClients[i].stop();
            }
            if (recv == "SYN") {
              serverClients[i].println("SYN/ACK");
            }
            if (recv == "terminal") {
              terminal();
              serverClients[i].println("opened CMD");
            }
            recv.toCharArray(buf,recv.length());
            //Keyboard.print(recv); //bug aicea
            Serial.print(recv);
          }
        }
      }
    }
  }
  else {
    Serial.println("WiFi not connected!");
    for(i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i]) serverClients[i].stop();
    }
    delay(1000);
  }
}
