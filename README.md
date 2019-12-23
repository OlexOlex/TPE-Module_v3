# TPE-Module v3

### This is be the reworked (but still not finished) third version of the TPE-Module. ###
Development environment: Arduino  
Libraries used (might not be a complete list): Arduino standard libraries for ESP8266, ESP8266WiFi.h, ESP8266WebServer.h, FS.h, NTPClient.h, WiFiUDP.h, DNSServer.h, ArduinoOTA.h, ESP8266mDNS.h

#### Major changes: ####

Software:
* Supports multiple users (saved in local file)
* Four user groups: not logged in, viewer, guest/controller, admin (first not used yet in webapp)
* Session tokens!
* Requests still via HTTP Post/Put, responses are in JSON format
* Now features a "real" web server that serves arbitrary files (except for the config files and users "database" file)
* Therefore can serve any webpage/webapp you want, customize it!
* Fileupload for generic files, use those 3MB of SPIFFS!
* No more flashing or file writing/reading via USB-SerialAdapter, no bypass to files except for opening the device
* OTA (Binary file upload plus user triggered update process)
* GPS! Get the module's location. Or disable it to save power.
* Hide it if you want to. Does only serve the page on /webapp if you don't want it to serve it on /. Optionally serves a dummy/decoy page on / ("oh, thats just my RaspberryPi I use to automatically water the plants.")
* New webapp, no more static html-page (alter the existing one, upload any custom webapp you like, I'm no web developer)
* Webapp way more comfortable to use than the old static served html page.
* Controls for outputs send data immediately or as soon as you lift your finger from the screen or mouse button (Button, Sliders, dropdowns), no need to enter your password all the time
* More options, more data, more controll (the T stands for "total", remember? ;) )
* Upload script for initial file upload (Webapp files to configure the first user and demo sequence files)

Hardware:
* One more controllable output (now 6)
* GPS module
* Enable/disable GPS on hardware level (power off)
* Flat foil-type button to save space for output no.6
* no more USB-Serial adapter

Broken, worked once until a library update, tried to fix it:
* Control via a server (regularly sends requests via HTTPS to a server and reacts according to the response)

Planned features:
* Rules (on battery state, time, geofencing via GPS and/or WIFI, etc.)


#### Notes ####
This project uses a slightly modified version of the NTP client library to set the time using GPS if available. Changes done can be seen in customchanges_NTPClient.txt  
Don't use GPS anyway? Disable it simply by replacing the code in setTimeFromGPS() by "return;" to make it return immediately. 

### Disclaimer and License ###
This project is a hobby project thet grew over time, I cannot guarantee anything but that I had no bad intentions when writing this (does not do damage on my purpose), but it's "as is", use at your own risk, there are flaws and even thou I tested it regularly there are probably bugs in it.

License see License.txt
