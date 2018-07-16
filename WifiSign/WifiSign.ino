/*
 * WiFi sign - Duinotech
 * Author Dre West
 */

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <DMD2.h>
#include <fonts/Arial14.h>


const char* ssid = "draggy";  //your wifi network name
const char* pwd = "";   //your wifi password
String location = "mainroom";  //set this to the location where the sign is, one word.



const int SIGN_WIDTH = 1; //these make the sign 1x1 DMD panel
const int SIGN_HEIGHT = 1;

WiFiServer server(80);  //start a WiFi server on port 80
HTTPClient http;
String command;         //this is the string that we are going to get from the server.
String line;            //this is a simple line buffer to process the entire text from the server, line by line.


void sendPage(WiFiClient c); //this is a function that we can write later, define it now so we can use it while it's empty
short nextEndpoint(short index, int* endpoint_list, short len); //find the next endpoint from an index point
int fillEndpointList(const char* cmd_str, int len, int* endpoint_list); //fill the endpoint list

bool clockMode = false;

unsigned long last_runtime = 0;
unsigned long refresh_runtime = 60000; // 1 minute

SPIDMD dmd(SIGN_WIDTH,SIGN_HEIGHT);  // DMD controls the entire display
DMD_TextBox box(dmd);  // "box" provides a text box to automatically write to/scroll the display
const uint8_t *FONT = Arial14;


void setup()
{
	Serial.begin(115200);

	//WiFi setup code
	Serial.print("Connecting to: ");
	Serial.println(ssid);

 String hostn = location + ".local";

	WiFi.hostname(hostn.c_str());   //on latest windows, linux and mac, you can type "<location>.local" to connect to this device instead of pin numbers

	//connect to the network
	WiFi.begin(ssid,pwd);
	while(WiFi.status() != WL_CONNECTED){
		//not connected yet, give it time.
		delay(500);
		Serial.print(".");
	}

	//print out some nice information
	Serial.println("Wifi Connected");
	Serial.print("IP: ");
	Serial.println(WiFi.localIP());
	Serial.print("Or try using http://");
	Serial.println(hostn);
	server.begin();

  dmd.setBrightness(255);
  dmd.selectFont(FONT);
  dmd.begin();
}

