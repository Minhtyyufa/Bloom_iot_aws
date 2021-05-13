#include <Adafruit_SGP30.h>
#include <DHT.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
extern "C" {
#include "libb64/cdecode.h"
}

#define WATERSENSORPIN A0
#define DHTTYPE DHT22
//#define DHTPIN 14

uint8_t DHTPin = 14;
DHT dht(DHTPin, DHTTYPE);

// YOUR SSID AND PASSWORD
const char* ssid = "__SSID__";
const char* password = "__PASSWORD__";

Adafruit_SGP30 sgp;
//const char JSONPAYLOAD[] = "{ \"SystemID\" : \"%s\", \"timestamp\": %lu, \"readingType\" : \"%s\",  \"ReadingValue\": %d}";
const char JSONPAYLOADhum[] = "{ \"SystemID\" : \"%s\", \"timestamp\": %lu, \"readingType\" : \"%s\",  \"ReadingValue\": %2f}";
const char JSONPAYLOADtemp[] = "{ \"SystemID\" : \"%s\", \"timestamp\": %lu, \"readingType\" : \"%s\",  \"ReadingValue\": %2f}";
const char JSONPAYLOADCO2[] = "{ \"SystemID\" : \"%s\", \"timestamp\": %lu, \"readingType\" : \"%s\",  \"ReadingValue\": %2f}";
const char JSONPAYLOADliq[] = "{ \"SystemID\" : \"%s\", \"timestamp\": %lu, \"readingType\" : \"%s\",  \"ReadingValue\": %2f}";

String sgp30_ID;
String esp8266_ID;
String t;
String h;
String c;
String l;
//int status = WL_IDLE_STATUS;
int tick=0;
char payload[512];
char rcvdPayload[512];
int SPGcounter = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);


// Find this awsEndpoint in the AWS Console: Manage - Things, choose your thing
// choose Interact, its the HTTPS Rest endpoint 
// YOUR ENDPOINT HERE
const char* awsEndpoint = "__AWS_IOT_ENDPOINT__";

// For the two certificate strings below paste in the text of your AWS 
// device certificate and private key, comment out the BEGIN and END 
// lines, add a quote character at the start of each line and a quote 
// and backslash at the end of each line:

//PUT YOUR CERTICATE PEM HERE
// xxxxxxxxxx-certificate.pem.crt
const String certificatePemCrt = \
//-----BEGIN CERTIFICATE-----



//-----END CERTIFICATE-----

//PUT YOUR PRIVATE KEY PEM HERE
// xxxxxxxxxx-private.pem.key
const String privatePemKey = \
//-----BEGIN RSA PRIVATE KEY-----


//-----END RSA PRIVATE KEY-----

// This is the AWS IoT CA Certificate from: 
// https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication
// This one in here is the 'RSA 2048 bit key: Amazon Root CA 1' which is valid 
// until January 16, 2038 so unless it gets revoked you can leave this as is:
const String caPemCrt = \
//-----BEGIN CERTIFICATE-----
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy" \
"rqXRfboQnoZsG4q5WTP468SQvvG5";
//-----END CERTIFICATE-----

WiFiClientSecure wiFiClient;
void msgReceived(char* topic, byte* payload, unsigned int len);
PubSubClient pubSubClient(awsEndpoint, 8883, msgReceived, wiFiClient); 

void setup() {
  Serial.begin(115200); Serial.println();

  Serial.println("ESP8266 AWS IoT Example");

  Serial.print("Connecting to "); Serial.print(ssid);
  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  Serial.print(", WiFi connected, IP address: "); Serial.println(WiFi.localIP());

  // get current time, otherwise certificates are flagged as expired
  setCurrentTime();

  uint8_t binaryCert[certificatePemCrt.length() * 3 / 4];
  int len = b64decode(certificatePemCrt, binaryCert);
  wiFiClient.setCertificate(binaryCert, len);
  
  uint8_t binaryPrivate[privatePemKey.length() * 3 / 4];
  len = b64decode(privatePemKey, binaryPrivate);
  wiFiClient.setPrivateKey(binaryPrivate, len);

  uint8_t binaryCA[caPemCrt.length() * 3 / 4];
  len = b64decode(caPemCrt, binaryCA);
  wiFiClient.setCACert(binaryCA, len);

  if (! sgp.begin()){
    Serial.println("Sensor not found :(");
    while (1);
  }
  sgp30_ID = String(sgp.serialnumber[0], HEX) + String(sgp.serialnumber[1], HEX) + String(sgp.serialnumber[2], HEX);
  sgp30_ID.toUpperCase();
  Serial.println("Found SGP30 serial 0x"+ sgp30_ID);
  esp8266_ID = String(ESP.getChipId());
  t = String("Air-temp Level");
  h = String("Air-humidity Level");
  c = String("CO2 Level");
  l = String("Liquid Level");
// If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!
  pinMode(DHTPin, INPUT);
  dht.begin();
  if (! sgp.begin()){
    Serial.println("Gas sensor not found :(");
    while (1);
  }


}

