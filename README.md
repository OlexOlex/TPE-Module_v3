# TPE-Module v3

### This is the reworked (but still not completely finished) third version of the TPE-Module. ###

**A simple, small, cheap and relatively easy to build module to control 6 outputs freely configurable as dimming (PWM) output, binary (on/off) output or pulse output via WIFI and a webinterface** (designed for controlling vibrators and other custom modules, especially in BDSM play)
Features:

Runs on any newer ESP8266 module (the ones with more RAM than the first version, usually featuring 4MB flash. 512MB flash is too small)


## Features:
* Provides a **WIFI-accesspoint** and/or **connects to a (list of) WIFI(s)**, e.g. control it via your home WIFI or outside via the accesspoint or your phones' accesspoint.
* **Control via web page / web app**, works in virtually any modern browser on any system, as long as it's in the same network (Tested on linux and Android, only Windows seems to make trouble) - you can also control it with anything that can send HTTP-POST request, e.g. with curl or wget from a linux server, e.g. in a cronjob or bash-/shellscript on a Raspberry Pi
* **No need for a dedicated app**, call the webpage from the private browsing window of your business cell phone and no traces are left, "Is my platform supported?" is answered by "If it has a modern browser and network access, most probably yes"
* Controls up to **six outputs at LiPo-battery voltage (3,6-4.2V) with estimated up to 500mA peak** to control any peripherial device by dimming in 1024 steps (though the lower 150 are usually useless for vibrators), by turning it on/off or by a short pulse. Limitations are given by the ULN2003A, the battery used and the battery charge/control circuit (usually 3A max overall).
* **User management** enables you to handle three or four different user types: "admin"s can do everything, "basic" can view the current state of the module and control the outputs, "viewer"s can only see the current state of the module. The fourth user group would be unauthorized/not logged in users. Each user is identified by the name and has a role and a password he can change.
* Supports many **custom sequences for PWM outputs** Each sequence is a file in the filesystem with a simple and easy format, available sequence files are autodetected on boot. Name these files as you want the sequences to be shown in the drop down menus.
* Runs at least for **13 hours idling** (WIFI acesspoint running, one client connected, simultaniously connected to a WIFI network, a few website requests, GPS off) with a 3,7V 1200mAh LiPo battery (uses approx. 90mA in idle). Without a client connected to the accesspoint, it runs over 14 hours. It **lasts longer than 30 hours** idling when it does not provide an accesspoint and GPS is off. Charge it for approximately 1.5 hours at 5V, 1A and you are good to go again. GPS enabled draws approximately additional 50mA, power draw on the outputs depends on the connected devise and the PWM ratio used.
* Turns on on 1 sec button press, you can **turn it off on the website only when logged in as admin** - or customize the hardware by using a switch to turn it on and off.
* **Local file server** to upload and manage the files on the module
* **Custom webapp possible** since the module serves any web page or web application you upload and configure (any file named "public[...]" will be served without login-checks). The web app provided includes a custom file to provide information about the module or the wearer.
* See the **GPS position**, or disable GPS for power saving
* **Firmware update via upload** - upload the desired firmware binary file, then provide the hash sum for the device to check the file before updating.
* "Turn all off" as some kind of "emergency off"
* **86x51x21,5mm** in size - smaller than a pack of cigaretts. Depending on your skills and the features you want your module to have, you can build it in different cases.
* **Status page** that shows you (roughly!) the current battery voltage and lets you turn off the module (with a voltage divider of 1.5k Ohm to 4.7k Ohm and a multiplication by 4 (as shown/done in the source files) below a measured supply voltage of 3000mV the battery will drain quickly (10-15 minutes remain with a 1.2Ah LiPo cell)
* **Many options** to configure the device behaviour.
* **Access it in the local network** at its IP, http://[your configured servername here] or its default "ESP-XXXXX" name if the registering of the host name did not work, or http://192.168.4.1 if your device is connected to the modules' accesspoint. If it has no other network access, ANY http://XXX url will lead to the module.
* **Configurable to not answer or show a dummy/decoy page on a request to [hostname]/ or [ModuleIP]/** so if someone gets the IP from the router he will not be presented the main page on / - [hostname]/webApp or [ModuleIP]/webApp always shows the main page.
* Sockets provide ground, battery voltage and the signal (on = tied to ground, max 0.5A each, max ~3A alltogether until the battery protection turns off power. off = no potential) when the module is turned on, so you can permanently **power external modules/cirquits if needed**, or **run a motor directly** between the signal line and the battery voltage line
* Charge it with a common **micro USB** cable
* **Noncommercially open source** - details see Licence.txt (tl;dr: No warranty on anything, use at your own risk. If you want to sell it, don't charge for the firmware or devvelopment)
* Material cost: **approximately 18€** for me when buying the stuff in China.
* **Relatively easy to build, most parts are widely available and replaceably easy to use modules or parts**, the connectors are common 3.5mm stereo jacks
* Any 3-4V vibrating device with a remote control on a cable can easily be converted to fit this module (if you solder a socket to the original remote control, you can still use the device with the original remote control)
* First time you flashed your module? If you use linux, use the provided upload script for initial file upload (Webapp files that make it more comfortable to configure the first user and uploads demo sequence files as well) or adapt it to a powershell script.
* If no admin user is configured, any login credentials are acceppted until an admin account is created (useful for initial setup, a shellscript for uploading the needed files in a Linux environments is provided)
* Rudimentary debug output on serial TX line in case weird behaviour is seen.

### Disadvantages:
* It sometimes takes a while until the router knows the device by its configured name, so it might take some time until you can access it at http://[your configured servername here]
* The larger a file is, the longer it takes to load. So keep your web app small.
* Known bug: many sequences at the same time with many changes per second lead to a reset after some time, probably the watchdog bites.

### Minor "not as perfect as it could be" properties:
* the ESP8266 Module does only support at maximum 4 client devices connected to its WIFI accesspoint
* The requests to the module are unencrypted HTTP-POST requests - no ssl encryption possible
* Web app only optimized for mobile phone screen size
* No hashing of user and WIFI passwords (plaintext passwords could be read by flashing a custom firmware that reads all files)
* Remote controlling via requests to a remote server url do not work any more since the library handling HTTPS requests changed
* Rules do not work yet, so geofencing and other things like reacting on low or full battery states is not possible
* File upload not perfectly secure and stable, but worked 99% while testing and usage.
* Sometimes the hash function of the device does not provide the same hash value as the md5-hash on a computer. This might be due to different Filesystems. If in doubt: listen on the serial TX pin, there is some debug output.


## Material used for this "full feature" Model:
* ESP8266 module ESP-12 (or ESP-12E / ESP-12F) 
* TP4056 micro-USB LiPo charging board with overcurrent and undervoltage protection (not just safe charging but safe discharging as well)
* ULN2003A chip or module for driving the outputs
* AMS1117 3.3V 800mA voltage regulator (yes, the cheap ones might only reach 2.9V on a close-to-empty battery, but the module runs even with 2.4V, so any cheap one will suffice)
* 3,7V single cell LiPo battery (without any further protection circuit. I used 62,8mm x 30,3mm x 7,3mm 1200mAh)
* ABS plastic box, I used 86x50x21,5mm (or 85x50x21,5mm according to other vendors) or similar
* Button (push = on, else = off), e.g. a self adhesive foil button which can be fittet anywhere on the outside of the chassis
* 6 sockets for 3.5mm stereo audio jacks - use the threaded ones, they are better for glueing to the chassis and can be screwed to the chassis when you drill the outer half of the mounting hole with a larger drill. Optional if you hardwire your devices.
* Resistors (7x 4,7kOhm, 2x 1,5kOhm)
* Cables (a few thicker ones for the main power, thinner ones for the signals)
* 2 N-Channel (MOS)FET transistor (I used AOD407, overkill but do the job. One needed if no GPS module is used)
* Additionally you might want dust covers for the micro-USB port and the sockets, a "prototyping" pcb (no breadboard) and shrinking tube might be useful as well
* a GPS module, e.g. uBlox NEO6 or NEO7 (can be spared if you don't want GPS)
* For building a small bullet vibrator, a common mobile phone vibration motor, some case and a 2 or three wire cable featuring a 3.5mm stereo audio jack (if you use a common audio cable, red and white / left and right are battery voltage and Signal).
* Or use an existing cable-controlled device, cut it's cable and add a 3.5mm stereo audio jack (use tip and mid terminals). Or hardwire the device to the module by soldering the wires to the ULN2003A.

### Tools needed for the module as shown above:
* A soldering iron and some solder wire
* A side cutter or strong scissors for cutting wires, a wire stripper might be handy, too.
* An electric drilling machine (a vertical drilling machine is even more useful)
* A 5mm steel drill for drilling the holes for the 3,5mm sockets to stick through the case, optionally an 8mm steel drill to widen the front of the holes to screw the ring shaped nuts to the sockets
* A 4mm drill and/or milling cutter for drilling the holes for the micro-USB socket (multiple holes next to each other becoming a slot)
* A small file for cleaning the hole for the micro-USB socket
* Waterproof glue and hot glue or similar, to glue the components safely together and to the case (two component resin or silicon might work as well, hot glue does not stick too good). Be careful with the USB and audio sockets, especioally if the glue likes to creep in gaps.
* Something to measure and mark where to drill the holes in the case
* A USB to serial converter with 3.3V signal level to initially flash the ESP8266
* A spare wire to temporarily connect GPIO 0 to ground while bootup to enter flash mode



**Development environment**: Arduino  
**Libraries used** (might not be a complete list): Adafruit ESP8266, esp8266_mDNS, NTPClient, ArduinoOTA. These are: Arduino standard libraries for ESP8266, ESP8266WiFi.h, ESP8266WebServer.h, FS.h, ESP8266mDNS.h NTPClient.h, WiFiUDP.h, DNSServer.h, ArduinoOTA.h 


#### Notes ####
This project uses a slightly modified version of the NTP client library to set the time using GPS if available. Changes done can be seen in customchanges_NTPClient.txt  
Don't use GPS anyway? Disable it simply by replacing the code in setTimeFromGPS() by "return;" to make it return immediately, this way no changes need to be made in NTP client library.  
To enter boot mode create a temporary connection via a spare wire between GPIO 0 and ground ("-"), then press the button and keep it pressed to enter flash mode. The button needs to be pressed until flashing is done since the not yet flashed firmware needs to configure GPIO2 to bypass the button providing power.  

### Disclaimer and License ###
This project is a hobby project thet grew over time, I cannot guarantee anything but that I had no bad intentions when writing this (does not do damage on my purpose), but it's "as is", use at your own risk, there are flaws and even though I tested it regularly there are probably bugs in it.

License see License.txt, rights to and license of the ractive.js library used in the demo web app see https://github.com/ractivejs/ractive and the according LICENSE.md file.
