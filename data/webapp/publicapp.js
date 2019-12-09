
// username
var name = "";
var encodedName = encodeURIComponent(name);
var role = ""; // "v"/"u"/"a" (view=watch/ user=+control / admin=+administrate)
var token = ""; // session token
var title = ""; // web page title (set after login)

// TODO: show button for visitors without login

var apiUrls = {
  info: "/info",
  base: "/api",
  login: "/login",
  logout: "/logout",
  control: "/control",
  gps: "/gps",
  status: "/status",
  updatepwd: "/updatepwd",
  adminstatus: "/adminstatus",
  adminoutputs: "/adminoutputs",
  adminmodule: "/adminmodule",
  adminwifi: "/adminwifi",
  admingps: "/admingps",
  adminrules: "/adminrules",
  adminusers: "/adminusers",
  files: "/files"
}

// initialize page on load
document.addEventListener("DOMContentLoaded", function(event) {
  showLoginScreen();
});

// ##############################################
// functions
// ##############################################

function showLoginScreen(){
  // show login screen
  var menuRactive = new Ractive({
    el: '#header',
    template: '#loginscreen',
    data: { }
  });
  // show no other content
  var contRactive = new Ractive({
    el: '#content',
    template: '',
    data: {
    }
  });
  document.getElementById("feedback").innerHTML = "";
}

function showMenue(){
  // show admin menu if the users role is admin
  if(role == "a"){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#menuadmin',
      data: {
        name: name,
        title: title
      }
    });
  // else show base menu only
}else if((role == "b") || (role == "v")){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#menubase',
      data: {
        name: name,
        title: title
      }
    });
  }else{
    var menuRactive = new Ractive({
      el: '#header',
      template: '#menunouser',
      data: {
        name: name,
        title: title
      }
    });
  }
}


function showControl(){
  showMenue();

  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.control,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("control response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // parse answer to json object
                      var dataObj = JSON.parse(xhttp.responseText);
                      dataObj.role = role;
                      var contRactive = new Ractive({
                          el: '#content',
                          template: '#control',
                          data: dataObj
                        });

                      // on HTTPUNAUTHORIZED, the credentials given were incorrect
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showLoginScreen();
                        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                      }else{
                        console.log("Error sending showControl request.");
                        // TODO: better feedback
                      }
                  }
  );
}

function showStatus(){

  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);


  sendPostRequest(apiUrls.base + apiUrls.status,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("status response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // parse answer to json object
                      var dataObj = JSON.parse(xhttp.responseText);

                      // convert battery voltage from mV to V
                      var vbat = dataObj.vbatmv / 1000;
                      dataObj.vbat = vbat;
                      dataObj.vpercent = calculateVpercent(dataObj.vbat);
                      if(dataObj.gpsstate == "enabled"){
                        dataObj.lastgpsfix = dataObj.lastgpsdate.substr(0, 2);
                        dataObj.lastgpsfix += ".";
                        dataObj.lastgpsfix += dataObj.lastgpsdate.substr(2, 2);
                        dataObj.lastgpsfix += ".20";
                        dataObj.lastgpsfix += dataObj.lastgpsdate.substr(4, 2);
                        dataObj.lastgpsfix += " ";
                        dataObj.lastgpsfix += dataObj.lastgpstime.substr(0, 2);
                        dataObj.lastgpsfix += ":";
                        dataObj.lastgpsfix += dataObj.lastgpstime.substr(2, 2);
                        dataObj.lastgpsfix += ":";
                        dataObj.lastgpsfix += dataObj.lastgpstime.substr(4, 2);
                      }

                      var date = new Date(0);
                      date.setUTCSeconds(dataObj.moduletime);
                      dataObj.moduleyear = date.getFullYear();
                      dataObj.modulemonth = date.getMonth()+1;
                      dataObj.moduleday = date.getDate();
                      dataObj.modulehours = date.getHours();
                      dataObj.moduleminutes = (date.getMinutes() < 10 ? "0" : "") + date.getMinutes();
                      dataObj.moduleseconds = date.getSeconds();

                      var contRactive = new Ractive({
                        el: '#content',
                        template: '#status',
                        data: dataObj
                      });
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showLoginScreen();
                      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                    }else{
                      console.log("Error sending showStatus request.");
                      // TODO: better feedback
                    }
                  }
  );
}


function showInfo(){
  // TODO: pull data
  //var infohtml = "<h2>Slave information:</h2><br>Slave is owned and collared by Master.<br><img src='publicslavepic.jpg' alt='image of my slave'/>";


  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.info,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("info response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      var contRactive = new Ractive({
                          el: '#content',
                          // no data, plain html
                          template: xhttp.responseText
                        });
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect
                    }else if (status == 0){
                      showLoginScreen();
                      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                    }else{
                      console.log("Error fetching info data");
                      // TODO: better feedback
                    }
                  }
  );
}


