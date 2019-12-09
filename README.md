# TPE-Module v3

### This is be the reworked (but still not finished) third version of the TPE-Module. ###

#### Major changes: ####

Software:
* Supports multiple users
* Four user groups: not logged in, viewer, guest/controller, admin (first not used yet in webapp)
* Session tokens!
* Requests still via HTTP Post/Put, responses are in JSON format
* Now features a "real" web server that serves arbitrary files
* Therefore can serve any webpage/webapp you want, customize it!
* Fileupload for generic files, use those 3MB of SPIFFS
* No more flashing or file writing/reading via USB-SerialAdapter, no bypass to files except for opening the device
* OTA (Binary file upload plus user triggered update process)
* GPS! Get the wearers location. Or disable it to save power.
* Hide it if you want to. Does only serve the page on /webapp if you don't want it to serve it on /. Optionally serves a dummy/decoy page on / ("oh, thats just my RaspberryPi I use to automatically water the plants.")
* New webapp, no more static html-page (alter the existing one, upload any custom webapp you like, I'm no web developer)
* Webapp way more comfortable to use than the old static served html page.
* Controls for outputs send data immediately or as soon as you lift your finger from the screen or mouse button (Button, Sliders, dropdowns), no need to enter your password all the time
* More options, more data, more controll (the T stands for "total", remember? ;) )
* Upload script for initial file upload (Webapp files to configure the first user and demo sequence files)

Hardware:
* One more controllable output (now 6)
* GPS, enable/disable
* flat foil-type button

Broken, worked once until a library update, tried to fix it:
* Control via a server (regularly sends requests via HTTPS to a server and reacts according to the response)

Planned features:
* Rules (on battery state, geofencing via GPS and/or WIFI, etc.)


#### Notes ####
This project uses a slightly modified version of the NTP client library to set the time using GPS if available. Changes done  Disable by making setTimeFromGPS() return immediately. 

### Disclaimer and License ###
This project is a hobby project, I cannot guarantee anything but that I had no bad intentions when writing this (should not do damage on my purpose), but it's "as is", use at your own risk.

Use this project for good things like having fun together with someone, don't use it against someones will or to damage someone or something unlawfully. Don't sell this software. If you want to sell the hardware, don't make profit from it except you and me agreed otherwise.
