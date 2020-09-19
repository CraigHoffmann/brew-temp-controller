# brew-temp-controller

Brew Temp Controller is an ESP8266 based temperature controller for brewing kombucha.  It simply attempts to maintain a set temp for the brew using a low power heater made from a string of resistors.  This wont keep a coffee hot but is sufficient to keep my kombucha brew around 23deg where I live in Australia.  A graph of temps for the past 24hrs can be displayed via a web browser.

The wifi SSID and Password are hard coded in this project - just edit in the .ino file.  Once connected to your network the web page can be found at the ip address or http://mybrew.local/

The web page uses google charts api to draw the graph.