function showGps(){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.gps,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("gps response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // parse answer to json object
                      var gpsdata = JSON.parse(xhttp.responseText);
                      // create human readable strings from data
                      if(gpsdata.gpsstate == "enabled"){
                        gpsdata.lastgpsfix = gpsdata.lastgpsdate.substr(0, 2);
                        gpsdata.lastgpsfix += ".";
                        gpsdata.lastgpsfix += gpsdata.lastgpsdate.substr(2, 2);
                        gpsdata.lastgpsfix += ".20";
                        gpsdata.lastgpsfix += gpsdata.lastgpsdate.substr(4, 2);
                        gpsdata.lastgpsfix += " ";
                        gpsdata.lastgpsfix += gpsdata.lastgpstime.substr(0, 2);
                        gpsdata.lastgpsfix += ":";
                        gpsdata.lastgpsfix += gpsdata.lastgpstime.substr(2, 2);
                        gpsdata.lastgpsfix += " ";
                        gpsdata.lastgpsfix += gpsdata.lastgpstime.substr(4, 2);

                        /*if(gpsdata.posfix == 0){
                          gpsdata.lastGpsFix = "N/A";
                          gpsdata.lastTimeFix = "N/A";
                          gpsdata.latpos = "N/A";
                          gpsdata.lonpos = "N/A";
                        }else{*/
                          gpsdata.lastGpsFix = buildDateTimeString(gpsdata.lastgpsdate, gpsdata.lastgpstime);
                          // create custom format of lat + lon for usage in google maps link
                          var latpos = gpsdata.lat;
                          var lonpos = gpsdata.lon;
                          latpos.replace(" ", "%20");
                          lonpos.replace(" ", "%20");
                          gpsdata.latpos = latpos;
                          gpsdata.lonpos = lonpos;
                          // split on the space => (d)dd and mm.mmmm[N/S/E/W]
                          var latParts = gpsdata.lat.split(" ");
                          var lonParts = gpsdata.lon.split(" ");

                          // S/W are negative, N/E positive
                          gpsdata.mlatpos = 1;
                          gpsdata.mlonpos = 1;
                          // so check for S/W and make sure to clean the strings of non-decimal characters
                          if(latParts[1].indexOf("S") > 2){
                            gpsdata.mlatpos = -1;
                            latParts[1] = latParts[1].replace("S", "");
                          }else{
                            latParts[1] = latParts[1].replace("N", "");
                          }
                          if(lonParts[1].indexOf("W") > 2){
                            gpsdata.mlonpos = -1;
                            lonParts[1] = lonParts[1].replace("W", "");
                          }else{
                            lonParts[1] = lonParts[1].replace("E", "");
                          }
                          gpsdata.mlatpos = gpsdata.mlatpos * (parseFloat(latParts[0]) + (parseFloat(latParts[1]/60)));
                          gpsdata.mlonpos = gpsdata.mlonpos * (parseFloat(lonParts[0]) + (parseFloat(lonParts[1]/60)));
                        /*}*/
                      }

                      var contRactive = new Ractive({
                          el: '#content',
                          template: '#gps',
                          data: {
                            gps: gpsdata
                          }
                        });
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect
                    }else if (status == 0){
                      showLoginScreen();
                      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                    }else{
                      console.log("Error fetching GPS data");
                      // TODO: better feedback
                    }
                  }
  );
}

function showAccount(){
  // check role
  if(role == ""){
    return;
  }

  var data = {};
  data.name = name;
  data.role = role;
  data.roles = [{name: "Admin", id: "a"},
      {name: "Basic", id: "b"},
      {name: "View", id: "v"}]

  var contRactive = new Ractive({
      el: '#content',
      template: '#account',
      data: data
    });
}

