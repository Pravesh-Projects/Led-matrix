#define BLYNK_PRINT Serial
#include <Wire.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
char auth[] = "60b07653de7745bd9b2b4d5737332957";
char ssid[] = "*****"; //remove * and type your SSID
char pass[] = "&&&&&&"; //remove & and type your password
#include <SoftwareSerial.h>
SoftwareSerial EspSerial(2, 3); // RX, TX
#define ESP8266_BAUD 9600
ESP8266 wifi(&EspSerial);
WidgetTerminal terminal(V1);
BLYNK_WRITE(V1)
{
  if (String(".") == param.asStr()) {
    terminal.println("Reset to hello") ;
    Wire.beginTransmission(5);
    Wire.write('@');
    Wire.endTransmission();
} else {
    terminal.write(param.getBuffer(), param.getLength());
    Serial.println(param.asStr());
    Wire.beginTransmission(5);
    Wire.write(param.asStr());
    Wire.endTransmission();
    terminal.println();
}
terminal.flush();
}
void setup()
{
  Serial.begin(9600);
  Wire.begin();
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
  Blynk.begin(auth, wifi, ssid, pass);
  terminal.println(F("-------------"));
  terminal.flush();
}
void loop()
{
  Blynk.run();
}