unsigned long lastPublish;
int msgCount;

void publish_toAWS(String id, float hum, float temp,float eCO2,String h,String t,String c,String l ) {
    pubSubCheckConnect();
    sprintf(payload,JSONPAYLOADhum,id.c_str(),currenttime().c_str(),h.c_str(),hum);
    Serial.println(payload);  
    pubSubClient.publish("ESP8266_TEST", payload);  
    
    sprintf(payload,JSONPAYLOADtemp,id.c_str(),currenttime().c_str(),t.c_str(),temp);
    Serial.println(payload);  
    pubSubClient.publish("ESP8266_TEST", payload); 
       
    sprintf(payload,JSONPAYLOADCO2,id.c_str(),currenttime().c_str(),c.c_str(),eCO2);
    Serial.println(payload);  
    pubSubClient.publish("ESP8266_TEST", payload);    
    
    sprintf(payload,JSONPAYLOADliq,id.c_str(),currenttime().c_str(),l.c_str(),analogRead(WATERSENSORPIN)/10.24);
    Serial.println(payload);  
    pubSubClient.publish("ESP8266_TEST", payload);      
        Serial.println(hum); 
        Serial.println(temp);  
        Serial.println(eCO2);  
        Serial.println(analogRead(WATERSENSORPIN)/10.24);  

}

void setAbsoluteHumidity() 
{
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    float temperature = dht.readTemperature(false);
    float humidity = dht.readHumidity();
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
    sgp.setHumidity(absoluteHumidityScaled);
    return;
}

void recalibrateSPG()
{
  uint16_t TVOC_base, eCO2_base;
    if (! sgp.getIAQBaseline(&eCO2_base, &TVOC_base)) {
      Serial.println("Failed to get baseline readings");
      return;
    }
    Serial.print("****Baseline values: eCO2: 0x"); Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x"); Serial.println(TVOC_base, HEX);
}

float getCO2()
{
  setAbsoluteHumidity();
  
    if (! sgp.IAQmeasure()) {
    Serial.println("C02 Measurement failed");
    return 0.0;
  }

  float CO2 = (float)sgp.eCO2;
  
  SPGcounter++;
    if (SPGcounter == 30) {
      SPGcounter = 0;
      recalibrateSPG();
    } 

  return CO2;
}
float getTemperature() {
  float temperature;
  float newTemperature = dht.readTemperature(true);
  if(!isnan(newTemperature)) temperature = newTemperature;
  return temperature;
}

float getHumidity() {
  float humidity;
  float newHumidity = dht.readHumidity();
  if(!isnan(newHumidity)) humidity = newHumidity;
  return humidity;
}
void loop() {

  if (tick >= 3)  {
    tick = 0;
    if (! sgp.IAQmeasure()) {
      Serial.println("Measurement failed");
      return;
    }

    if (! sgp.IAQmeasureRaw()) {
      Serial.println("Raw Measurement failed");
      return;
    }
    
  pubSubCheckConnect();


  if (millis() - lastPublish > 10000) {

    publish_toAWS(esp8266_ID,getHumidity(),getTemperature(),getCO2(),h,t,c,l); 

    delay(15000);
    lastPublish = millis();
  }


}
tick++;
  
}

void msgReceived(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on "); Serial.print(topic); Serial.print(": ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void pubSubCheckConnect() {
  if ( ! pubSubClient.connected()) {
    Serial.print("PubSubClient connecting to: "); Serial.print(awsEndpoint);
    while ( ! pubSubClient.connected()) {
      Serial.print(".");
      pubSubClient.connect("ESPthing");
    }
    Serial.println(" connected");
    pubSubClient.subscribe("inTopic");
  }
  pubSubClient.loop();
}

int b64decode(String b64Text, uint8_t* output) {
  base64_decodestate s;
  base64_init_decodestate(&s);
  int cnt = base64_decode_block(b64Text.c_str(), b64Text.length(), (char*)output, &s);
  return cnt;
}
String currenttime(){
  char *asctime(const struct tm *time);
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: "); Serial.print(asctime(&timeinfo));
  return asctime(&timeinfo);
}


void setCurrentTime() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: "); Serial.print(asctime(&timeinfo));
}