function showAdminStatus(){
  // show admin page if the users role is admin, else do nothing
  if(role == "a"){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#adminmenu',
      data: {
        name: name,
        title: title
      }
    });
    var formData = new FormData();
    formData.append("user", encodedName);
    formData.append("token", token);

    sendPostRequest(apiUrls.base + apiUrls.adminstatus,
                    formData,
                    function(xhttp, status) {
                      console.log("status:");
                      console.log(status);
                      console.log("adminoutputs response:");
                      console.log(xhttp.responseText);
                      // if HTTPOK (credentials were right)
                      if (status == 200 ) {
                        // parse answer to json object
                        var dataObj = JSON.parse(xhttp.responseText);
                        dataObj.filepercent = Math.round((dataObj.useddisk * 10000) / dataObj.totaldisk)/100;
                        dataObj.freedisk = dataObj.totaldisk - dataObj.useddisk;

                        dataObj.vbat = dataObj.vbatmv / 1000;
                        dataObj.vpercent = calculateVpercent(dataObj.vbat);

                        if(dataObj.gpsstate == "enabled"){
                          dataObj.lastgpsfix = dataObj.lastgpsdate.substr(0, 2);
                          dataObj.lastgpsfix += ".";
                          dataObj.lastgpsfix += dataObj.lastgpsdate.substr(2, 2);
                          dataObj.lastgpsfix += ".20";
                          dataObj.lastgpsfix += dataObj.lastgpsdate.substr(4, 2);
                          dataObj.lastgpsfix += " ";
                          dataObj.lastgpsfix += dataObj.lastgpstime.substr(0, 2);
                          dataObj.lastgpsfix += ":";
                          dataObj.lastgpsfix += dataObj.lastgpstime.substr(2, 2);
                          dataObj.lastgpsfix += " ";
                          dataObj.lastgpsfix += dataObj.lastgpstime.substr(4, 2);
                        }

                        var date = new Date(0);
                        date.setUTCSeconds(dataObj.moduletime);
                        dataObj.moduleyear = date.getFullYear();
                        dataObj.modulemonth = date.getMonth()+1;
                        dataObj.moduleday = date.getDate();
                        dataObj.modulehours = date.getHours();
                        dataObj.moduleminutes = (date.getMinutes() < 10 ? "0" : "") + date.getMinutes();
                        dataObj.moduleseconds = date.getSeconds();

                        var contRactive = new Ractive({
                          el: '#content',
                          template: '#adminstatus',
                          data: dataObj
                        });
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showLoginScreen();
                        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                      }else{
                        console.log("Error fetching adminstatus data");
                        // TODO: better feedback
                      }
                    }
    );

  }
}

function showAdminOutputs(){
  // show admin page if the users role is admin, else do nothing
  if(role == "a"){

    var formData = new FormData();
    formData.append("user", encodedName);
    formData.append("token", token);

    sendPostRequest(apiUrls.base + apiUrls.adminoutputs,
                    formData,
                    function(xhttp, status) {
                      console.log("status:");
                      console.log(status);
                      console.log("adminoutputs response:");
                      console.log(xhttp.responseText);
                      // if HTTPOK (credentials were right)
                      if (status == 200 ) {
                        // parse answer to json object
                        var dataObj = JSON.parse(xhttp.responseText);

                        var contRactive = new Ractive({
                            el: '#content',
                            template: '#adminoutputs',
                            data: dataObj
                          });
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showLoginScreen();
                        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                      }else{
                        console.log("Error fetching adminoutputs data");
                        // TODO: better feedback
                      }
                    }
    );
  }
}


function showAdminModule(){
  // show admin page if the users role is admin, else do nothing
  if(role == "a"){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#adminmenu',
      data: {
        name: name,
        title: title
      }
    });

    var formData = new FormData();
    formData.append("user", encodedName);
    formData.append("token", token);

    sendPostRequest(apiUrls.base + apiUrls.adminmodule,
                    formData,
                    function(xhttp, status){
                      console.log(status);
                      console.log("adminmodule response:");
                      console.log(xhttp.responseText);
                      if(status == 200){
                        if(xhttp.responseText == ""){
                          // TODO: show information for failed request
                          return;
                        }
                        var data = JSON.parse(xhttp.responseText);
                        data.serverinterval = data.intervalms / 1000;
                        if((data.serverrcmode == 1) && (data.connect == 1)){
                          data.servermode = "c";
                        }else if(data.connect == 1){
                          data.servermode = "s";
                        }else{
                          data.servermode = "n";
                        }

                        var contRactive = new Ractive({
                            el: '#content',
                            template: '#adminmodule',
                            data: data
                          });
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showLoginScreen();
                        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                      }else{
                        console.log("Error fetching adminmodule data");
                        // TODO: better feedback
                      }
                    }
    );

  }
}

function showAdminWifi(){
  // show admin page if the users role is admin, else do nothing
  if(role == "a"){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#adminmenu',
      data: {
        name: name,
        title: title
      }
    });

    var formData = new FormData();
    formData.append("user", encodedName);
    formData.append("token", token);

    sendPostRequest(apiUrls.base + apiUrls.adminwifi,
                    formData,
                    function(xhttp, status){
                      console.log(status);
                      console.log("adminwifi response:");
                      console.log(xhttp.responseText);
                      if(status == 200){
                        if(xhttp.responseText == ""){
                          // TODO: show information for failed request
                          return;
                        }
                        var data = JSON.parse(xhttp.responseText);

                        data.apon = false;
                        data.wifion = false;
                        if((data.wifimode == 1) || (data.wifimode == 3)){
                          data.wifion = true;
                        }
                        if(data.wifimode > 1){
                          data.apon = true;
                        }
                        // TODO: Mode 4: AP only whenno wifi available

                        var contRactive = new Ractive({
                            el: '#content',
                            template: '#adminwifi',
                            data: data
                          });
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showLoginScreen();
                        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                      }else{
                        console.log("Error fetching adminwifi data");
                        // TODO: better feedback
                      }
                    }
    );
  }
}


