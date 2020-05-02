//IP ADDRESS : 192.168.43.199   

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#define  DEBUG  0
#if  DEBUG
#define PRINT(s, x) { Serial.print(F(s)); Serial.print(x); }
#define PRINTS(x) Serial.print(F(x))
#define PRINTX(x) Serial.println(x, HEX)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTX(x)
#endif
#define MAX_DEVICES 8
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D8
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
const char* ssid = "******"; //remove * and type your SSID
const char* password = "&&&&&&&"; //remove & and type your password
WiFiServer server(80);
uint8_t frameDelay = 75;  
textEffect_t  scrollEffect = PA_SCROLL_LEFT;
#define BUF_SIZE  512
char curMessage[BUF_SIZE];
char newMessage[BUF_SIZE];
bool newMessageAvailable = false;
int arr=0;
const char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
int x=0;
char WebPage[] =
"<!DOCTYPE html>" \
"<html>" \
"<head>" \
"<title>CORI LED Matrix Display</title>" \

"<script>" \
"strLine = \"\";" \

"function SendData()" \
"{" \
"  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
"  var request = new XMLHttpRequest();" \
"  strLine = \"&MSG=\" + document.getElementById(\"data_form\").Message.value;" \
"  strLine = strLine + \"/&SD=\" + document.getElementById(\"data_form\").ScrollType.value;" \
"  strLine = strLine + \"/&I=\" + document.getElementById(\"data_form\").Invert.value;" \
"  strLine = strLine + \"/&SP=\" + document.getElementById(\"data_form\").Speed.value;" \
"  request.open(\"GET\", strLine + nocache, false);" \
"  request.send(null);" \
"}" \
"</script>" \
"</head>" \

"<body>" \
"<p><b>CORI LED Matrix Display</b></p>" \

"<form id=\"data_form\" name=\"frmText\">" \
"<label>Message:<br><input type=\"text\" name=\"Message\" maxlength=\"255\"></label>" \
"<br><br>" \
"<input type = \"radio\" name = \"Invert\" value = \"0\" checked> Normal" \
"<input type = \"radio\" name = \"Invert\" value = \"1\"> Inverse" \
"<br>" \
"<input type = \"radio\" name = \"ScrollType\" value = \"L\" checked> Left Scroll" \
"<input type = \"radio\" name = \"ScrollType\" value = \"R\"> Right Scroll" \
"<br><br>" \
"<label>Speed:<br>Fast<input type=\"range\" name=\"Speed\"min=\"10\" max=\"200\">Slow"\
"<br>" \
"</form>" \
"<br>" \
"<input type=\"submit\" value=\"Send Data\" onclick=\"SendData()\">" \
"</body>" \
"</html>";

char *err2Str(wl_status_t code)
{
  switch (code)
  {
  case WL_IDLE_STATUS:    return("IDLE");           break; 
  case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; 
  case WL_CONNECTED:      return("CONNECTED");      break; 
  case WL_CONNECT_FAILED: return("CONNECT_FAILED"); break; 
  case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; 
  default: return("??");
  }
}

uint8_t htoi(char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}

void getData(char *szMesg, uint8_t len)
{
  char *pStart, *pEnd;      

  // check text message
  pStart = strstr(szMesg, "/&MSG=");
  if (pStart != NULL)
  {
    char *psz = newMessage;

    pStart += 6;  
    pEnd = strstr(pStart, "/&");

    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isdigit(*(pStart + 1)))
        {
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }

      *psz = '\0'; 
      newMessageAvailable = (strlen(newMessage) != 0);
      PRINT("\nNew Msg: ", newMessage);
       for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
 
      for(int i=0; i<strlen(newMessage); i++){
        EEPROM.write(i, newMessage[i]);
        x=1;
    }EEPROM.commit();Serial.println(newMessage);
  }

  
  pStart = strstr(szMesg, "/&SD=");
  if (pStart != NULL)
  {
    pStart += 5;  

    PRINT("\nScroll direction: ", *pStart);
    scrollEffect = (*pStart == 'R' ? PA_SCROLL_RIGHT : PA_SCROLL_LEFT);
    P.setTextEffect(scrollEffect, scrollEffect);
    P.displayReset();
  }

  
  pStart = strstr(szMesg, "/&I=");
  if (pStart != NULL)
  {
    pStart += 4; 

    PRINT("\nInvert mode: ", *pStart);
    P.setInvert(*pStart == '1');
  }

  // check speed
  pStart = strstr(szMesg, "/&SP=");
  if (pStart != NULL)
  {
    pStart += 5;  // skip to start of data

    int16_t speed = atoi(pStart);
    PRINT("\nSpeed: ", P.getSpeed());
    P.setSpeed(speed);
    frameDelay = speed;
  }
}}

void handleWiFi(void)
{
  static enum { S_IDLE, S_WAIT_CONN, S_READ, S_EXTRACT, S_RESPONSE, S_DISCONN } state = S_IDLE;
  static char szBuf[1024];
  static uint16_t idxBuf = 0;
  static WiFiClient client;
  static uint32_t timeStart;
  switch (state)
  {
  case S_IDLE:  
    PRINTS("\nS_IDLE");
    idxBuf = 0;
    state = S_WAIT_CONN;
    break;
  case S_WAIT_CONN:   
  {
    client = server.available();
    if (!client) break;
    if (!client.connected()) break;
#if DEBUG
    char szTxt[20];
    sprintf(szTxt, "%03d:%03d:%03d:%03d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
    PRINT("\nNew client @ ", szTxt);
#endif
    timeStart = millis();
    state = S_READ;
  }
  break;
  case S_READ: 
    PRINTS("\nS_READ ");
    while (client.available())
    {
      char c = client.read();
      if ((c == '\r') || (c == '\n'))
      {
        szBuf[idxBuf] = '\0';
        client.flush();
        PRINT("\nRecv: ", szBuf);
        state = S_EXTRACT;
      }
      else
        szBuf[idxBuf++] = (char)c;
    }
    if (millis() - timeStart > 1000)
    {
      PRINTS("\nWait timeout");
      state = S_DISCONN;
    }
    break;
  case S_EXTRACT: 
    PRINTS("\nS_EXTRACT");
    getData(szBuf, BUF_SIZE);
    state = S_RESPONSE;
    break;
  case S_RESPONSE: 
    PRINTS("\nS_RESPONSE");
    client.print(WebResponse);
    client.print(WebPage);
    state = S_DISCONN;
    break;
  case S_DISCONN: 
    PRINTS("\nS_DISCONN");
    client.flush();
    client.stop();
    state = S_IDLE;
    break;
  default:  state = S_IDLE;
  }
}
void setup()
{
  Serial.begin(57600);
  PRINTS("\n[MD_Parola WiFi Message Display]\nType a message for the scrolling display from your internet browser");
  EEPROM.begin(512); 
  P.begin();
  P.displayClear();
  P.displaySuspend(false);
  P.displayScroll(curMessage, PA_LEFT, scrollEffect, frameDelay);
  curMessage[0] = newMessage[0] = '\0';
  PRINT("\nConnecting to ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    PRINT("\n", err2Str(WiFi.status()));
    delay(500);
  }
  PRINTS("\nWiFi connected");
  server.begin();
  PRINTS("\nServer started");
  fun();
}

void loop()
{
  handleWiFi();

  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
}
void fun(){
  char Data[100];
  for(int i=0; i<17; i++){
    Data[i] = char(EEPROM.read(i));
  }
  Serial.println(Data);
  sprintf(curMessage,Data);
  PRINT("\nAssigned IP ", curMessage);
}

