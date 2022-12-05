// based on https://www.electronics-lab.com/project/esp32-webserver-tutorial/

// Designed and tested on olimex ESP32-POE-ISO
// This board is less than 1/3 the cost of Arduino Uno + Ethernet shield+ POE module.

#include <WiFi.h>  //used for webserver in all cases.
#include <ETH.h>   //only used for ethernet connections.

// For Wifi only. Replace with your network credentials 
// const char* ssid = "SSID-NAME"; 
// const char* password = "PASSWD"; 

// Set web server port number to 80 
WiFiServer server(80); 

// Variable to store the HTTP request String header;
String header;

String stringIP;  // text version of IP Address.

// Fireplace variables.
int const fireUp = 3;        //relay1,2 - positive voltage positive wire relay
int const fireDown = 4;      //relay3,4 - negative voltage positive wire relay
int const onSeconds = 5;     //normal level for turning on
int const offSeconds = 100;  //Just a flag for off. Actual time calculated on secondsOn.
int const incrementSeconds = 1;
int secondsOn = 0;  // 0 means off. Positive numbers are the seconds of fireOn.

int fireFunction(int pin, int seconds) {  //return secondsOn

  // If the request is turn on and the fireplace is already on (secondsOn >0),
  // then just turn up the fireplace more.
  if (seconds == onSeconds) {
    Serial.println("seconds=onSeconds"); //debug
    if (secondsOn > 0 ) seconds = 2;
  }

  // If the request is to turn the fireplace off, then turn down by secondsOn + 2
  if (seconds == offSeconds) {
    seconds = secondsOn + 2;
    Serial.println(seconds);  //debug
  }
  
  //Run up or down pins for duration seconds in ms.
  digitalWrite(pin, HIGH);
  delay(seconds * 1000);
  digitalWrite(pin, LOW);
  
  //set secondsOn.
  if ( pin == fireUp ) {                  //up pin, increment.
    secondsOn = secondsOn + seconds;
  }
  else {
    secondsOn = secondsOn - seconds;      //else down pin, decrement.
    if ( secondsOn < 0 ) secondsOn = 0;
  }

  return secondsOn;
}


void setup() {
  Serial.begin(115200);

  // Relay modules prepared 
  pinMode(fireUp, OUTPUT);
  pinMode(fireDown, OUTPUT);

  // For Wifi connection:
  // WiFi.begin();
  // comment out WiFi.config line for DHCP.
  // WiFi.config(IPAddress(192, 168, 128, 207),IPAddress(192, 168, 128, 1),IPAddress(255, 255, 255, 0),IPAddress(192, 168, 128, 1));
  // stringIP = WiFi.localIP().toString().c_str();
  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  //For ethernet connection:
  ETH.begin();
  // comment out ETH.config line for DHCP.
  ETH.config(IPAddress(192, 168, 128, 207),IPAddress(192, 168, 128, 1),IPAddress(255, 255, 255, 0),IPAddress(192, 168, 128, 1));
  stringIP = ETH.localIP().toString().c_str();

  // Print local IP address and start web server
  Serial.println("");
  //  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(stringIP);     // Network ip
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Path based contols
            if (header.indexOf("GET /fireon") >= 0) {
              secondsOn = fireFunction(fireUp, onSeconds);
              Serial.println("fireOn");
              Serial.print("secondsOn: ");
              Serial.println(secondsOn);
            } else if (header.indexOf("GET /fireup") >= 0) {
              secondsOn = fireFunction(fireUp, incrementSeconds);
              Serial.println("fireUp");
              Serial.print("secondsOn: ");
              Serial.println(secondsOn);
            } else if (header.indexOf("GET /firedown") >= 0) {
              secondsOn = fireFunction(fireDown, incrementSeconds);
              Serial.println("fireDown");
              Serial.print("secondsOn: ");
              Serial.println(secondsOn);
            } else if (header.indexOf("GET /fireoff") >= 0) {
              secondsOn = fireFunction(fireDown, offSeconds);
              Serial.println("fireOff");
              Serial.print("secondsOn: ");
              Serial.println(secondsOn);
            }
       
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto;</style></head>");

            // Web Page Heading
            client.println("<h3>ESP32 Fireplace Web Server - <a href=\"/\">Refresh</a></h3>");
            
            // Display current state 
            client.println("<p>Fireplace Seconds on: " + String(secondsOn) + "</p>");
            
            // Display the controls      
            client.println("<p><a href=\"/fireon\"><button>Turn Fire On</button></a> - http://" + stringIP + "/fireon</p>");
            client.println("<p><a href=\"/fireup\"><button>Turn Fire Up</button></a> - http://" + stringIP + "/fireup</p>");
            client.println("<p><a href=\"/firedown\"><button>Turn Fire Down</button></a> - http://" + stringIP + "/firedown</p>");
            client.println("<p><a href=\"/fireoff\"><button>Turn Fire Off</button></a> - http://" + stringIP + "/fireoff</p>");
               
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
