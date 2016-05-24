// Sääasemaware versio 0.5b (25.5.2016)
//
// ESP8266-mikrokontrollerille tarkoitettu ohjelma, joka laskee keskiarvoja dht22-sensorilta 
// saatavista mittatuloksista ja lähettää ne Thingspeakkiin.
//
// Koodi käyttää adafruitin DHT-kirjastoa, sekä MathWorksin Thingspeak-kirjastoa. 
// Molemmat löytyvät arduino IDE:n "Manage libraries"-velhon alta.
//
// Kaikki testaus on suoritettu LoLinin valmistamalla NodeMCU v3 -piirillä.
// Kytkentäkaaviot & tarkempi blogiteksti ja kokoonpano-ohjeet löytyvät kötöstelyblogistani 
// osoitteesta http://kotostelyt.blogspot.fi
//
// Koodi: Antti Kallunki

// Kirjastot
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>

// Konffit, Wlan & Thingspeak-kanava
const char* kirjoitusavain = "Thingspeak-kanavasi kirjoitusavain";
unsigned long kanavanumero = 123456;  // Thingspeak-kanavasi kanavanumero    
const char* ssid = "Wlanisi SSID";
const char* password = "Wlanin salasana";
const int lampokentta = 1;            // Thingspeak-kanavasi kenttä lämpötiloille
const int kosteuskentta = 2;          // Thingspeak-kanavasi kenttä ilmankosteudelle

//toimintojen intervallit (millisekunteina)
int mittausintervalli = 2000;
int pilvi_intervalli = 20000;

// Sensorin pinnin ja tyypin alustus
#define DHTPIN 2
#define DHTTYPE DHT22   // sensorityyppi (adafruitin DHT-kirjastossa bugi DHT11-sensorin kanssa)

// Määritetään DHT-sensori "dht"
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

void setup() {
 Serial.begin(115200);
 delay(15);
 
 WiFi.begin(ssid, password); //wlaniyhteys
 Serial.print("\nYhdistetaan verkkoon: ");
 Serial.println(ssid);
 while (WiFi.status() != WL_CONNECTED) 
 {
  delay(500);
  Serial.print(".");
 }
 Serial.println("Pliplong, yhdistetty!");
 Serial.println("IP-osoite: ");
 Serial.println(WiFi.localIP());
 
 ThingSpeak.begin(client);
 dht.begin();
}

void loop() {

 // muuttujien alustus

 int lampo_onnistuneet = 0.0;
 int kosteus_onnistuneet = 0.0;
 float sum_lampo = 0.0;
 float sum_kosteus = 0.0;
 float h_sensorilta = 0;
 float t_sensorilta = 0;
 float lampo_keskiarvo = 0;
 float kosteus_keskiarvo = 0;
 unsigned long mittausajastin = 0;
 unsigned long pilviajastin = 0;

 // itse silmukka
 while(1)
 { 

  yield();                               // Jos ei anna yieldiä, niin heittää wdt resettiä ~10 sekunnin välein 
 
  if (millis() - mittausajastin >= mittausintervalli) 
  {
   h_sensorilta = dht.readHumidity();     // Luetaan kosteus
   t_sensorilta = dht.readTemperature();  // Luetaan lämpötila
    
   if (!isnan(t_sensorilta) && !isnan(h_sensorilta) ) // Ehto toteutuu, mikäli arvojen lukeminen onnistuu
    {
     sum_lampo = sum_lampo + t_sensorilta;
     lampo_onnistuneet++;

     sum_kosteus = sum_kosteus + h_sensorilta;
     kosteus_onnistuneet++;
    } 
    else                                  // Debuggauskäyttöön virhe sarjaporttiin
    {
     Serial.println("\nHupsistakeikkaa t. DHT22");   
    } 
   mittausajastin = millis();             // Ajastimen nollaus
  }
 
  // Datan lähetys pilveen & sarjaporttiin
  if (millis() - pilviajastin >= pilvi_intervalli) 
  {
   lampo_keskiarvo = sum_lampo / lampo_onnistuneet;       // Lasketaan summista keskiarvot
   kosteus_keskiarvo = sum_kosteus / kosteus_onnistuneet;

   ThingSpeak.setField(lampokentta, lampo_keskiarvo);     // Asetetaan lasketut keskiarvot haluttuihin kenttiin
   ThingSpeak.setField(kosteuskentta, kosteus_keskiarvo);

   ThingSpeak.writeFields(kanavanumero, kirjoitusavain);  // Lähetetään tiedot pilveen

   Serial.print("\nIlmankosteus: ");                      // Raportoidaan keskiarvot sarjaporttiin
   Serial.print(kosteus_keskiarvo);
   Serial.print(" %\t");
   Serial.print("Lampotila: "); 
   Serial.print(lampo_keskiarvo);
   Serial.println(" *C ");    

   pilviajastin = millis();             // Ajastimen & muuttujien nollaus
   sum_lampo = 0;
   lampo_onnistuneet = 0;
   sum_kosteus = 0;
   kosteus_onnistuneet = 0;
  }
 }
}

