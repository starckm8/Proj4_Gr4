/*
Funktion: DHT 22-, SDS011-Sensoren auslesen, in Json-String speichern, auf WebSocket-Server hochladen.
          Zusätzlich Status mit LED-Streifen anzeigen lassen.

Autoren: Lisa Weidler, Timo Storm, Matthias Starck

Erstelldatum: 22.10.2021
Letzte Änderung: 07.12.2021

Titel: Informatiklabor, Projekt 4 - Gruppe 4


todo: - 


*/



#include <ESP8266WiFi.h>                                      //Import von Bibliothek für das Herstellen der Wlan-Verbindung 
#include <WebSocketsServer.h>                                 //Import von Bibliothek für Serverkommunikation
#include <ArduinoJson.h>                                      //Import von Bibliothek um JSON Dokumente erstellen zu können
#include <ESPDateTime.h>                                      //Import von Bibliothek um aktuelles Datum und Uhrzeit einzubinden
#include <DHT.h>                                              //Import von Bibliothek für den DHT22
#include <SDS011.h>                                           //Import von Bibliothek für SDS011
#include <Adafruit_NeoPixel.h>                                //Import von Bibliothek für den LED Streifen WS2812

#define LED D0                                                //LED auf Microcontroller wird über Ein-/Ausgang D0 gesteuert
#define DHT_TYPE DHT22                                        //genauen Typ des DHT festelegen
#define txd D2                                                //TXD des SDS011 mit Eingang D2 verbunden
#define rxd D3                                                //RXD des SDS011 mit Eingang D3 verbunden
#define dht_pin_2 D1                                          //DHT ist mit dem Eingang D1 verbunden
#define ssid "Xperia St"                                      //Name des Wlans/Hotspots
#define password "29032409"                                   //Passwort für das Wlan/Hotspot
#define Pin D6                                                //Pin für LED Streifen

const int led_timer = 1000;
const int output_timer = 5000;                                //Timer für das Messen der Daten (Messintervall)
const int capacity = JSON_OBJECT_SIZE(13);                    //Größe des JSON Objekts auf 13 festgelegt

int led_status = HIGH;
WebSocketsServer server = WebSocketsServer(80);               //Server über den Kommunikation mit Clienten geschieht wird mit server angesprochen
int client_num;                                               //Variable für Clienten-ID
DHT dht(dht_pin_2, DHT_TYPE);                                 //Der Temperatursensor wird ab jetzt mit „dht“ angesprochen, zugehöriger Pin wird übergeben
SDS011 sds;                                                   //Der Feinstaubsensor wird mit "sds" angesprochen
int numPixels = 8;                                            //Anzahl der LEDs auf LED-Streifen
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numPixels, Pin, NEO_GRB + NEO_KHZ800); //Übergabe des Pins und der Anzahl an LEDs auf dem Streifen
uint32_t red = strip.Color(255, 0, 0);                        //RGB Farbe Rot
uint32_t green = strip.Color(0, 255, 0);                      //RGB Farbe Grün
uint32_t yellow = strip.Color(255, 255, 0);                   //RGB Farbe Gelb
uint32_t out = strip.Color(0,0,0);                            //LED aus
float p10, p25;                                               //Variablen für Feinstaubwerte 
unsigned long op_rt = 0;                                      //Stand der zuletzt gespeicherten Millisekunden
unsigned long led_rt = 0;
long mil;
int c = 0;                                                    //Variable für Index im Array "storage"
int i;                                                        //Zählvariable für LEDs
//bool error = false;
bool over_ten = false;                                        //Wahrheitswert für die Bildung des Durchschnitts auf falsch gesetzt
String json_string;
bool conection_lost = false;
float avg[4];                                                 //Array für die gebildeten Durchschnittswerte deklariert

float limits[][2] = {                                         //Array um kritische Werte zu definieren (temp,lf,pm10,pm2.5)
  {24.0, 0},
  {70.0, 0},
  {25.0, 0},
  {25.0, 0}
};

