// ****************************************************************
// Kombucha brew temperature controller
// by Craig Hoffmann
//
// ****************************************************************

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>


// ****************************************************************
// Some defines that are used
// ****************************************************************

#define MY_HOSTNAME "mybrew"
#define ONE_WIRE_BUS 0                         // Data wire (1 wire bus) is plugged into pin GPIO 0
#define PROCESS_PERIOD_mS (1000 * 30)          // 30 Seconds
#define COUNTS_PER_CAPTURE 10                  // 10 * 30 seconds = 5mins  ie save reading every 10 PROCESS_PERIOD_mS
#define MAX_TEMPS_24HRs 288                    // Number of readings to remember for plotting 288 * 5min
#define HEAT_CONTROL 2                         // GPIO 2 used to switch heater on/off (high - off)
#define HEATER_ON 0
#define HEATER_OFF 1
#define CHART_HEATER_OFF_VALUE 0
#define CHART_HEATER_OFF_TO_ON_DELTA 1
#define VALID_DATA_CODE_ADDR 0                 // eeprom address used to store int value code
#define VALID_DATA_CODE ((int)14643)           // just a value used to flag if eeprom has been written before
#define SET_TEMP_ADDR 4                        // the target set temp address in eeprom
#define NOTES_STR_ADDR 8                       // the notes string start address in eeprom
#define MAX_NOTE_LENGTH 250


// ****************************************************************
// Setup the global variables
// ****************************************************************

const char* ssid = "";        //type your ssid
const char* password = "";    //type your password
char TempStr[6];
float Temps24hr[MAX_TEMPS_24HRs + 1];
float RoomTemps24hr[MAX_TEMPS_24HRs + 1];
char Heater24hr[MAX_TEMPS_24HRs + 1];
float SetTemp = 25.0;
int ProcessCounter = 0;
char CurrentHeater = 0;
float CurrentTemp = 0.0;
float CurrentRoomTemp = 0.0;
unsigned long PreviousMillis = 0;
char NoteStr[MAX_NOTE_LENGTH] = "Add a brief note here";

// ****************************************************************
// Start some things up
// ****************************************************************

//Web server object. Will be listening in port 80 (default for HTTP)
ESP8266WebServer server(80);   
 
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);


// ****************************************************************
// Power on/reset setup
// ****************************************************************

