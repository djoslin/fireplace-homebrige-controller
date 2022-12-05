// Designed and tested on Arduino Uno + Ethernet Shield + POE module.

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
//byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x97, 0x7B };
//IPAddress ip(192,168,128,205);

byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x97, 0x7B }; //physical mac address
byte ip[] = { 192, 168, 128, 205 }; // ip in lan
//byte gateway[] = { 192, 168, 128, 1 }; // internet access via router
//byte subnet[] = { 255, 255, 255, 0 }; //subnet mask
EthernetServer server(80); //server port

// Client variables 
char linebuf[80];
int charcount=0;

int const fireUp = 7;        //relay1,2 - positive voltage positive wire relay
int const fireDown = 8;      //relay3,4 - negative voltage positive wire relay
int const onSeconds = 5;     //normal level for turning on
int const offSeconds = 100;  //Just a flag for off. Actual time calculated on secondsOn.
int const incrementSeconds = 1;

int secondsOn = 0;  // 0 means off. Positive numbers are the seconds of fireOn.

void setup() { 
  // Relay modules prepared 
  pinMode(fireUp, OUTPUT);
  pinMode(fireDown, OUTPUT);
  
  // Open serial communication at a baud rate of 9600
  Serial.begin(9600);
  
  // disable SD card on ethernet shield if not in use.
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

// Display dashboard page with on/off button for relay
// It also print Temperature in C and F
void dashboardPage(EthernetClient &client) {
  client.println("<!DOCTYPE HTML><html><head>");
  client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body>");                                                             
  client.println("<h3>Arduino Fireplace Controller - <a href=\"/\">Refresh</a></h3>");
  // Status and control urls.
  client.println("<b>Fireplace secondsOn: </b></br>");
  client.println(secondsOn);
  client.println("</p>");
           
  client.println("<a href=\"/fireon\">Turn Fire On</a></br>");
  client.println("<a href=\"/fireup\">Turn Fire Up</a></br>");
  client.println("<a href=\"/firedown\">Turn Fire Down</a></br>");
  client.println("<a href=\"/fireoff\">Turn Fire Off</a></br>");
  
  client.println("</body></html>"); 
}

void loop() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
       char c = client.read();
       //read char by char HTTP request
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          dashboardPage(client);
          break;
        }
        if (c == '\n') {
          if (strstr(linebuf,"GET /fireon") > 0){
            secondsOn = fireFunction(fireUp, onSeconds);
            Serial.println("fireOn");
            Serial.println("secondsOn:");
            Serial.println(secondsOn);
          }
          else if (strstr(linebuf,"GET /fireup") > 0){
            secondsOn = fireFunction(fireUp, incrementSeconds);
            Serial.println("fireUp");
            Serial.println("secondsOn:");
            Serial.println(secondsOn);
          }
          else if (strstr(linebuf,"GET /firedown") > 0){
            secondsOn = fireFunction(fireDown, incrementSeconds);
            Serial.println("secondsOn:");
            Serial.println(secondsOn);
          }
          else if (strstr(linebuf,"GET /fireoff") > 0){
            secondsOn = fireFunction(fireDown, offSeconds);
            Serial.println("secondsOn:");
            Serial.println(secondsOn);
          }
          // you're starting a new line
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;          
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}

int fireFunction(int pin, int seconds) {  //return secondsOn

  // If the fireplace is on, then just turn up the fireplace more.
  if (seconds == onSeconds) {
    if (secondsOn >= 5) seconds = 2 * incrementSeconds;
  }

  // If the request is to turn the fireplace off, then turn down by secondsOn + 2
  if (seconds == offSeconds) {
    seconds = secondsOn + 2;
  }

  int duration;  //ms for delay
  duration = seconds * 1000;

  //Run up or down pins for duration in seconds.
  digitalWrite(pin, HIGH);
  delay(duration);
  digitalWrite(pin, LOW);
  
  //update secondsOn.
  if ( pin == fireUp ) {                //up pin, increment.
    secondsOn = secondsOn + seconds;
  }
  else {
    secondsOn = secondsOn - seconds;    //else down pin, decrement.
    if ( secondsOn < 0 ) secondsOn = 0;
  }

  return secondsOn;
}