void loop()
{
	//check for a client
	WiFiClient client = server.available();

	//clear the command
	command = "";

	//the next few lines are tricky to understand but forms the basis of network connection programming

	//if we have a client
	if(client)
	{
		//clear the line buffer
		line = "";

		//while the client is connected
		while(client.connected())
		{
			//if the client has incoming data available
			if (client.available())
			{

				//read a byte of that data
				byte c = client.read();

				//write it to Serial, you should see a letter, because the byte is valid ascii
				//Serial.write(c);

				//if it is a newline character, we have finished this line so we can check it.
				if(c == '\n')
				{
					//begin processing line:

          //Serial.println(line);
          
					//check if the line starts with "GET" which is a HTML word
					if(line.startsWith("GET /?"))
					{ 
						//this line has the params, so copy it into our command buffer
						command = line;
            Serial.println("found command params");
            Serial.println(line);
						// keep getting the information though, as we need to listen to the client before we can respond.
					}


					//check if this line is empty, if it is, we have 2 newlines in a row 
					//(first one finished last line, this is empty)
					if (line.length() == 0)
					{ 

						//this is the end of the request. they have sent two new lines in a row, 
						//so here we respond by sending a page back.
						Serial.println("end of client request, responding");

						//send a page to the client in response
						sendPage(client);

						//break out of "while (client connected)"
						break; 

					} 

					//the line is finished, but it's not GET and it's not empty, 
					//so we don't care about it. clear out the line buffer
					line = "";          
				} 
				else 
				{
					//it is not a newline character, but we stil have a character 
					//if we got anything other than an \r (we don't want \r)
					if ( c != '\r')
					{
						//add it to the line buffer     
						line += (char)c;            
					}
				}
				//keep looping through the data sent from the client, 
			}
		}

    Serial.println(command);

		/*
		 * Here we have finished receiving data so we respond.
		 */
		//here he's no longer sending or receiving data from us, so check if we have a command and process it.
		int endpoints[16] = {0}; //have a list of parameter endpoints.

		if(command.length() > 0){


			/*
			 * The command line would look something like /?option=text&var=value&var2=value2& ...
			 * so we make a list of 16 endpoints, which stores the index of all the '?' and '&'
			 * then when we find the index of, say, 'option' we can then find the next end point to extract the entire value
			 */

			int n_endpoints = fillEndpointList(command.c_str(), command.length(), endpoints);

			//here is where we define our commands.



			// ?option=""
			short opt_index_start = command.indexOf("option=")+7; //the start of the command value.
			short opt_index_end = nextEndpoint(opt_index_start,endpoints,n_endpoints); //end of command value (start of another command)

			String opt_value = command.substring(opt_index_start,opt_index_end);

			Serial.println(command);
      Serial.println(n_endpoints);
      for(int j = 0; j < n_endpoints; j++){
        Serial.print(endpoints[j],DEC);
        Serial.print(",");}
			Serial.print("opt_indexes: ");
			Serial.print(opt_index_start,DEC);
			Serial.print("-");
			Serial.println(opt_index_end,DEC);
			Serial.print("option=");
			Serial.print(opt_value);
			Serial.println();

			//we have the opt_value stored in a string, so we can compare it to see what the user wants to do
			if(opt_value == "text")
			{
				clockMode = false;
				//we got a text command


				// ?text=""
				short txt_index_start = command.indexOf("text=")+5; //the start of the text value
				short txt_index_end = nextEndpoint(txt_index_start,endpoints,n_endpoints);
				String txt_value = command.substring(txt_index_start, txt_index_end);

				Serial.println(command);
				Serial.print("txt_indexes: ");
				Serial.print(txt_index_start,DEC);
				Serial.print("-");
				Serial.println(txt_index_end,DEC);
				Serial.print("text=");
				Serial.print(txt_value);
				Serial.println();
				// we have text stored in a string, so format this to make it a little more nicer to read

				txt_value.replace("+"," "); //html will automatically change spaces to + signs so we change it back

				Serial.print("setting message to: ");
				Serial.println(txt_value);

				dmd.drawString(0, 0, txt_value.c_str());

			}
			else if (opt_value == "clock")
			{ 
				//we got a clock command
				clockMode = true;
			}
			else
			{
				//not sure what this option is is
				Serial.println("not sure what option is:");
				Serial.println(opt_value);
			}
		}
		client.stop();                    //close connection
	}

	if (clockMode)
	{
		String datetime;
		unsigned long current_runtime = millis();
		if (current_runtime - last_runtime > refresh_runtime)
		{
      last_runtime = current_runtime;
			//get google, they have GMT in the response
			http.begin("www.google.com");
			const char* important_headers[] = { "Date" };
			http.collectHeaders(important_headers, (size_t)1 );      

			int rc = http.GET();
			if(rc > 0)
			{
				if(http.hasHeader("Date")){
					datetime = http.header("Date");
/*
         __                              ____                        ________  _________
   _____/ /_  ____ _____  ____ ____     / __/________  ____ ___     / ____/  |/  /_  __/
  / ___/ __ \/ __ `/ __ \/ __ `/ _ \   / /_/ ___/ __ \/ __ `__ \   / / __/ /|_/ / / /   
 / /__/ / / / /_/ / / / / /_/ /  __/  / __/ /  / /_/ / / / / / /  / /_/ / /  / / / /    
 \___/_/ /_/\__,_/_/ /_/\__, /\___/  /_/ /_/   \____/_/ /_/ /_/   \____/_/  /_/ /_/     
                       /____/                                                           
*/
					//here we just write out what we've got as GMT to the panel.
					dmd.drawString(0, 0, datetime.c_str());
				}
			}
			else 
			{
				Serial.println("error getting time from google");
				Serial.println(rc,DEC);
			}
		}
	}

	for(int i = 0; i < 48; i++)
	{
		dmd.marqueeScrollX(1);
		delay(10);
	}
	for(int i = 0; i < 48; i++)
	{
		dmd.marqueeScrollX(-1);
		delay(10);
	}
}
//returns the index of the next endpoint, coming from index, for max of len.
short nextEndpoint(short index, int* endpoint_list, short len){
	short i;
	for(i = 0; i < len; i++)
		if (endpoint_list[i] > index)
			return endpoint_list[i];
	return 0;
}

//fills the list of endpoints.
int fillEndpointList(const char* cmd_str, int len, int* endpoint_list){
	int ep = 0;
	int i;
	for(i = 0; i < len; i++){
		if (cmd_str[i] == '?' || cmd_str[i] == '&' || cmd_str[i] == ' '){
			endpoint_list[ep] = i;
			ep++;
		}
	}
	endpoint_list[ep] = ep;
	return ep;
}

void sendPage(WiFiClient c){
	c.println("HTTP/1.1 200 OK");
	c.println("Content-type:text/html");
	c.println();


	//print out html header

	c.println("<html>"
			"<head>"
			"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
			"<title>");

	c.println(location); //location in the title

		c.println("</title>"
				"<style>"
				"*{ font-family: sans-serif;"
				" border-radius: 5px;"
				"}"
				"h1,h3{"
				" text-align: center;"
				"}"
				"input[type=text]{"
				" width: 100%;"
				" padding: 5;"
				" margin: 3;"
				" margin-bottom: 25;"
				" margin-top:10;"
				" border: 1px solid darkgrey;"
				"}"
				"input[type=submit]{"
				" background-color: #4286f4;"
				" color: white;"
				" padding: 10;"
				" margin: 5%;"
				" width: 90%;"
				" font-weight: bold;"
				" font-size: 24px;"
				"}"
				"</style>"
				"</head>"
				);

	c.print("<h1>");
	c.print(location);   //location in the header
	c.println("</h1>");

	c.println("<h3>Jaycar - Wireless Sign</h3>"
			"<form>"
			"<fieldset><legend>Dot Matrix Display</legend>"
			"<input type='radio' name='option' value='text' checked>Show Text:<br>"
			"<input type='text' name='text' ><br>"
			"<input type='radio' name='option' value='clock'>Show Clock<br>"
			"<br>"
			"<input type='submit' Value='Confirm'>"
			"</fieldset>"
			"</form>"
			"</html>");
	//final line to finish it off 
	c.println();
}



