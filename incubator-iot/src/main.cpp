#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>

#define HEATER D5

const char *ssid = "kuota"; //silakan disesuaikan sendiri
const char *password = "mihungoyeng12";//silakan disesuaikan sendiri

const char *mqtt_server = "ec2-54-89-37-231.compute-1.amazonaws.com";

WiFiClient espClient;
PubSubClient client(espClient);

SimpleDHT11 dht11(D6);

long now = millis();
long lastMeasure = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

void heaterON()
{
  digitalWrite(HEATER, HIGH);
}

void heaterOFF()
{
  digitalWrite(HEATER, LOW);
}

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "inc/heater")
  {
    Serial.print("Changing Incubator heater to ");
    if (messageTemp == "on")
    {
      heaterON();
      Serial.print("On");
    }
    else if (messageTemp == "off")
    {
      heaterOFF();
      Serial.print("Off");
    }
  }
  Serial.println();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP8266Client", "jti", "1234"))
    {
      Serial.println("connected");
      client.subscribe("inc/heater");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(HEATER, OUTPUT);
  Serial.println("Mqtt Node-RED Smart Incubator");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  if (!client.loop())
  {
    client.connect("ESP8266Client");
  }
  now = millis();
  if (now - lastMeasure > 3000)
  {
    lastMeasure = now;
    int err = SimpleDHTErrSuccess;

    byte temperature = 0;
    byte humidity = 0;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Pembacaan DHT11 gagal, err=");
      Serial.println(err);
      delay(1000);
      return;
    }
    static char temperatureTemp[7];
    static char humidityTemp[7];
    dtostrf(temperature, 4, 2, temperatureTemp);
    dtostrf(humidity, 4, 2, humidityTemp);
    Serial.print("Temperature = ");
    Serial.println(temperatureTemp);
    Serial.print("Humidity = ");
    Serial.println(humidityTemp);
    if (temperature <= 30)
    {
      heaterON();
    }
    else if (temperature > 40)
    {
      heaterOFF();
    }
    client.publish("inc/temp", temperatureTemp);
    client.publish("inc/humid", humidityTemp);
  }
}