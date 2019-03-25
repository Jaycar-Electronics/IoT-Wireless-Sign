// IoT Wireless Sign

// NTP client
#include <NTPClient.h>
//ESP Wifi server
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// For the Dot Matrix Display
#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial14.h>

#define WIFI_SSID ""
#define WIFI_PWD ""
#define LOCATION "mainroom.local" //keep the .local part of this 

WiFiUDP ntpUDP;
//set offset relying on your timezone
NTPClient timeClient(ntpUDP, "au.pool.ntp.org", 0, 60000);


/* DMD variables*/
const int SIGN_WIDTH = 1; //these make the sign 1x1 DMD panel
const int SIGN_HEIGHT = 1;
const uint8_t *FONT = Arial14;

SPIDMD dmd(SIGN_WIDTH,SIGN_HEIGHT);  // DMD controls the entire display
DMD_TextBox box(dmd);  // "box" provides a text box to automatically write to/scroll the display
long lasttime = 0; // to manage the marquee scrolling


ESP8266WebServer server(80);  //start a WiFi server on port 80

String signDisplay = "Hello world";
int signSpeed = 40;
int signMode = 0;
// signMode = 0; // display Message marquee
// signMode = 1; // display Time


void setup()
{
	Serial.begin(115200);
	//WiFi setup code
	Serial.print("Connecting to: ");
	Serial.println(WIFI_SSID);

	String hostn = LOCATION;
	// on latest windows, linux and mac, you can type "<location>.local" to connect to this device instead of IP numbers
	if( WiFi.hostname(hostn)){
		Serial.println("Unable to set hostname: hostname too long");
		Serial.println(hostn);
	}
	//connect to the network
	WiFi.begin(WIFI_SSID, WIFI_PWD);

	while(WiFi.status() != WL_CONNECTED){
		//not connected yet, give it time.
		delay(500);
		Serial.print(".");
	}

	// print out information to serial console
	Serial.println("Wifi Connected");
	Serial.print("IP: ");
	Serial.println(WiFi.localIP());
	Serial.print("Or try using http://");
	Serial.println(WiFi.hostname());

	timeClient.begin(); //start the ntp timeclient

	// Define some better functions here, such as a post request
	server.on("/set", HTTP_POST, [](){
		signDisplay = server.arg("display");
		String mode = server.arg("timeMode");
		String speed = server.arg("timeSpeed");

		signSpeed = speed.toInt();
		signMode = mode.toInt();

		Serial.print("Web Client set the string to: ");
		Serial.println(signDisplay);
		Serial.print("delaytime is set to: ");
		Serial.println(signSpeed, DEC);

		if(signMode){
			dmd.drawString(0,0, timeClient.getFormattedTime());
		}
		else{
			dmd.drawString(0,0, signDisplay);
		}
		server.sendHeader("Location", String("/"), true);
		server.send(302, "text/plain", "Ok, string set, redirecting"); //send signDisplay back to confirm;
	});

	//when we don't have a mount, look on SPIFFS, and return 404 if SPIFFS has nothing
	server.onNotFound([](){
		if(!fileRead(server.uri())){
			server.send(404, "text/html", "<h1> 404 File not Found on SPIFFS</h1>");
		}
	});

	//start up the webserver
	server.begin();

	// Start up the DMD
	dmd.setBrightness(255);
	dmd.selectFont(FONT);
	dmd.begin();

	Serial.println("Server and DMD started");
	dmd.drawString(0,0, WiFi.localIP().toString());
	lasttime = millis();
}

void loop()
{
	if (millis() > lasttime + signSpeed){
		lasttime = millis();
		dmd.marqueeScrollX(1);
	}
	timeClient.update();
	server.handleClient();
}

bool fileRead(String filepath){

	if(filepath.endsWith("/")) filepath += "index.html";

	Serial.println(filepath);
	if( SPIFFS.exists(filepath) ) {
		File f = SPIFFS.open(filepath, "r");
		server.streamFile(f, contentType(filepath));
		f.close();
		return true;
	}
	return false;
}

String contentType(String filepath){
	if(filepath.endsWith(".html")) return "text/html";
	else if(filepath.endsWith(".css")) return "text/css";
	else if(filepath.endsWith(".js")) return "text/javascript";
	else return "text/plain";
}
