Protocol information for TPE-Modul v3

HTML paths:

/ sends depending on the configuration no page: the webApp as stated below, a "decoy"/dummy page "public[something]", or nothing

/webApp sends webApp html file "public[something]" as configured, default: "publicWebApp.html" or error page if the file is not found in the file system.

/info sends info "page" (the one without surrounding <html></html>) as configured.

/public[something] sends file public[something] if found in file system, else, depending on configuration, an error message or no response


Interface paths for POST/GET parameters if not stated otherwise, returning data in JSON format if not stated otherwise:

/api/control Get the current status about outputs and control the status of outputs. (Get values: all logged in, set values: Control and Admin only)
Parameters:
user=[username]
token=[sessionToken]
alloff=1 // optional, turns all outputs off, the rest of the message will be ignored
pin[1-6]=[0-1023]/[0-1]/1 // optional, set PWM of pin [1-6] to [0-1023] / set toggle pin [1-6] to [off/on] / pulse pin [1-6]
pwm[1-6]="[sequenceNameString]" // optional, set and start sequence [sequenceNameString] on pwm pin [1-6]

Returns: HTTP/OK and JSON
{
    "outputs": [ // list of all 6 outputs
            {
                "name":"[name]",
                "id":[1-6],
                "type":"[p/t/b/n]", // p=pm, t=toggle, b=pulse, n=deactivated, n/a
                "value":[0-1023/0-1], // only existing when type=p or t
                "seq":"[sequencename]" // only existing when type=p, no sequence = "-"
            },
            [...]
        ],
    "sequences": ["sequencename1", [...], "sequencnameN"] // list of all available sequences (addressed by filename)
}
or HTTP/UNAUTHORIZED


/api/status Get basic module status (battery voltage, WIFI status, system time, GPS position if available, etc.) (logged in only)
Parameters:
user=[username]
token=[sessionToken]

Returns: HTTP/OK and JSON
{
    "vbatmv":[battery voltage in mV],
    "ssid":"[SSID connected to]", // "-" if not connected
    "ip":"[IP], // if configured to connect to any WIFI AP
    "hostname":"[hostname]", // if configured to connect to any WIFI AP
    "gatewayip":"[gateay IP]", // if configured to connect to any WIFI AP
    "dnsip":"[DNS IP]", // if configured to connect to any WIFI AP
    "apssid":"[AP SSID]", // if configured to open a standalone AP
    "apip":"[AP IP]", // if configured to open a standalone AP
    "gpsstate":"[enabled/disabled]",
    "lastgpsdate":"[date of last fix]", // if GPS enabled
    "lastgpstime":"[time of last fix]", // if GPS enabled
    "posfix":[0,1,2,6], // if GPS enabled. 0 = no fix, 1 = 2D/3D fix, 2 = differential, 6 = estimate
    "freeheap":[number of bytes free], // basically "free RAM"
    "moduletime":[timestamp] // unix timestamp of the modules time (set via GPS/NTP)
}
or HTTP/UNAUTHORIZED


/api/gps Get detailed GPS data (position, altitude, speed, movement direction, number of satellites available/used) (logged in only)
Parameters:
user=[username]
token=[sessionToken]

Returns: HTTP/OK and JSON
{
    "gpsstate":"[enabled/disabled]",
    "lastgpsdate":"[date of last fix]", // if GPS enabled
    "lastgpstime":"[time of last fix]", // if GPS enabled
    "lon":"[longitude]",
    "lat":"[latitude]",
    "elev":"[elevation]",
    "gndspeed":"[ground speed in km/h]",
    "course":"[course]", // course on ground in degree
    "hordil":"[estimated horizontal dilution in m]",
    "numsat":"[number of satellites used]",
    "posfix":[0,1,2,6] // if GPS enabled. 0 = no fix, 1 = 2D/3D fix, 2 = differential, 6 = estimate
}
or HTTP/UNAUTHORIZED