void setup() 
{
  int i;
  
  Serial.begin(115200);
  delay(10);
 
  pinMode(HEAT_CONTROL, OUTPUT);          
  digitalWrite(HEAT_CONTROL, LOW);

  DS18B20.begin();      // IC Default 9 bit.
   
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
   
  WiFi.begin(ssid, password);   // Connect to WiFi network
   
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.on("/", HandleRootPath);         //Associate the handler function to the path
  server.on("/ConfirmSave", HandleSaveConfirmation);   //Associate the handler function to the path
  server.begin();                         //Start the server
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  // Setup mDNS for easy address resolution
  if (!MDNS.begin(MY_HOSTNAME))   // NOTE: In windows need bonjour installed for mDNS to work, linux install Avahi
  {
    Serial.println("mDNS setup failed!");
  }
  else
  {
    Serial.print("mDNS setup http://");
    Serial.print(MY_HOSTNAME);
    Serial.println(".local/");
  }

  EEPROM.begin(512);
  EEPROM.get(VALID_DATA_CODE_ADDR,i);
  if (i==VALID_DATA_CODE)  // Does it look like data has been written to the eeprom before??
  {
    EEPROM.get(SET_TEMP_ADDR,i);
    if ((i<20) || (i>30))
    {
      EEPROM.put(SET_TEMP_ADDR,((int)SetTemp));
      EEPROM.put(NOTES_STR_ADDR,NoteStr);
      EEPROM.commit();
    }
    else
    {
      SetTemp = i;   // use the value found in eeprom
      EEPROM.get(NOTES_STR_ADDR,NoteStr);
    }
  }
  else
  {
    EEPROM.put(VALID_DATA_CODE_ADDR,VALID_DATA_CODE);
    EEPROM.put(SET_TEMP_ADDR,((int)SetTemp));
    EEPROM.put(NOTES_STR_ADDR,NoteStr);
    EEPROM.commit();
  }

  // Initialise some data
  for (i=0;i<MAX_TEMPS_24HRs;i++)
  {
    Temps24hr[i]=0;
    RoomTemps24hr[i]=0;
    Heater24hr[i] = 0;
  }

  CurrentTemp = getTemperature(0);        // Assumes first temp sensor is the brew temp
  dtostrf(CurrentTemp, 6, 1, TempStr);
  Serial.print("Starting... Brew: ");
  Serial.println(TempStr);
  CurrentRoomTemp = getTemperature(1);    // Assumes second temp sensor is the room temp
  dtostrf(CurrentRoomTemp, 6, 1, TempStr);
  Serial.print("Starting... Room: ");
  Serial.println(TempStr);

  PreviousMillis = millis();
    
}

 
void loop() 
{
  int i;
  
  // Check the Temp regularly and turn the heater on or off as required
  
  if ((millis() - PreviousMillis) >= PROCESS_PERIOD_mS) 
  {
    PreviousMillis += PROCESS_PERIOD_mS;

    CurrentTemp = getTemperature(0);  // Assumes brew sensor is the first one on onewire bus
    dtostrf(CurrentTemp, 6, 1, TempStr);
    Serial.print("Brew: ");
    Serial.println(TempStr);


    // Check if above or below set temp and turn on heater as required

    if (SetTemp > CurrentTemp)
    {
      digitalWrite(HEAT_CONTROL, HEATER_ON);
      CurrentHeater++;   
    }
    else
    {
      digitalWrite(HEAT_CONTROL, HEATER_OFF);
    }

    // Log the temp less often for charting 24hrs of data

    ProcessCounter++; 
    if (ProcessCounter >= COUNTS_PER_CAPTURE)
    {
      for (i=0;i<(MAX_TEMPS_24HRs);i++)
      {
        Temps24hr[i] = Temps24hr[i+1];
        Heater24hr[i] = Heater24hr[i+1];
        RoomTemps24hr[i] = RoomTemps24hr[i+1];
      }
      Temps24hr[MAX_TEMPS_24HRs] = CurrentTemp;
      Heater24hr[MAX_TEMPS_24HRs] = CurrentHeater;

      CurrentRoomTemp = getTemperature(1);  // Assumes room sensor is the second one on onewire bus
      dtostrf(CurrentRoomTemp, 6, 1, TempStr);
      Serial.print("Room: ");
      Serial.println(TempStr);

      RoomTemps24hr[MAX_TEMPS_24HRs] = CurrentRoomTemp;
      CurrentHeater = 0;
      ProcessCounter = 0; 

    }    
  }

  
  server.handleClient();         //Handling of incoming requests

}


float getTemperature(int Sensor) 
{
  float tempC;
  do {
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(Sensor);
    delay(100);
  } while (tempC == 85.0 || tempC == (-127.0));
  return tempC;
}


