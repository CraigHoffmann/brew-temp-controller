# brew-temp-controller

Brew Temp Controller is an ESP8266 based temperature controller for brewing kombucha.  It simply attempts to maintain a set temp.  A graph of temps for the past 24hrs can be displayed via a web browser.

The wifi SSID and Password are hard coded in this project - just edit in the .ino file.  Once connected to your network the web page can be found at the ip address or http://mybrew.local/

The web page uses google charts api to draw the graph.