/api/login login. Who would have guessed.
Parameters:
user=[username]
pwd=[password]

Returns: HTTP/OK and JSON
{
    "token":"[token]",
    "role":"[v/c/a]", // viewer, guest controller, admin
    "title":"[configured web page title]"
}
or HTTP/UNAUTHORIZED


/api/logout logout. Who would have guessed.
Parameters:
user=[username]
token=[sessionToken]
Returns: HTTP/OK
or HTTP/UNAUTHORIZED // e.g. if username and/or token did not match


/api/updatepwd password change. TODO: Needs to be reworked a bit to return consistent "success":[0/1]
Parameters:
user=[username]
token=[sessionToken]
pwd=[currentPassword]
newPwd=[newpassword]

Returns: HTTP/OK and JSON
{
    "success":[0/1]
}
or HTTP/BADREQUEST and JSON
{
    "success":"false",
    "reason":"[emptyString/argMissing]"
}
or HTTP/UNAUTHORIZED


/api/adminstatus get a detailed overview about the module status (admin only)
Parameters:
user=[username]
token=[sessionToken]

Returns: HTTP/OK and JSON
{
    "vbatmv":[battery voltage in mV],
    "ssid":"[SSID connected to]", // "-" if not connected
    "ip":"[IP], // if configured to connect to any WIFI AP
    "hostname":"[hostname]", // if configured to connect to any WIFI AP
    "gatewayip":"[gateay IP]", // if configured to connect to any WIFI AP
    "dnsip":"[DNS IP]", // if configured to connect to any WIFI AP
    "apssid":"[AP SSID]", // if configured to open a standalone AP
    "apip":"[AP IP]", // if configured to open a standalone AP
    "chipid":"[chip ID]",
    "freeheap":[number of bytes free], // basically "free RAM"
    "useddisk":[bytes used of file system],
    "totaldisk":[total size of file system in byte],    
    "gpsstate":"[enabled/disabled]",
    "lastgpsdate":"[date of last fix]", // if GPS enabled
    "lastgpstime":"[time of last fix]", // if GPS enabled
    "posfix":[0,1,2,6], // if GPS enabled. 0 = no fix, 1 = 2D/3D fix, 2 = differential, 6 = estimate
    "moduletime":[timestamp], // unix timestamp of the modules time (set via GPS/NTP)
    "fwversion":"[firmware version]"
}
or HTTP/UNAUTHORIZED


/api/adminoutputs see and set the descriptive names and functionality of the outputs (admin only)
Parameters:
user=[username]
token=[sessionToken]
pinname[1-6]=[name] // optional, set name of output[1-6]
pinfunction[1-6]=[p/t/b/n] // optional, set function of output[1-6]

Returns: HTTP/OK and JSON
{
    "outputs":[
        {
            "name":"[name],
            "id":[1-6],
            "type":"[p/t/b/n]"
        }
        [...]
    ]
}
or HTTP/UNAUTHORIZED


/api/adminmodule See and set the base module configuration (remote view/control server parameters, firmware update, main files) (admin only)
Parameters:
user=[username]
token=[sessionToken]
serverurl=[url] // optional, set url of server to connect to
serverport=[port number] // optional, set port of server to connect to
fwupdatefile=[filename] // optional, update firmware to this file stored locally
fwmd5=[md5 sum of the firmware file] // optional
certfingerprint=[server cert fingerprint] // optional, so https can be used to connect to the remote server (no local cert chain matching at the time)
servermode=[c/s/n] // optional, control / supervise / no connection to a server
serverinterval=[number of seconds] // optional, interval in sec to connect to the remote server
infofile=[filename] // optional, file to serve on /info
hidesite=[true/false] // optional, don't serve anything on path /
dummyfile=[filename] // optional, file to serve as dummy on path /
showdummyfile=[true/false] // optional, show the dummy file on path / (if / not hidden, aka hidesite was false)
pagetietle=[tilte, percent-encoded!] // optional, title to be served on login. %-encode it! there might be special characters, UTF8 does not work, etc.
gpsmode=[0/1/2] // optional, 0 = off, 1= on, 2 = Not Implemented Yet once a day for timestamp sync (battery saving)
ntpserver=[ntpserverurl] // optional, NTP server to use if GPS is disabled
reset=1 // optional, reboot device
moduleoff=1 // optional, shut down the module