void HandleRootPath()
{
  int i;

  server.sendContent("HTTP/1.1 200 OK\r\n");    // start the page header
  server.sendContent("Content-Type: text/html\r\n");
  server.sendContent("Connection: close\r\n");  // the connection will be closed after completion of the response
  server.sendContent("Refresh: 300\r\n");       // tell browser to refresh the page automatically every 5 mins
  server.sendContent("\r\n");                   // this separates header from content that follows
  server.sendContent("<!DOCTYPE HTML>");
  server.sendContent("<html>");
  server.sendContent("<head>");
  server.sendContent("  <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>");
  server.sendContent("  <script type=\"text/javascript\">");
  server.sendContent("    google.charts.load('current', {'packages':['corechart']});");
  server.sendContent("    google.charts.setOnLoadCallback(drawChart);");

  server.sendContent("    function drawChart() {");
  server.sendContent("      var data = google.visualization.arrayToDataTable([");
  server.sendContent("        ['Reading', 'Heater', 'Brew Temp', 'Room Temp'],");
  for (i=0;i<(MAX_TEMPS_24HRs);i++)
  {
    server.sendContent("        [' ',");  
    dtostrf(Heater24hr[i] * CHART_HEATER_OFF_TO_ON_DELTA + CHART_HEATER_OFF_VALUE, 6, 1, TempStr);
    server.sendContent(TempStr);   
    server.sendContent(",");
    dtostrf(Temps24hr[i], 6, 1, TempStr);
    server.sendContent(TempStr);
    server.sendContent(",");
    dtostrf(RoomTemps24hr[i], 6, 1, TempStr);
    server.sendContent(TempStr);
    server.sendContent("],");
  }
  server.sendContent("        [' ',");
  dtostrf(Heater24hr[MAX_TEMPS_24HRs] * CHART_HEATER_OFF_TO_ON_DELTA + CHART_HEATER_OFF_VALUE, 6, 1, TempStr);
  server.sendContent(TempStr);
  server.sendContent(",");
  dtostrf(Temps24hr[MAX_TEMPS_24HRs], 6, 1, TempStr);
  server.sendContent(TempStr);
  server.sendContent(",");
  dtostrf(RoomTemps24hr[MAX_TEMPS_24HRs], 6, 1, TempStr);
  server.sendContent(TempStr);
  server.sendContent("]");

  server.sendContent("      ]);");

  server.sendContent("      var options = {");
  server.sendContent("        title: '24hr Temperature (Celcius)',");
  server.sendContent("        curveType: 'none',");
  server.sendContent("        legend: { position: 'bottom' }");
  server.sendContent("      };");

  server.sendContent("      var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));");   // was curve_chart

  server.sendContent("      chart.draw(data, options);");
  server.sendContent("    }");
  server.sendContent("  </script>");
  server.sendContent("</head>");
  server.sendContent("<body><center><h1>Brew Temp Controller</h1>");
  server.sendContent("<div id=\"curve_chart\" style=\"width: 900px; height: 400px\"></div>");
  server.sendContent("<p><form action=\"/ConfirmSave\">");
  dtostrf(SetTemp, 2, 0, TempStr);
  server.sendContent(String("Target Temperature: <input type=\"number\" name=\"SetTemp\" value=\"") + TempStr + "\" min=\"20\" max=\"30\">");
  server.sendContent("<input type=\"submit\" value=\"Update\">");
  server.sendContent("</form>");

  server.sendContent("<p><form action=\"/ConfirmSave\"><textarea rows=\"4\" cols=\"50\" name=\"Notes\" maxlength=\"250\">");
  server.sendContent(NoteStr);
  server.sendContent("</textarea><p><input type=\"submit\" value=\"Save Notes\"></form>");
  
  server.sendContent("</center></body>");

  server.sendContent("</html>");
  server.sendContent("\r\n");
  server.client().stop(); // Stop is needed because we sent no content length
  
}
/***
void HandleRootPath()
{
  int i;

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent("<!DOCTYPE html>");
  server.sendContent("<html>");
  server.sendContent("<head>");
  server.sendContent("  <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>");
  server.sendContent("  <script type=\"text/javascript\">");
  server.sendContent("    google.charts.load('current', {'packages':['corechart']});");
  server.sendContent("    google.charts.setOnLoadCallback(drawChart);");

  server.sendContent("    function drawChart() {");
  server.sendContent("      var data = google.visualization.arrayToDataTable([");
  server.sendContent("        ['Reading', 'Heater', 'Brew Temp', 'Room Temp'],");
  for (i=0;i<(MAX_TEMPS_24HRs);i++)
  {
    server.sendContent("        [' ',");  
    dtostrf(Heater24hr[i] * CHART_HEATER_OFF_TO_ON_DELTA + CHART_HEATER_OFF_VALUE, 6, 1, TempStr);
    server.sendContent(TempStr);   
    server.sendContent(",");
    dtostrf(Temps24hr[i], 6, 1, TempStr);
    server.sendContent(TempStr);
    server.sendContent(",");
    dtostrf(RoomTemps24hr[i], 6, 1, TempStr);
    server.sendContent(TempStr);
    server.sendContent("],");
  }
  server.sendContent("        [' ',");
  dtostrf(Heater24hr[MAX_TEMPS_24HRs] * CHART_HEATER_OFF_TO_ON_DELTA + CHART_HEATER_OFF_VALUE, 6, 1, TempStr);
  server.sendContent(TempStr);
  server.sendContent(",");
  dtostrf(Temps24hr[MAX_TEMPS_24HRs], 6, 1, TempStr);
  server.sendContent(TempStr);
  server.sendContent(",");
  dtostrf(RoomTemps24hr[MAX_TEMPS_24HRs], 6, 1, TempStr);
  server.sendContent(TempStr);
  server.sendContent("]");

  server.sendContent("      ]);");

  server.sendContent("      var options = {");
  server.sendContent("        title: '24hr Temperature (Celcius)',");
  server.sendContent("        curveType: 'none',");
  server.sendContent("        legend: { position: 'bottom' }");
  server.sendContent("      };");

  server.sendContent("      var chart = new google.visualization.LineChart(document.getElementById('curve_chart'));");   // was curve_chart

  server.sendContent("      chart.draw(data, options);");
  server.sendContent("    }");
  server.sendContent("  </script>");
  server.sendContent("</head>");
  server.sendContent("<body><center><h1>Brew Temp Controller</h1>");
  server.sendContent("<div id=\"curve_chart\" style=\"width: 900px; height: 400px\"></div>");
  server.sendContent("<p><form action=\"/ConfirmSave\">");
  dtostrf(SetTemp, 2, 0, TempStr);
  server.sendContent(String("Target Temperature: <input type=\"number\" name=\"SetTemp\" value=\"") + TempStr + "\" min=\"20\" max=\"30\">");
  server.sendContent("<input type=\"submit\" value=\"Update\">");
  server.sendContent("</form>");

  server.sendContent("<p><form action=\"/ConfirmSave\"><textarea rows=\"4\" cols=\"50\" name=\"Notes\" maxlength=\"250\">");
  server.sendContent(NoteStr);
  server.sendContent("</textarea><p><input type=\"submit\" value=\"Save Notes\"></form>");
  
  server.sendContent("</center></body>");

  server.sendContent("</html>");
  server.client().stop(); // Stop is needed because we sent no content length
  
}
**/