function showAdminFiles(){
  // show admin page if the users role is admin, else do nothing
  if(role == "a"){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#adminmenu',
      data: {
        name: name,
        title: title
      }
    });

    var formData = new FormData();
    formData.append("user", encodedName);
    formData.append("token", token);

    sendPostRequest(apiUrls.base + apiUrls.files, formData, function(xhttp, status){
                    console.log("status:");
                    console.log(status);
                    console.log("adminfiles response:");
                    console.log(xhttp.responseText);
                    if(status == 200){
                      if(xhttp.responseText == ""){
                        // TODO: show information for failed request
                        return;
                      }
                      var data = JSON.parse(xhttp.responseText);
                      data.filepercent = Math.round((data.useddisk * 10000) / data.totaldisk)/100;
                      data.freedisk = data.totaldisk - data.useddisk;
                      // username and token for file links (might need to contin login credentials)
                      data.username = name;
                      data.token = token;
                      var contRactive = new Ractive({
                          el: '#content',
                          template: '#adminfiles',
                          data: data
                        });
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showLoginScreen();
                      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                    }else{
                      console.log("Error fetching adminfiles data");
                      // TODO: better feedback
                    }
                  }
    );
  }
}


function showAdminUsers(){
  if(role == "a"){
    var menuRactive = new Ractive({
      el: '#header',
      template: '#adminmenu',
      data: {
        name: name,
        title: title
      }
    });
    var formData = new FormData();
    formData.append("user", encodedName);
    formData.append("token", token);

    sendPostRequest(apiUrls.base + apiUrls.adminusers,
                    formData,
                    function(xhttp, status){
                      console.log("status:");
                      console.log(status);
                      console.log("adminusers response:");
                      console.log(xhttp.responseText);
                    if(status == 200){
                      if(xhttp.responseText == ""){
                        // TODO: show information for failed request
                        return;
                      }
                      var data = JSON.parse(xhttp.responseText);

                      data.roles = [{"name": "Admin", "id": "a"},
                                     {"name": "Basic", "id": "b"},
                                     {"name": "Viewer", "id": "v"}];

                      var contRactive = new Ractive({
                          el: '#content',
                          template: '#adminusers',
                          data: data
                        });
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showLoginScreen();
                      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                    }else{
                      console.log("Error fetching adminusers data");
                      // TODO: better feedback
                    }
                  }
    );


  }
}


// ###################################
// user routines (login, logout)
// ###################################


// sends login request, saves username to "name" and shows control screen on success
function login(){
  var userName = document.getElementById("name").value;
  var pwd = encodeURIComponent(document.getElementById("pwd").value);
  var formData = new FormData();
  formData.append("user", userName);
  formData.append("pwd", pwd);

  // save name/nick in case of successful login
  name = userName;
  encodedName = encodeURIComponent(name);

  // clear feedback div in case there was a failed login attempt
  document.getElementById("feedback").innerHTML = "";
  // TODO: disable input fields and submit button

  sendPostRequest(apiUrls.base + apiUrls.login,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("login response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // parse answer to json object
                      var dataObj = JSON.parse(xhttp.responseText);
                      // save token and role
                      token = dataObj.token;
                      role = dataObj.role;
                      // safe and set new page title
                      title = dataObj.title;
                      document.title = title;
                      // show control page
                      showControl();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect
                    }else if (status == 401){
                      document.getElementById("feedback").innerHTML = "Username and Password wrong";
                    }else if (status == 0){
                      document.getElementById("feedback").innerHTML = "Login request timeout.";
                    }else{
                      document.getElementById("feedback").innerHTML = "Something went wrong. Response status: " + String(status);
                    }
                  }
  );
}

// sends login request, saves username to "name" and shows control screen on success
function logout(){
  // request for sending logout
  var xhttp = new XMLHttpRequest();
  var formData = new FormData();
  //var params = "user=" + name +"&token=" + token;
  formData.append("token", token);
  formData.append("user", encodedName);
  document.getElementById("feedback").innerHTML = "";

  sendPostRequest(apiUrls.base + apiUrls.logout,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("logout response:");
                    console.log(xhttp.responseText);
                    // show login screen. either because logout successful or allready logged out for some reason
                    showLoginScreen();
                    // if HTTPNOCONTENT ("HTTPOK but no content")
                    if (status == 200 ) {
                      console.log("logged out");
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect
                    }else if (status == 401){
                      document.getElementById("feedback").innerHTML = "Token invalid. Already logged out.";
                    }else if (status == 0){
                      document.getElementById("feedback").innerHTML = "Logout request timeout.";
                    }else{
                      document.getElementById("feedback").innerHTML = "Got status " + status;
                    }
                    // "erase" token and user name
                    token = "";
                    name = "";
                    encodedName = "";
                  }
  );
}




