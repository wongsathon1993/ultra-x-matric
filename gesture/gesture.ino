#include <WiFi.h>
#include <ArduinoJson.h>
#include <binary.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#define WIFI_STA_NAME "MoMieNote8"
#define WIFI_STA_PASS "MoMie5027vivek"

#define MQTT_SERVER "m16.cloudmqtt.com"
#define MQTT_PORT 16319
#define MQTT_USER_ID "GESTURE01"
#define MQTT_USERNAME "ggaomyqh"
#define MQTT_PASSWORD "3wjA27NFU3ET"

const int uOneTrigPin = 25;
const int uOneEchoPin = 26;

const int uTwoTrigPin = 17;
const int uTwoEchoPin = 16;

int soundPotValue;

Max72xxPanel matrix = Max72xxPanel(5, 1, 1);

char output[1024];

WiFiClient client;
PubSubClient mqtt(client);

void noFace()
{
  matrix.drawPixel(0, 0, 1);
  matrix.write();
}

void happyRightFace()
{
  matrix.drawPixel(3, 2, 1);
  matrix.drawPixel(5, 2, 1); //eye
  matrix.drawPixel(2, 3, 1);
  matrix.drawPixel(2, 4, 1);
  matrix.drawPixel(3, 5, 1);
  matrix.drawPixel(5, 5, 1); //eye
  matrix.write();
}

void happyLeftFace()
{
  matrix.drawPixel(2, 2, 1);
  matrix.drawPixel(5, 2, 1); //eye
  matrix.drawPixel(4, 3, 1);
  matrix.drawPixel(4, 4, 1);
  matrix.drawPixel(2, 5, 1);
  matrix.drawPixel(5, 5, 1); //eye
  matrix.write();
}

void sadRightFace()
{
  matrix.drawPixel(2, 2, 1);
  matrix.drawPixel(5, 2, 1); //eye
  matrix.drawPixel(3, 3, 1);
  matrix.drawPixel(3, 4, 1);
  matrix.drawPixel(2, 5, 1);
  matrix.drawPixel(5, 5, 1); //eye
  matrix.write();
}

void sadLeftFace()
{
  matrix.drawPixel(0, 4, 1);
  matrix.write();
}

void sadFiveFace()
{
  matrix.drawPixel(4, 4, 1);
  matrix.write();
}

void emptyFace()
{
  matrix.drawPixel(7, 7, 1);
  matrix.write();
}

int currentIndex = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_STA_NAME);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  matrix.setIntensity(7);
  matrix.fillScreen(1);
  matrix.write();
  delay(500);
  matrix.fillScreen(0);
  matrix.write();
  pinMode(uOneTrigPin, OUTPUT);
  pinMode(uOneEchoPin, INPUT);
  delay(500);
  pinMode(uTwoTrigPin, OUTPUT);
  pinMode(uTwoEchoPin, INPUT);
}

void loop()
{
  if (mqtt.connected() == false)
  {
    Serial.print("MQTT connection... ");
    if (mqtt.connect(MQTT_USER_ID, MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("connected");
      sendSensorData();
    }
    else
    {
      Serial.println("failed");
      delay(5000);
    }
  }
  else
  {
    mqtt.loop();
    soundPotValue = analogRead(36);

    Serial.println(soundPotValue);

    if (((soundPotValue < 2047) && (soundPotValue > 0)) || (soundPotValue == 4095))
    {
      long duration1, distance1;
      digitalWrite(uOneTrigPin, LOW);
      delay(50);
      digitalWrite(uOneTrigPin, HIGH);
      delay(50);
      digitalWrite(uOneTrigPin, LOW);
      duration1 = pulseIn(uOneEchoPin, HIGH);
      distance1 = (duration1 / 2) / 29.1;

      Serial.print("Sensor One: ");
      Serial.println(distance1);

      delay(100);

      if ((distance1 < 20) && (distance1 > 0))
      {
        if ((currentIndex >= 0) && (currentIndex != 5))
        {
          currentIndex += 1;
        }
        delay(100);
        displayFace(currentIndex);
      }

      delay(100);

      long duration2, distance2;
      digitalWrite(uTwoTrigPin, LOW);
      delay(50);
      digitalWrite(uTwoTrigPin, HIGH);
      delay(50);
      digitalWrite(uTwoTrigPin, LOW);
      duration2 = pulseIn(uTwoEchoPin, HIGH);
      distance2 = (duration2 / 2) / 29.1;

      Serial.print("Sensor Two: ");
      Serial.println(distance2);

      delay(100);

      if ((distance2 < 20) && (distance2 > 0))
      {
        if ((currentIndex <= 5) && (currentIndex != 0))
        {
          currentIndex -= 1;
        }
        delay(100);
        displayFace(currentIndex);
      }

      delay(100);
    }
    else
    {
      delay(100);
      sendDataToServer(currentIndex);
    }
    delay(100);
  }
}

void displayFace(int index)
{
  matrix.fillScreen(0);
  matrix.write();
  Serial.println(index);
  switch (index)
  {
  case 0:
    noFace();
    break;
  case 1:
    happyRightFace();
    break;
  case 2:
    happyLeftFace();
    break;
  case 3:
    sadRightFace();
    break;
  case 4:
    sadLeftFace();
    break;
  case 5:
    sadFiveFace();
    break;
  default:
    emptyFace();
    break;
  }
}

void sendSensorData()
{
  StaticJsonDocument<1024> doc;

  doc["sensor"] = "GESTURE";
  doc["id"] = 140137;
  doc["config"] = 64;
  doc["active"] = true;

  serializeJson(doc, output);

  mqtt.publish("/sensors", output);
}

void sendDataToServer(int index)

{
  long prob = 0;
  StaticJsonDocument<1024> doc;

  doc["type"] = "GESTURE";
  doc["face"] = index;

  switch (index)
  {
  case 0:
    prob = 0.04;
    doc["value"] = prob;
    break;
  case 1:
    prob = 2.80;
    doc["value"] = prob;
    break;
  case 2:
    prob = 5.56;
    doc["value"] = prob;
    break;
  case 3:
    prob = 8.32;
    doc["value"] = prob;
    break;
  case 4:
    prob = 11.08;
    doc["value"] = prob;
    break;
  case 5:
    prob = 13.84;
    doc["value"] = prob;
    break;
  default:
    prob = 16.6;
    doc["value"] = prob;
    break;
  }

  serializeJson(doc, output);

  mqtt.publish("/system", output);
}

void resetSystem()
{
  currentIndex = 0;
}