Returns: HTTP/OK and JSON
{
    "updatestatus":[0/1], // optional if firmware update was requested
    "serverrcmode":[0/1], // may the server control the module?
    "connect":[0/1], // create connection to the server?
    "serverurl":"[server url]",
    "serverport":[serverport],
    "intervalms":[server connection interval in ms],
    "certfingerprint":[fingerprint],
    "infofile":[filename],
    "hidesite":[0/1],
    "dummyfile":"[filename]",
    "showdummyfile":[0/1],
    "title":"[title]",
    "ntpserver":"[url]",
    "gpsUsage":"[0/1/2]",
    "changesapplied":[1-n] // optional, number of changes applied
    "action":"[restart/off]" // optional, if restart/shutdown was requested. Usually the reboot/shutdown cancels sending the message
}
or HTTP/UNAUTHORIZED


/api/adminwifi configure WIFI  (admin only)
Parameters:
user=[username]
token=[sessionToken]
forgetwifi=[ssid] // optional, wifi information to be deleted (ssid/password pair will be deleted from the device)
apssid=[ssid] // optional, if the module AP shall be configured
appwd=[password] // optional, if the module AP shall be configured
enifiap=[0/1] // optional, disable/enable the WIFI accesspoint of the module
enwific=[0/1] // optional, disable/enable if the module should connect to other wifi accesspoints
ssid=[ssid] // optional, to set the SSID of the credentials of a WIFI network to connect to
wifipwd=[password] // optional, to set the password of the credentials of a WIFI network to connect to
hname=[hostname] // optional, the hostname to use when connecting to a WIFI network

Returns: HTTP/OK and JSON
{
    "apssid":"[AP ssid]",
    "wifimode":[1/2/3], // Client/AP/both
    "numificlients":[number of clients connected to module AP],
    "hostname":"[hostname]",
    "knownWifis":["[SSID1]",[...],"[SSIDn]"]
}
of HTTP/UNAUTHORIZED


/api/adminrules Not Implemented Yet. Will send the configured rules and accept data for adding, reconfiguring or removing rules (e.g. geofencing).
Parameters:
user=[username]
token=[sessionToken]
// more to come

/api/adminusers configure users. Sends a list of users (Name and role: viewer/controller/admin) (admin only)
Parameters:
user=[username]
token=[sessionToken]
newUser=[username] // optional
userPwd=[password] // optional
userRole=[v/c/a] // optional
delUser=[username] // optional
updateUser=[username] // optional, used to update the role of a user (parameter role mandatory!)
role=[v/c/a] // optional, used for updateUser

Returns: HTTP/OK or HTTP/CONFLICT (e.g. if a new user already existed or one to remove not) and JSON
{
    "users":
    [
        {
            "name":"[name]",
            "role":"[v/c/a]"
        },
        [...]
    ]
}
or HTTP/UNAUTHORIZED



/api/files List and manage files, fileupload as seen in the webApp (admin only)
Parameters:
user=[username]
token=[sessionToken]
del_[0-n]="[filenameToDelete]"

Returns: HTTP/OK and JSON
{
    "totaldisk":[bytes],
    "useddisk":[bytes],
    "deletedfiles":["filename1",...,"filenameN"],
    "files":
    [
        {
            "name":"[filename]",
            "size":[filesize in byte]
        },
        [...]
    ]
}
or HTTP/INSUFFICHIENTSTORAGE and JSON
{"freedisk":[free space in SPIFFS in byte]}
or HTTP/UNAUTHORIZED