// ###################################
// config routines
// ###################################


/**
 * send request to add a new entry to the list of known wifis
 */
function addWifi(){
  var formData = new FormData;
  formData.append("ssid", encodeURIComponent(document.getElementById("newwifissid").value));
  formData.append("wifipwd", encodeURIComponent(document.getElementById("newwifipwd").value));
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.adminwifi,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("Add WiFi response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                      // TODO: show feedback
                      showAdminWifi();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showAdminWifi();
                    }else{
                      console.log("Error retrieving addWifi response.");
                      // TODO: better feedback
                      logout();
                    }
                  }
  );
}


/**
 * send request to add a new entry to the list of known wifis
 */
function forgetWifi(){
  if(document.getElementById("wifitodel").value == ""){
    return;
  }
  var formData = new FormData;
  formData.append("forgetwifi", encodeURIComponent(document.getElementById("wifitodel").value));
  formData.append("user", encodedName);
  formData.append("token", token);


  sendPostRequest(apiUrls.base + apiUrls.adminwifi,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("Add WiFi response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                      // TODO: show feedback instead of reload
                      showAdminWifi();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showAdminWifi();
                    }else{
                      console.log("Error fetching forgetwifi response.");
                      // TODO: better feedback
                      logout();
                    }
                  }
  );
}

/**
 * send request to start WPS
 */
function startWps(){
  var formData = new FormData();
  formData.append("startwps", "1");
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.adminwifi,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("Start WPS response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                      // TODO: show feedback instead of reload
                      showAdminWifi();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showAdminWifi();
                    }else{
                      console.log("Error fetching startwps response.");
                      // TODO: better feedback
                      logout();
                    }
                  }
  );
}

// sets wifi station/client config (on/off)
function setWifiConfig(){
  var formData = new FormData();
  if(document.getElementById("wifion").checked){
    console.log("wifi on");
    formData.append("enwific", "1");
  }else{
    console.log("wifi off");
    formData.append("enwific", "0");
  }
  formData.append("hname", document.getElementById("hname").value);

  formData.append("user", encodedName);
  formData.append("token", token);


  sendPostRequest(apiUrls.base + apiUrls.adminwifi,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("set wifi config response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                      // TODO: show feedback instead of reload
                      showAdminWifi();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showAdminWifi();
                    }else{
                      console.log("Error fetching setWifiConfig response.");
                      // TODO: better feedback
                      logout();
                    }
                  }
  );
}


// sets wifi ap config
function setWifiApConfig(){
  // check for minimal ap password length (8 characters)
  if((document.getElementById("appwd").value != "") && (document.getElementById("appwd").value.length < 8)){
    document.getElementById("apfeedbackdiv").innerHTML = "Password given not long enough";
    return;
  }
  // reset ap feedbackdiv
  document.getElementById("apfeedbackdiv").innerHTML = "";

  var formData = new FormData();
  if(document.getElementById("apon").checked){
    console.log("wifi ap on");
    formData.append("enwifiap", "1");
  }else{
    console.log("wifi ap off");
    formData.append("enwifiap", "0");
  }
  formData.append("apssid", encodeURIComponent(document.getElementById("apssid").value));
  formData.append("appwd", encodeURIComponent(document.getElementById("appwd").value));
  formData.append("user", encodedName);
  formData.append("token", token);


  sendPostRequest(apiUrls.base + apiUrls.adminwifi,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("set wifi ap config response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                      // TODO: show feedback instead of reload
                      showAdminWifi();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showAdminWifi();
                    }else{
                      console.log("Error fetching setWifiApConfig response.");
                      // TODO: better feedback
                      logout();
                    }
                  }
  );
}


/**
 * send request to set module config
 * (server mode and server )
 */
