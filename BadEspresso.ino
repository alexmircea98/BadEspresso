/*
  Keyboard logout

  This sketch demonstrates the BadEspresso functionality.
  It's a combination of a few examples from arduino ide such as KeyboardLogout and Wifi 
  telnet and a few other sources from the internet, all under free licences.

  This is compatible with esp32-S2/S3 or greater.
  
  https://www.arduino.cc/en/Tutorial/BuiltInExamples/KeyboardLogout

  author: Alexandru Mircea 
*/

#include "USB.h"
#include "USBHIDKeyboard.h"


#include <WiFi.h>
#include <WiFiMulti.h>

#include "FS.h"
#include "SPIFFS.h"

#define OSX 0
#define WINDOWS 1
#define UBUNTU 2

//how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 1

#define FORMAT_SPIFFS_IF_FAILED true

USBHIDKeyboard Keyboard;
WiFiMulti wifiMulti;


WiFiServer server(1337);
WiFiClient serverClients[MAX_SRV_CLIENTS];

int platform = 1; //DEFAULT WINDOWS


void lock(){
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
      // CTRL-ALT-DEL:  //DEPRICATED, but usefull
//      Keyboard.press(KEY_LEFT_CTRL);
//      Keyboard.press(KEY_LEFT_ALT);
//      Keyboard.press(KEY_DELETE);
//      delay(100);
//      Keyboard.releaseAll();
//      // ALT + k:
//      delay(2000);
//      Keyboard.press(KEY_LEFT_ALT);
//      delay(100);
//      Keyboard.release(KEY_LEFT_ALT);
//      delay(2000);
//      Keyboard.press('k');
//      delay(100);
//      Keyboard.releaseAll();  
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press('l');
      delay(100);
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

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected to " + WiFi.SSID()+"\n");
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

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char * path){


    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void getFile(fs::FS &fs, const char * path){
    File file = fs.open(path);
    String line = String();
    String ssid;
    String pass;
    while(file.available()) {
      line = file.readStringUntil('\r');
      ssid = getValue(line, ' ', 0);
      pass = getValue(line, ' ', 1);
      wifiMulti.addAP(ssid.c_str(), pass.c_str());
    }
    file.close();

//    return res;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}


String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void parse_ssids(String file){
    String line = getValue(file, '\r', 0);
  String ssid;
  String pass;
  Serial.println(line);
  //while(line.length() > 0){
    ssid = getValue(line, ' ', 0);
    pass = getValue(line, ' ', 1);
    Serial.println(ssid);
    Serial.println(pass);
    wifiMulti.addAP(ssid.c_str(), pass.c_str());
    file.remove(0, file.indexOf('\n') + 1);
    line = getValue(file, '\r', 0);
    Serial.println(line.length());
  //} 
}

void setup() {
  
  Keyboard.begin();
  USB.begin();

  Serial.begin(115200);
  
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
  }
  Serial.println("The device started!!");
  //SPIFFS.format();
  if(!SPIFFS.exists("/list.txt")){
    Serial.println( "list.txt does not exist. Creating!" );
    //GEN FILE WITH DEFAULT SSID
    writeFile(SPIFFS, "/list.txt", "<SSID> <password>\r\n");  //CHANGE HERE!!!!!!!!!!!!!!!!!!
  } else {
    Serial.println( "list.txt already exist. Reading!!" );
  }
  
  readFile(SPIFFS, "/list.txt");
  getFile(SPIFFS, "/list.txt");

  
  Serial.println("\nConnecting");
  connect_to_wifi();

  //start UART and the server
  server.begin();
  server.setNoDelay(true);

}

void loop() {

 uint8_t i;
 String recv;
 //char buf[30];
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
            String arg1 = getValue(recv, ' ', 0);
            if (arg1 == "exit") {
              serverClients[i].stop();
            }
            if (arg1 == "addSSID") {
              String arg2 = getValue(recv, ' ', 1);
              String arg3 = getValue(recv, ' ', 2);
              String line = arg2+" "+arg3+"\r\n";
              appendFile(SPIFFS, "/list.txt", line.c_str());
            }
            if (arg1 == "SYN") {
              serverClients[i].println("SYN/ACK");
            }
            if (arg1 == "send") {
              recv.remove(0,5);
              serverClients[i].println("Sent: "+recv);
              Keyboard.print(recv);
            }
            if (arg1 == "platform") {
              String arg2 = getValue(recv, ' ', 1);
              if (arg2 == "set") {
                String arg3 = getValue(recv, ' ', 2);
                if(arg3 != NULL){
                platform = arg3.toInt();  
                }
              }
              serverClients[i].println("Platform is:");
              serverClients[i].println(platform);
            }
            if (arg1 == "terminal") {
              terminal();
              serverClients[i].println("opened CMD");
            }
            if (arg1 == "lock") {
              lock();
              serverClients[i].println("locked");
            }
            if (arg1 == "help") {
              serverClients[i].println("exit - closes the connection");
              serverClients[i].println("help - lists this list");
              serverClients[i].println("addSSID <SSID> <Password> - add a new set of credential for an AP");
              serverClients[i].println("SYN - responds with ACK, for debugging purposes");
              serverClients[i].println("send <message> - sends the message content as keyboard input");
              serverClients[i].println("platform [set x] - gives the index of the selected platform; If set option is selected it switches to the set platform ( 0 - OSX, 1 (default) - Windows, 2 - Linux)");
              serverClients[i].println("terminal - opens a terminal for the selected platform");
              serverClients[i].println("lock - locks the station for the selected platform");

            }
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
