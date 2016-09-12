#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

#define DHTTYPE DHT22
#define DHTPIN  D1
#define LED D0
 
const char* ssid = "";
const char* password = "";
const char* ts_server = "api.thingspeak.com";
String apiKey = "APIKEY";
 
ESP8266WebServer server(80);
 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

WiFiClient client;
 
float humidity, temp_c;  // Values read from sensor
String webString="";     // String to display
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000, pushInterval=20000;
 
void handle_root() {
  server.send(200, "text/plain", "Hello from nodemcu, read from /temp or /humidity");
  delay(100);
}

int getTemperature() {
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_c = dht.readTemperature();     // Read temperature as Centigrade
    
    if(temp_c > 1000 || humidity > 100){
      Serial.println("ERRORL: Sensor data out of bounds!");
      return -1;
    }
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_c)) {
      Serial.println("ERROR: Failed to read from DHT sensor!");
      return -1;
    }
    return 1;
  }
}

void handle_temp() {
  getTemperature();
  webString="Temperature: "+String((int)temp_c)+" C";
  server.send(200, "text/plain", webString);
}

void handle_humidity() {
  getTemperature();
  webString="Humidity: "+String((int)humidity)+"%";
  server.send(200, "text/plain", webString);
}

void setup(void)
{
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
  dht.begin();           // initialize temperature sensor

  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(LED, LOW);
  Serial.println("");
  Serial.println("NodeMCU temperature monitor");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
   
  server.on("/", handle_root);
  
  server.on("/temp", handle_temp);
 
  server.on("/humidity", handle_humidity);
  
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void)
{
  server.handleClient();
  if(getTemperature() > 0){
    if (client.connect(ts_server,80)) { 
      getTemperature();
      String postStr = apiKey;
      postStr +="&field1=";
      postStr += String((int)temp_c);
      postStr +="&field2=";
      postStr += String((int)humidity);
      postStr += "\r\n\r\n";
      
      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(postStr.length());
      client.print("\n\n");
      client.print(postStr);
      
      Serial.print("Temperature: ");
      Serial.print(temp_c);
      Serial.print(" degrees Celcius Humidity: ");
      Serial.print(humidity);
      Serial.println("% send to Thingspeak");
    }
    client.stop();    
  }
  Serial.println("Waiting");
  delay(pushInterval);
} 
 