function setModuleConfig(){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  formData.append("servermode", document.getElementById("servermode").value);
  formData.append("pagetitle", document.getElementById("pagetitle").value);

  formData.append("serverurl", encodeURIComponent(document.getElementById("serverurl").value));
  formData.append("serverport", document.getElementById("serverport").value);
  formData.append("serverinterval", document.getElementById("serverinterval").value);
  formData.append("infofile", encodeURIComponent(document.getElementById("infofile").value));
  formData.append("dummyfile", encodeURIComponent(document.getElementById("dummyfile").value));
  formData.append("certfingerprint", encodeURIComponent(document.getElementById("certfingerprint").value));

  formData.append("hidesite", document.getElementById("hidesite").checked);
  formData.append("showdummyfile", document.getElementById("showdummyfile").checked);

  formData.append("gpsmode", document.getElementById("gpsmode").value);
  formData.append("ntpserver", encodeURIComponent(document.getElementById("ntpserver").value));

  if(document.getElementById("fwupdate").checked == true){
    formData.append("fwupdatefile", encodeURIComponent(document.getElementById("fwupdatefile").value));
    formData.append("fwmd5", document.getElementById("fwmd5").value);
  }

  // TODO v1.2: enable/disable setting time using GPS time
  sendPostRequest((apiUrls.base + apiUrls.adminmodule), formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        console.log("ERROR: response empty");
        // "refresh page"
        showAdminModule();
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      data.serverinterval = data.intervalms / 1000;
      if((data.serverrcmode == 1) && (data.connect == 1)){
        data.servermode = "c";
      }else if(data.connect == 1){
        data.servermode = "s";
      }else{
        data.servermode = "n";
      }
      // show content with new data
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminmodule',
          data: data
        });

    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
    }else if (status == 401){
      // TODO feedback
    }else if (status == 0){
      showAdminModule();
    }else{
      console.log("Error fetching setModuleConfig response.");
      // TODO: better feedback
    }
  });
}



function setOutputConfig(){

  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  for(var i=1; i<=6; i++){
    formData.append("pinname" + String(i), encodeURIComponent(document.getElementById("name" + String(i)).value));
    formData.append("pinfunction" + String(i), document.getElementById("type" + String(i)).value);
  }


  sendPostRequest((apiUrls.base + apiUrls.adminoutputs), formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        console.log("ERROR: response empty");
        // "refresh page"
        showAdminOutputs();
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      // show content with new data
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminoutputs',
          data: data
        });
    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
    }else if (status == 401){
      // TODO feedback
    }else if (status == 0){
      showAdminOutputs();
    }else{
      console.log("Error fetching setOutputConfig response.");
      // TODO: better feedback
    }
  });
}

// to be called by dropdown elements of
function updateUsers(users){

  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  var names = "";
  var roles = ""

  for(var user in users){
    if(user.role != document.getElementById(user.name + "role").value){
      names = names + user.name + " ";
      roles = roles + document.getElementById(user.name + "role").value;
    }
  }

  formData.append("updateUsers", names);
  formData.append("roles", roles);

  sendPostRequest((apiUrls.base + apiUrls.adminusers), formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        console.log("ERROR: response empty");
        // "refresh page"
        showAdminUsers();
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      // show content with new data
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminusers',
          data: data
      });
    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
    }else if (status == 401){
      // TODO feedback
    }else if (status == 0){
      showLoginScreen();
      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
    }else{
      console.log("Error fetching updateUsers response.");
      // TODO: better feedback
    }
  });
}

function changePassword(){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  formData.append("pwd", encodeURIComponent(document.getElementById("pwd").value));
  if(document.getElementById("newPwd").value != document.getElementById("newPwd2").value){
    // TODO: feedback to the user, passwords do not match
    document.getElementById("pwdfeedbackdiv").innerHTML = "New passwords don't match.";
    return;
  }
  formData.append("newPwd", encodeURIComponent(document.getElementById("newPwd").value));

  sendPostRequest((apiUrls.base + apiUrls.updatepwd), formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        console.log("ERROR: response empty");
        // "refresh page"
        showAdminUsers();
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      // show content with new data
      var contRactive = new Ractive({
          el: '#content',
          template: '#account',
          data: data
      });
    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
    }else if (status == 401){
      showLoginScreen();
      document.getElementById("feedback").innerHTML = "Unauthorized. Logged out.";
      // TODO feedback
    }else if (status == 0){
      showLoginScreen();
      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
    }else{
      console.log("Error fetching changePassword response");
      // TODO: better feedback
    }
  });
}


function delUser(user){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  formData.append("delUser", encodeURIComponent(user));

  // TODO! show confirm dialogue!

  sendPostRequest((apiUrls.base + apiUrls.adminusers), formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        console.log("ERROR: response empty");
        // "refresh page"
        showAdminUsers();
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      // show content with new data
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminusers',
          data: data
      });
    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
    }else if (status == 401){
      // TODO feedback
    }else if (status == 0){
      showLoginScreen();
      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
    }else{
      console.log("Error fetching deluser response");
      // TODO: better feedback
    }
  });
}


function addUser(){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  formData.append("newUser", encodeURIComponent(document.getElementById("newUser").value));
  formData.append("userPwd", encodeURIComponent(document.getElementById("newUserPwd").value));
  formData.append("userRole", document.getElementById("newUserRole").value);

  sendPostRequest((apiUrls.base + apiUrls.adminusers), formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        console.log("ERROR: response empty");
        // "refresh page"
        showAdminUsers();
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      // show content with new data
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminusers',
          data: data
      });
    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
    }else if (status == 401){
      // TODO feedback
    }else if (status == 0){
      showLoginScreen();
      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
    }else{
      console.log("Error fetching adduser response.");
      // TODO: better feedback
    }
  });
}

