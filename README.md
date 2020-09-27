# Brew Temp Controller

Brew Temp Controller is an ESP8266-01 based temperature controller for brewing kombucha.  The temp can be monitored via a browser over wifi.  It simply attempts to maintain a set temp for the brew using a low power heater made from a string of resistors.  This wont keep a coffee hot but is sufficient to keep my kombucha brew around 25degC where I live in Australia.  It struggles when the room temp drops about 7-8degC below the set temp for an extended period of time.  A graph of temps for the past 24hrs can be displayed via a web browser.  It monitors both the room temperature and the brew temp using a couple of DS18B20 one wire sensors

The wifi SSID and Password are hard coded in this project - just edit in the .ino file.  Once connected to a wifi network the web page can be found at the dhcp assigned ip address or http://mybrew.local/ using mDNS.

The web page uses ![google charts](https://developers.google.com/chart) api to draw the graph.

![mybrew Web Page](https://github.com/CraigHoffmann/brew-temp-controller/blob/master/mybrew.png?raw=true)

![mybrew Web Page](https://github.com/CraigHoffmann/brew-temp-controller/blob/master/Images/BrewTempControlSchematic.jpg?raw=true)

![mybrew Web Page](https://github.com/CraigHoffmann/brew-temp-controller/blob/master/Images/BrewTempControlWiring.jpg?raw=true)

The string of resistors are used as the heater element and are stitched inside a "blanket" that wraps around the brew.  One of the DS18B20 temp sensors is also stitched inside the blanket.  When wrapped around the brew container they shound both make contact with the container to provide best heating and temp measurement.

![mybrew Web Page](https://github.com/CraigHoffmann/brew-temp-controller/blob/master/Images/resistors.jpg?raw=true)

Below you can see how it all looks when setup.

<img src="https://github.com/CraigHoffmann/brew-temp-controller/blob/master/Images/heatersetup.jpg?raw=true" width="45%">
<img src="https://github.com/CraigHoffmann/brew-temp-controller/blob/master/Images/brew.jpg?raw=true" width="45%">

And this is the end result after a successful brew.

![mybrew Web Page](https://github.com/CraigHoffmann/brew-temp-controller/blob/master/Images/bottled.jpg?raw=true)
