# TPE-Module v3

This will be the reworked (but still not finished) third version of the TPE-Module when I upload the Code.

Major changes:

Software:
* supports multiple users
* Four user groups: not logged in, viewer, guest/controller, admin (first not used yet in webapp)
* Requests still via HTTP Post/Put, responses are in JSON format
* Now features a "real" web server that serves arbitrary files
* Therefore can serve any webpage/webapp you want, customize it!
* Fileupload for generic files, use those 3MB of SPIFFS
* No more flashing or file writing/reading via USB-SerialAdapter, no bypass to files except for opening the device
* OTA (Binary file upload plus user triggered update process)
* GPS! Get the wearers location. Or disable it to save power.
* Hide it if you want to. Does only serve the page on /webapp. optionally serves a dummy/decoy page on / ("oh, thats just my RaspberryPi I use to automatically water the plants.")
* New webapp, no more static html-page
* Webapp way more comfortable to use
* Controls for outputs send data as soon as you lift your finger from the screen or mouse button
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