// TODO: set new user role

// ###################################
// control routines
// ###################################


// requests setting of pwm
function setOutputPWM(opId){
  // TODO: check for last time of update, so there will not be a request every millisecond!
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  // only send request if it will not be blocked by an activ sequence
  if(document.getElementById("seq"+opId).value == "-" ){
    formData.append("pwm" + opId, document.getElementById("pin"+opId).value);
  }

  sendPostRequest(apiUrls.base + apiUrls.control,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("control (pwm) response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                        // TODO: show feedback

                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin/guest
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showControl();
                      }else{
                        console.log("Error sending pwm data");
                        // TODO: better feedback
                      }
                  }
  );
}

// requests setting of sequence
function setSequence(opId){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  formData.append("pin" + opId, encodeURIComponent(document.getElementById("seq"+opId).value));
  if(document.getElementById("seq"+opId).value == "-"){
    formData.append("pwm" + opId, "-");
  }else{
    document.getElementById("pin"+opId).value = 0;
    // Todo: set shown value to 0 as well
  }


  sendPostRequest(apiUrls.base + apiUrls.control,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("control (sequence) response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                        // TODO: show feedback
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin/guest
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showControl();
                      }else{
                        console.log("Error sending sequence data");
                        // TODO: better feedback
                      }
                  }
  );
}


// requests output toggle
function toggle(opId){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  if(document.getElementById(opId).checked){
    formData.append("pin" + opId, "1");
  }else{
    formData.append("pin" + opId, "0");
  }

  sendPostRequest(apiUrls.base + apiUrls.control,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("control (toggle) response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                        // TODO: show feedback
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin/guest
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showControl();
                      }else{
                        console.log("Error sending toggle output data");
                        // TODO: better feedback
                      }
                  }
  );
}


// requests output pulse
function pulseOp(opId){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  formData.append("pin" + opId, "1");

  sendPostRequest(apiUrls.base + apiUrls.control,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("control (pulse) response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                      // TODO: show feedback
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin/guest
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showControl();
                    }else{
                      console.log("Error sending pulse output data");
                      // TODO: better feedback
                    }
                  }
  );
}

// sends a request for module to turn all outputs off
function allOutputsOff(){
  var formData = new FormData();
  formData.append("alloff", "1");
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.control,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("control (outputs off) response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if(status == 200 ){
                      var dataObj = JSON.parse(xhttp.responseText);
                      var contRactive = new Ractive({
                          el: '#content',
                          template: '#control',
                          data: dataObj
                        });

                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin/guest
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showControl();
                    }else{
                      console.log("Error sending alloutputsoff data");
                      // TODO: better feedback
                    }
                  }
  );
}

// sends a request for module to turn off
function turnoff(){
  var formData = new FormData();
  formData.append("moduleoff", "1");
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.adminmodule,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("adminmodule (turn off) response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                      // no need to parse answer to json object
                        window.getComputedStyle(document.getElementById("turnoffbutton")).backgroundColor = "#660000";
                        showLoginScreen();
                      // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                      }else if (status == 401){
                        // TODO feedback
                      }else if (status == 0){
                        showLoginScreen();
                        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                      }else{
                        console.log("Error retrieving turnoff answer. Logging out.");
                        // TODO: better feedback
                        showLoginScreen();
                      }
                  }
  );
}

// sends a request for module to restart
function restart(){
  var formData = new FormData();
  formData.append("reset", "1");
  formData.append("user", encodedName);
  formData.append("token", token);

  sendPostRequest(apiUrls.base + apiUrls.adminmodule,
                  formData,
                  function(xhttp, status) {
                    console.log("status:");
                    console.log(status);
                    console.log("Reset response:");
                    console.log(xhttp.responseText);
                    // if HTTPOK (credentials were right)
                    if (status == 200 ) {
                        // TODO: throws NoModificationAllowedError!?!?!?
                      window.getComputedStyle(document.getElementById("restartbutton")).backgroundColor = "#660000";
                      // no need to parse answer to json object
                      showLoginScreen();
                    // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
                    }else if (status == 401){
                      // TODO feedback
                    }else if (status == 0){
                      showLoginScreen();
                      document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
                    }else{
                      console.log("Error retrieving restart answer. Logging out.");
                      // TODO: better feedback
                      showLoginScreen();
                    }
                  }
  );
}




