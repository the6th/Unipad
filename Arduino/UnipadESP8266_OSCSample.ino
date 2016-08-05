/////////////////////////////
//
// Unipad x ESP8266
// http://unipad.digitalray.co.jp
//
/////////////////////////////
//[how to use]
//
// 1. Download Unipad on your smartphone
//  http://unipad.digitalray.co.jp/download/
//
// 2. Launch Unipad. -> new connection -> 
//   name : any you want.
//   ipaddress: none (input nothing)
//   port:6666 (or your config)
//   protcol:OSC
//
//   tap connection!
//
// 3. shown GamePad on your smartphone,when connection established.
//
// 4. check serial console on Arduino IDE.
//
// 5. Dialog is appeared ,if you press B button.
//
////////////////////////////////////
// tested on ESPr Developer
// https://www.switch-science.com/catalog/2500/
//

#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

// this scripts needs  OSC library
// download and add library
//
// https://github.com/CNMAT/OSC
//


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//----------------------------------start config
const unsigned int outPort = 6666;

int padType = 1;//gamepad

//your Wifi's ssid
const char* ssid = "**************";

//your Wifi's password
const char* password = "*************";

//----------------------------------end config

WiFiUDP Udp;
IPAddress outIp;
OSCErrorCode error;

//bit for check which button is Pressed.
int b = 0;

//button id reference to GAMEPAD API
//@see https://w3c.github.io/gamepad/#dictionary-gamepadeventinit-members
#define A 0
#define B 1
#define UP 12
#define DOWN 13
#define LEFT 14
#define RIGHT 15
#define TOUCHPAD 10
//#define MENU 16

void setup() {

  Serial.begin(115200);
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  Serial.println(WiFi.localIP());
  Udp.begin(outPort);

}

void loop() {

  rcvMessage();

  //Serial.println(b,BIN);

  if (pressed( UP ) )
    Serial.println("UP pressed");

  if (pressed( A ) )
    Serial.println("A pressed");

  if (pressed( B ) ){
    Serial.println("B pressed");
    openDialog();
  }

  delay(1);

}




bool pressed(int btnid) {
  return (b & (1 << btnid));
}

String IP2String(IPAddress address)
{
  return String(address[0]) + "." +
         String(address[1]) + "." +
         String(address[2]) + "." +
         String(address[3]);
}
void touchRepeat(OSCMessage &msg) {

  char str[8];
  int i = 0;
  b = 0;

  while ( i < msg.size()) {

    msg.getString(i, str, 8);
    if (strncmp(str, "Up", sizeof(str)) == 0) {
      b = b | (1 << UP);
    } else if (strncmp(str, "Down", sizeof(str)) == 0) {
      b = b | (1 << DOWN);
    } else if (strncmp(str, "Left", sizeof(str)) == 0) {
      b = b | (1 << LEFT);
    } else if (strncmp(str, "Right", sizeof(str)) == 0) {
      b = b | (1 << RIGHT);
    } else if (strncmp(str, "ButtonA", sizeof(str)) == 0) {
      b = b | (1 << A);
    } else if (strncmp(str, "ButtonB", sizeof(str)) == 0) {
      b = b | (1 << B);
    } else if (strncmp(str, "TouchArea", sizeof(str)) == 0) {
      b = b | (1 << TOUCHPAD);
    } else if (strncmp(str, "menu", sizeof(str)) == 0) {
      // btn[MENU] = 1;
    }
    i++;
  }//endwhile
}

void rcvMessage() {

  OSCMessage bundleIN;
  int size;
  b = 0;
  if ( (size = Udp.parsePacket()) > 0)
  {
    //Serial.println(size);
    while (size--) {
      bundleIN.fill(Udp.read());
    }

    if (!bundleIN.hasError()) {

      bundleIN.dispatch("/touch/repeat", touchRepeat);
      bundleIN.dispatch("/hello", receiveHello);
      bundleIN.dispatch("/dialog/clicked/1", receiveDialog); // 1 is dialogID   see-> openDialog()
    }
    else {
      error = bundleIN.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}


void receiveHello(OSCMessage &msg) {

  char ip[16];

  //set outputIP from /hello message
  msg.getString(0, ip, 16);
  Serial.print("param 0 is ");
  Serial.println(ip);

  msg.getString(0, ip, 16);
  outIp.fromString(ip);
  Serial.print("change oututUP is ");
  Serial.println( IP2String(outIp) );

  //get 3rd param to check broadcast or not
  msg.getString(2, ip, 16);

  if (strlen(ip) == 0) {

    //this is braod cast
    Serial.println("no ips");

    //set aruduino ip
    IP2String(WiFi.localIP()).toCharArray(ip, 16);

    char port[6];
    sprintf(port, "%d", outPort);
    OSCMessage smsg("/config/network");
    smsg.add("hogehoge" );  //this param is no use in current Unipad.
    smsg.add(ip );
    smsg.add(port);
    smsg.add(port);
    Udp.beginPacket(outIp, outPort);
    smsg.send(Udp);
    Udp.endPacket();
    smsg.empty();
    Serial.printf("set ip to %s:%d", ip, outPort);

  } else {
    Serial.println("set padtype");
    OSCMessage smsg("/config/type");
    smsg.add(padType);
    Udp.beginPacket(outIp, outPort);
    smsg.send(Udp);
    Udp.endPacket();
    smsg.empty();
  }


  char s1[50] = "<size=30>ESP IP is ";

  char s2[16];
  IP2String(WiFi.localIP()).toCharArray(s2, 16) ;
  char s3[9] = " </size>";
  strcat(s1, s2);
  strcat(s1, s3);

  OSCMessage imsg("/info");
  imsg.add( s1 );
  Udp.beginPacket(outIp, outPort);
  imsg.send(Udp);
  Udp.endPacket();
  imsg.empty();
}

void openDialog(){
  
  OSCMessage imsg("/dialog/open");
  imsg.add( "1");           //set dialog ID
  imsg.add( "openDialog");  //set dialog title
  imsg.add( "B is Pressed");//set dialog message
  imsg.add( "OK");          //set dialog button1
  imsg.add( "Next");        //set dialog button2
  Udp.beginPacket(outIp, outPort);
  imsg.send(Udp);
  Udp.endPacket();
  imsg.empty();

}
void receiveDialog(OSCMessage &msg) {

  char btn[20];
  msg.getString(0, btn  , 20);

  String strMsg = "";
  strMsg += btn;
  strMsg += " is Pressed.";
  
  char dialogmsg[40];
  strMsg.toCharArray(dialogmsg,40);
  
  OSCMessage imsg("/dialog/open");
  imsg.add( "0");             //set dialog ID
  imsg.add( "openDialog2");   //set dialog title
  imsg.add( dialogmsg);       //set dialog message
  imsg.add( "OK");            //set dialog button1
  Udp.beginPacket(outIp, outPort);
  imsg.send(Udp);
  Udp.endPacket();
  imsg.empty();

}