float storage[][10] = {                                      //Array um Messwerte einzuspeichern
  {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
  {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
  {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
  {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};


void connect_to_network(){                                  //Funktion: ESP soll sich mit dem Wlan/Hotspot verbinden
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){                     //Schleife solange keine Verbindung besteht
    Error();                                                //
    delay(1000);                                            
    Serial.println("Connecting to WiFi...");                //Jede Sekunde ein neuer Verbindungsversuch + nette Ausgabe
  }
  
  Serial.println("Connected to Network");                    
  Serial.println(WiFi.localIP());                           //Bei Erfolgreicher Verbindung wird die IP-Adresse auf seriellem Monitor ausgegeben
}


void average(){                                             //Funktion zum Filtern der Messwerte (Durchschnitt bilden)
  int count = c + 1;
  float sum;
  for(int i=0; i<=3; i++){                                  //Index für Zeilen wird pro Durchgang erhöht
    for(int j=0; j<=9; j++){                                //Index für Spalten wird pro Durchgang erhöht
      sum = sum + storage[i][j];                            //Für jede Zeile werden die Werte der Spalten aufsummiert
    }
    if(over_ten){                                           //Wenn jede Spalte der Zeile einen Wert hatte, dann wird die vorher gebildete Summe 
      avg[i] = sum / 10;                                    //durch 10 geteilt
    }
    else{                                                   //Wenn nicht wird durch die Anzahl der Spalten mit einem Wert geteilt
      avg[i] = sum / count;
    }
    sum = 0;
  }
}


void pack_json(){                                               //Funktion, um Messwerte in ein JSON Objekt zu wandeln
  json_string = "";
  StaticJsonDocument<capacity> doc;                             //JSON Dokument wird angelegt (Objekte und Werte können hierin gespeichert werden)
  doc["timestamp"] = DateTime.toString();                       //Jeder Messreihe wird Datum und Uhrzeit hinzugefügt, in der das JSON Objekt erstellt wird
  JsonObject filtered = doc.createNestedObject("filtered");     //Erstes JSON Objekt für die aktuellen Durchschnittswerte (gefiltert)
    filtered["temperature"] = avg[0];                           //Mittelwert-Temperatur zu JSON Dokument hinzufügen
    filtered["humidity"] = avg[1];                              //Mittelwert-Luftfeuchtigkeit zu JSON Dokument hinzufügen
    filtered["pm25"] = avg[2];                                  //Mittelwert-PM2.5 zu JSON Dokument hinzufügen
    filtered["pm100"] = avg[3];                                 //Mittelwert-PM10 zu JSON Dokument hinzufügen
  JsonObject unfiltered = doc.createNestedObject("unfiltered"); //Zweites JSON Objekt für die aktuellen Messwerte (ungefiltert)
    unfiltered["temperature"] = storage[0][c];                  //Temperatur zu JSON Dokument hinzufügen
    unfiltered["humidity"] = storage[1][c];                     //Luftfeuchtigkeit zu JSON Dokument hinzufügen
    unfiltered["pm25"] = storage[2][c];                         //PM2.5 zu JSON Dokument hinzufügen
    unfiltered["pm100"] = storage[3][c];                        //PM10 zu JSON Dokument hinzufügen

  serializeJson(doc, json_string);                              //Json Dokument in Bytestring umwandeln 
  Serial.println(json_string);   
}



//Funktion um Aktionen auf Websocket zu bearbeiten braucht Klientennummer, Aktionstyp, optional Daten zum Übertragen und deren Länge
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  switch(type){                                                         //Fallunterscheidung
    //Verbindung von Client unterbrochen
    case WStype_DISCONNECTED:                                           // Fall 1: vorhandene Verbindung abgebrochen
      Serial.printf("[%u] Disconnected!\n", num);                       //Gibt auf Monitor aus, welche Client nicht mehr Verbunden ist
      break;

    // Neue Verbindung von Client
    case WStype_CONNECTED:                                              // Fall 2: Neue Verbindung hergestellt                 
    {
      IPAddress ip = server.remoteIP(num);                              //Holt IP-Adresse des verbundenen Clients 
      Serial.printf("[%u] Connection from ", num);                    
      Serial.println(ip.toString());                                    //Gibt IP-Adresse aus
      client_num = num;                                                 //Speichert Nummer des verbundene Clients
    }
    break; 

    //Alle anderen Aktionen ingorieren
    case WStype_TEXT:
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
                           
  }
}
void Okay (){                                                   //LED-Status-Funktion alle Werte sind okay 
  for (i=1;i<=3; i++){                                          //LED 2-4 Farbe auf grün gestellt 
    strip.setPixelColor(i,green);
    
  }
  for (i=4;i<=numPixels;i++){                                   //restliche LEDs (5-8) auf aus gestellt     
    strip.setPixelColor(i,out);
  }
    strip.show();                                               //LEDs wie eingestellt leuchten lassen
}

void Increased(){                                               //LED-Status-Funktion mindestens ein Wert ist erhöht
   for (i=1;i<=3; i++){                                         //LED 2-4 Farbe auf grün gestellt
    strip.setPixelColor(i,green);
      }
      
    strip.setPixelColor(4,yellow);                              //LED 5 und 6 auf gelb gestellt
    strip.setPixelColor(5,yellow);  
   
    strip.setPixelColor(6,out);                                 //LED 7 und 8 auf aus gestellt
    strip.setPixelColor(7,out);
    strip.show();                                               //LEDs wie eingestellt leuchten lassen
}


void Dangerous(){                                               //LED-Status-Funktion mindestens ein Wert ist viel zu hoch
   for (i=1;i<=3; i++){                                         //LED 2-4 Farbe auf grün gestellt
    strip.setPixelColor(i,green);
      }
      
    strip.setPixelColor(4,yellow);                              //LED 5 und 6 auf gelb gestellt              
    strip.setPixelColor(5,yellow);  
   
    strip.setPixelColor(6,red);                                 //LED 7 und 8 auf rot gestellt
    strip.setPixelColor(7,red);
    strip.show();                                               //LEDs wie eingestellt leuchten lassen
}


void Error(){
 for (i=0;i<=numPixels;i++){
  strip.setPixelColor(i,red);
  strip.show();
  delay(200);
 }
 for (i=0;i<=numPixels;i++){
  strip.setPixelColor(i,out);
  strip.show();
  delay(200);
 }
}


void status_LED(){                                             //Funktion für LED Streifen, wann welcher Fall eintritt(okay,increased,dangerous)
  for(int j=0; j<=3; j++){                                     // Prüft ob ein oder mehrere Werte ihren Grenzwert überschreiten
      if(avg[j] > limits[j][0]){
         float dif = avg[j] - limits[j][0];                    //Betrag der Abweichung ermitteln
         limits[j][1] = 1;                                     
         if(dif > 0 && dif <= 3){                              //Fallunterscheidung: bei 0-3: increased, >3: dangerous
          Increased();
         }
         else if(dif > 3){
          Dangerous();
         }
      }
      else{
          limits[j][1] = 0;                                    
      }
    }
    float sum = 0;
    for(int j=0; j<=3; j++){
      sum = sum + limits[j][1];                               //gucken ob keine Grenze überschritten ist
    }
    if(sum == 0){
        Okay();                                               //Wenn keine Grenze überschritten: okay
    }
}



void setup(){
  Serial.begin(115200);                                      //seriellen Monitor starten mit Baudrate 115200Bd
  dht.begin();                                               //DHT22 starten
  sds.begin(txd, rxd);                                       //SDS011 starten und Pins übergeben
  strip.begin();                                             //LED Streifen starten
  
  DateTime.setTimeZone("CET-1");                             //Zeitzone auswählen
  DateTime.begin();                                          //Zeit Modul starten 

  connect_to_network();                                      //Mit Wlan verbinden

  server.begin();                                            //Server starten
  server.onEvent(onWebSocketEvent);                          //

  strip.setBrightness(75);                                   //Helligkeit der LEDs von 0=dunkel bis 255=ganz hell auswählen
  
  
  pinMode(LED, OUTPUT);                                      //Pin der Onboard LED als Ausgang geschaltet (standardmäßig auf Eingang geschaltet) 
}


void loop(){
  mil = millis();                                             //aktuelle Anzahl an Millisekunden, seit dem das Programm gestartet wurde in Variable gespeichert
  if (mil > led_rt + led_timer){
    led_rt = mil;                                             //Realtime aktualisieren
    if (led_status == HIGH){                                  // Zustand des LED-Pins ändern
      led_status = LOW;
      strip.setPixelColor(0, out);
      strip.show();
      
    }
    else{
      led_status = HIGH;
      strip.setPixelColor(0, strip.Color(0,0,255));               //erste LED soll blau sein, um Funktionstüchtigkeit des LED-Streifens anzuzeigen
      strip.show();
    }
  }
  
  if(WiFi.status() == WL_CONNECTED){
    if(mil > op_rt + output_timer){                            //alle 5 Sekunden (entsprechend "output_timer") soll eine Messung der Werte durchgeführt werden 
      op_rt = mil;
      sds.read(&p25, &p10);                                    //Feinstaubwerte auslesen
      storage[0][c] = dht.readTemperature();                   //Temperatur auslesen und zu Filter-Array hinzufügen
      storage[1][c] = dht.readHumidity();                      //Luftfeuchtigkeit auslesen und zu FIlter-Array hinzufügen
      storage[2][c] = p25;                                     //Feinstaubwerte zu Filter-Array hinzufügen
      storage[3][c] = p10;
      
      average();                                               //Durchschnitt der letzten 10 Werte bilden
      pack_json();                                             //Messwerte und Durchschnitte als JSON Dokument speichern
      server.sendTXT(client_num, json_string);                 //JSON Dokument über Server an Client übersenden
      status_LED();                                            //LED Streifen entsprechend der Werte aufleuchten lassen
      
      c++;                                                     //Wenn 10 Werte in Storage-Matrix gespeichert sollen die ersten wieder überschrieben werden
      if(c == 10){                                             //desahlb wird die Zählvariable c nach 10 durchgängen wieder auf 0 gesetzt
        c = 0;
        over_ten = true;
      }    
    }
    server.loop();                                            //Server aktiv halten
  }
  else{
    connect_to_network();
  }                                             
}