// gets file from file chooser and uploads it to the module
function uploadFile(){
  // get file chooser element
  var fileInput = document.getElementById("filechooser");
  // if no file was selected, don't even try to upload anything
  if(!fileInput.files[0]){
    return;
  }
  // create form data object and insert the file and authorization credentials
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  // TODO: not sure if needed...
  formData.append("filename", encodeURIComponent(fileInput.files[0].name));
  formData.append("filesize", fileInput.files[0].size);

  formData.append("file", fileInput.files[0]);

  // create new http request
  var xhttp = new XMLHttpRequest();
  // url where to send the request
  var targetUrl = apiUrls.base + apiUrls.files;

  xhttp.onreadystatechange = function(){
    // on response, execute this functiondocument.getElementById("pin"+opId).value
    if (this.readyState == 4) {
      console.log("File upload done. Status: " + this.status);
      console.log("Response:");
      console.log(xhttp.responseText);
      var data = JSON.parse(xhttp.responseText);
      data.filepercent = Math.round((data.useddisk * 10000) / data.totaldisk)/100;
      data.freedisk = data.totaldisk - data.useddisk;
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminfiles',
          data: data
        });
    }
  }
  // TODO: test and improve upload progress display to progress bar
  xhttp.onprogress = function (e) {
    if (e.lengthComputable) {
        console.log(e.loaded+  " / " + e.total);
        document.getElementById("uploadprogress").innerHTML = e.loaded + "B / " + e.total + "B";
        if(e.loaded >= e.total){
          document.getElementById("uploadprogress").innerHTML = "";
        }
    }
  }
  // open request
  xhttp.open("POST", targetUrl, true);
  // send the request
  xhttp.send(formData);
  //TODO: remove log output:
  console.log("sent file upload request:");
  // Display the key/value pairs
  for (var pair of formData.entries()) {
      console.log(pair[0]+ ', ' + pair[1]);
  }
}

// send request to delete selected files
function deleteFiles(){
  var formData = new FormData();
  formData.append("user", encodedName);
  formData.append("token", token);

  var boxarray = [];
  // get all checkboxes of the current view that are checked
  var checkboxes = document.querySelectorAll('input[type=checkbox]:checked');
  for (var i = 0; i < checkboxes.length; i++) {
    boxarray.push(checkboxes[i].value);
    formData.append("del_" + String(i), checkboxes[i].id);
    console.log("del " + checkboxes[i].id);
  }
  sendPostRequest(apiUrls.base + apiUrls.files, formData, function(xhttp, status){
    console.log("status:");
    console.log(status);
    console.log("control response:");
    console.log(xhttp.responseText);
    if(status == 200){
      if(xhttp.responseText == ""){
        // TODO: show information for failed request
        return;
      }
      var data = JSON.parse(xhttp.responseText);
      data.filepercent = Math.round((data.useddisk * 10000) / data.totaldisk)/100;
      data.freedisk = data.totaldisk - data.useddisk;
      var contRactive = new Ractive({
          el: '#content',
          template: '#adminfiles',
          data: data
        });
        // on HTTPUNAUTHORIZED, the credentials given were incorrect / user no admin
      }else if (status == 401){
        // TODO feedback
      }else if (status == 0){
        showAdminFiles();
        document.getElementById("feedback").innerHTML = "Timeout. Logged out.";
      }else{
        console.log("Error retrieving restart answer. Logging out.");
        // TODO: better feedback
        showLoginScreen();
      }
  });
}

// ###################################
// helper/utility routines
// ###################################

function loginKeyPress(e){
    // if enter is pressed
    if(e.keyCode == 13){
      login();
    }
}

// estimates battery state from voltage for a LiPo-battery
function calculateVpercent(vbat){
  // calculate battrey percentage
  var vpercent = Math.round((-144.8 * vbat*vbat)+(1308.5*vbat) - 2841.5);
  if(vpercent < 0){
    vpercent = 0;
  }else if (vpercent > 100) {
    vpercent = 100;
  }
  return vpercent;
}

// builds human readable string from GPS date and time sting
function buildDateTimeString(date, time){
  var dateTimeString = date.substr(0, 2) + "." + date.substr(2, 2) + ".20" + date.substr(4, 2);
  dateTimeString += " ";
  dateTimeString += time.substr(0, 2);
  dateTimeString += ":";
  dateTimeString += time.substr(2, 2);
  dateTimeString += ":";
  dateTimeString += time.substr(4, 2);
  dateTimeString += time.substr(6); // deciseconds are written .[XX] in input. so no extra dot needed
  return dateTimeString;
}


// Content-type: application/x-www-form-urlencoded
function sendPostRequest(targetUrl, formData, callback){

  // request for sending login data and receiving
  var xhttp = new XMLHttpRequest();

  xhttp.onreadystatechange = function(){
    // on response call callback
    if (this.readyState == 4) {
      // TODO: remove debug output
      console.log(xhttp);
      callback(xhttp, this.status);
    }
  }
  // Display the key/value pairs
  for (var pair of formData) {
      console.log(pair[0]+ ', ' + pair[1]);
  }
  // open request
  xhttp.open("POST", targetUrl, true);
  // send the request
  xhttp.send(formData);
}
