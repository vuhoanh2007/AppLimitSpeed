#include <TinyGPS++.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <SoftwareSerial.h>
#include <WebServer.h>

TinyGPSPlus gps;
SoftwareSerial gpsSerial(16,17); // RX, TX
WebServer server(80);

float limitspeed;
float curSpeed;
bool CON = false;
String logOver = "";

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define WIFI_SSID "hoanhh"
#define WIFI_PASS "hoanhpro123"
#define AP_WIFI_SSID "Limit_Speed_VIP"
#define AP_WIFI_PASS "hoanhpro123"
#define API_KEY "AIzaSyCZzDcsuKPxjkpzJEcw38QdOKIkWHip0oI"
#define DB_URL "https://iot-devices-c0515-default-rtdb.firebaseio.com"
#define USER_EMAIL "vuthaohoanh2007@gmail.com"
#define USER_PASS "hoanhpro123"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Dang ket noi toi WIFI");
  for (int i = 0; i<20; i++){
    if(WiFi.status() == WL_CONNECTED){
        CON = true;
        break;
    }
    Serial.print(".");
    delay(1000);
  }

  if(CON){
    Serial.println("Da ket noi!!!");
    config.api_key = API_KEY;

    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASS;
    config.database_url = DB_URL;
    config.token_status_callback = tokenStatusCallback;
    Firebase.reconnectWiFi(true);
    fbdo.setBSSLBufferSize(4096,1024);
    fbdo.setResponseSize(2048);
    Firebase.begin(&config, &auth);
    Firebase.setDoubleDigits(5);
    config.timeout.serverResponse = 10 * 1000;
  }else{
    Serial.println("Khong the ket noi, bat dau diem truy cap...");

    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("IP: ");
    Serial.println(IP);
  }

  server.on("/", handleRoot);
  server.on("/setLimit", handleSetLimit);
  server.begin();
  Serial.println("Da bat dau webserver");
}

void loop() {
  server.handleClient();

  while (gpsSerial.available() > 0){
    if(gps.encode(gpsSerial.read())){
      curSpeed = gps.speed.kmph();
      Serial.print("Toc do: ");
      Serial.println(curSpeed);
      if(CON){
        if (Firebase.ready() && (millis() - sendDataPrevMillis > 10000 || sendDataPrevMillis == 0)){
          sendDataPrevMillis = millis();

          Firebase.RTDB.setFloat(&fbdo, F("/currentSpeed"), curSpeed);
        }
      }

      if(curSpeed > limitspeed){
        String logE = "Da vuot qua toc do tai: ";
        logE += gps.time.value();
        logE += " Toc do: ";
        logE += curSpeed;
        logOver += logE + "\n";
        Serial.println(logE);
        Firebase.RTDB.setString(&fbdo, F("/overspeed"), logOver);
      }

      if(CON){
        if(Firebase.RTDB.getInt(&fbdo, F("/speedLimit"))){
          limitspeed = fbdo.intData();
          Serial.print("Da cap nhat gioi han toc do thanh: ");
          Serial.println(limitspeed);
        }
      }
    }
  }

}

void handleRoot() {
  String html = "<h1>ESP32 Speed Limit</h1>";
  html += "<p>Gioi han toc do hien tai: " + String(limitspeed) + " km/h</p>";
  html += "<form action='/setLimit' method='GET'>";
  html += "<input type='number' name='limit' placeholder='Dat gioi han'>";
  html += "<button type='submit'>Dat</button></form>";
  html += "<p>Log:</p><pre>" + logOver + "</pre>";
  server.send(200, "text/html", html);
}

void handleSetLimit() {
  if (server.hasArg("limit")) {
    limitspeed = server.arg("limit").toFloat();
    server.send(200, "text/html", "<p>Toc do duoc dat thanh " + String(limitspeed) + " km/h</p><a href='/'>Quay ve</a>");
  } else {
    server.send(400, "text/html", "<p>Khong hop le</p><a href='/'>Quay ve</a>");
  }
}