void HandleSaveConfirmation()
{
  int i;
  String ArgsString = "";
  
  if (server.arg("SetTemp") != "")
  {
    ArgsString = server.arg("SetTemp");     //Gets the value of the query parameter
    i = ArgsString.toInt();
    if ((i>=20) && (i<=30))
    {
      SetTemp = i;
      EEPROM.put(SET_TEMP_ADDR,((int)SetTemp));
    }
  }

  if (server.arg("Notes") != "")
  {
    ArgsString = server.arg("Notes");     //Gets the value of the query parameter
    ArgsString.toCharArray(NoteStr, MAX_NOTE_LENGTH);
    EEPROM.put(NOTES_STR_ADDR,NoteStr);
  }
  
  EEPROM.commit();

  server.sendContent("HTTP/1.1 200 OK\r\n");
  server.sendContent("Content-Type: text/html\r\n");
  server.sendContent("Connection: close\r\n");  // the connection will be closed after completion of the response
  server.sendContent("\r\n");                   // this separates header from content that follows
  server.sendContent("<!DOCTYPE HTML>");
  server.sendContent("<html>");
  server.sendContent("<head></head>");
  server.sendContent("<body><center><h1>Save Complete</h1>");
  server.sendContent("<p>");
  dtostrf(SetTemp, 2, 0, TempStr);
  server.sendContent(String("Target Temperature: <input type=\"number\" name=\"SetTemp\" value=\"") + TempStr + "\" readonly>");

  server.sendContent("<p><textarea rows=\"4\" cols=\"50\" name=\"Notes\" maxlength=\"250\" readonly>");
  server.sendContent(NoteStr);
  server.sendContent("</textarea><p>");

//  server.sendContent("<p><form action=\"/\"><input type=\"submit\" value=\"Return\">");
  server.sendContent("<p><form><input type=\"button\" value=\"Return\" onclick=\"window.location.href='/'\"/></form>");
  server.sendContent("</center></body>");
  server.sendContent("</html>");
  server.sendContent("\r\n");
  server.client().stop(); // Stop is needed because we sent no content length

}
