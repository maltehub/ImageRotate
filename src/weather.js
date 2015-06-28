
/**
 * Makes an HTTP request. 
 * 
 * @param url - the URL for the request. It doesn't need to have the params concatenated.
 * @param type - the type: POST or GET (not sure if it will work with PUT and DELETE).
 * @param params - params for the POST or GET request. If no params, then pass null.
 * @param header - the header to include in the request. If no header then pass null.
 *                 It is an array so for a header like: 
 *                 "Content-type", "application/x-www-form-urlencoded; charset=utf-8"
 * 					header = ["Content-type", "application/x-www-form-urlencoded; charset=utf-8"];
 *                  so header[0]  = "Content-type" and header[1] the rest. 
 * @param success - the success function to execute when the request succeeded. 
 * @param error - the error function to execute when the request failed. 
 * 
 * success and error will return responseText as a param in the function.
 */

var xhrRequest = function (url, type, params, header, success, error) {
	console.log("Sending request to: "+url);
	var request = new XMLHttpRequest();
	request.onload = function(){
		if (this.readyState == 4 && this.status == 200) {
	        console.log("Success: "+ this.responseText);
	        success(this.responseText);
		}
		else {
			console.log("Error: "+this.responseText);
			error(this.responseText);
		}
	};

	if (header != null) {
		console.log("Header found: "+ header[0] + " : "+ header[1]);
		request.setRequestHeader(header[0], header[1]);
	}

	paramsString = "";
	if (params != null) {
		for (var i = 0; i < params.length; i++) {
			paramsString += params[i];
			if (i < params.length -1){
				paramsString += "&";
			}
		};
	}
	if (type == 'POST') {
		request.open(type, url, true);
		console.log("paramsString : "+paramsString);
		request.send(paramsString);
	}
	else {
		if (paramsString != ""){
			url += "?" + paramsString;
		}
		request.open(type, url);
		request.send();
	}
}

var db = window.localStorage;

function clean_database(){
  // Clean the database
  db.removeItem("code");
  db.removeItem("access_token");
  db.removeItem("refresh_token");
  db.removeItem("code_error");
}


function sendMessageToApp(dictionary){

	var readable_dictionary = '';
	for (key in dictionary){
		readable_dictionary += key + ": " + dictionary[key] + " ";
	}

	Pebble.sendAppMessage(dictionary,
		function(e) {
		  console.log('Message sent to Pebble successfully! ' + readable_dictionary);
		},
		function(e) {
		  console.log('Error sending message to Pebble! ' + readable_dictionary);
		}
	);
}

function locationSuccess(pos) {
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather';
  var params = ['lat='+pos.coords.latitude,'lon='+pos.coords.longitude];

  var success = function(responseText){
	  	var json = JSON.parse(responseText);

		// Temperature in Kelvin requires adjustment
		var temperature = Math.round(json.main.temp - 273.15);
		console.log('Temperature is ' + temperature);

		// Conditions
		var conditions = json.weather[0].main;      
		console.log('Conditions are ' + conditions); 
		      
		  // Assemble dictionary using our keys
		var dictionary = {
		   'KEY_TEMPERATURE': temperature,
		   'KEY_CONDITIONS': conditions
		};
		  
		// Send to Pebble
		sendMessageToApp(dictionary);
	};

	var error = function(responseText) {
		//TODO implement an error state for this
	};

	xhrRequest(url, "GET", params, null, success, error);
}


// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }                     
);

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}


// ------------------------- //
// Google Calendar FUNCTIONS //
// ------------------------- //


var GOOGLE_CLIENT_ID = '1000865298828-ee7o3g9jsdimltbk9futjt6pp13vaav4.apps.googleusercontent.com';
var GOOGLE_CLIENT_SECRET = 'QtGvlFxQGpdshPoqaiXS_gOD';
var GOOGLE_REDIRECT_TOKEN_URI = 'http://vieju.net/misato/pebbleWear/configuration.php';
var CONFIG_URL = 'http://vieju.net/misato/pebbleWear/configuration.php';
var GOOGLE_API_KEY = 'AIzaSyADXDNNK8F-Q6tucJRzx0ecFB-yQe1k-gM';

// Util Date functions

// Parses a Date in the GCalendar readable format to a noraml Date object
// From: "2015-06-19T13:00:00+02:00" to 19 June 2015 at 13:00 (GMT+2)
function stringToDate(dateString){
	var dateStr = dateString.replace(/\D/g," ");
	var components = dateStr.split(" ");
	// modify month between 1 based ISO 8601 and zero based Date
	components[1]--;
	var date = new Date(Date.UTC(components[0],components[1],components[2],components[3],components[4],components[5]));
	return date;
}

// Returns a Google Calendar date to be used in the paramether of the GCalendar events call
// For example if today's date is: 19 June 2015 at 13:00 (GMT+2)
// It will return: "2015-06-19T13:00:00+02:00"
function dateToString(date) {
	var offset = date.getTimezoneOffset();
	var hours_offset = Math.abs(offset) / 60;
	var hours = '' + hours_offset;
	if (hours_offset < 10) {
		hours = '0' + hours_offset;
	}

	var dateString = date.toISOString();
	var gCalendarDate = dateString.substring(0, dateString.indexOf('.')) + "+"+ hours + ":00";
	return gCalendarDate;
}

// Get the next Google Calendar event
function getCalendarData(){
	use_access_token(function(access_token) {
		var today = new Date();
		var eventMinDate = dateToString(today);
		var calendar_id = encodeURIComponent("primary");


		//var google_calendar_url = "https://www.googleapis.com/calendar/v3/calendars/"+calendar_id+"/events?orderBy=startTime&maxResults=1&timeMin="+encodeURIComponent(eventMinDate) +"&key="+GOOGLE_API_KEY;
		var google_calendar_url = "https://www.googleapis.com/calendar/v3/calendars/"+calendar_id+"/events";
		var params = ["timeMin="+encodeURIComponent(eventMinDate),"maxResults=1","key="+GOOGLE_API_KEY];
		var header = ["Authorization","Bearer "+db.getItem("access_token")];

		var success = function(responseText){
			var events = JSON.parse(responseText);
				
			if (events && events.items && events.items.length > 0){
				var nextEvent = events.items[0];
				// Parse the Date String into readable format
				var eventDate = stringToDate(nextEvent.start.dateTime);
				var event_info = {
					'KEY_EVENT_NAME' : nextEvent.summary,
					'KEY_EVENT_DAY' : eventDate.getDate(),
          'KEY_EVENT_MONTH' : eventDate.getMonth(),
          'KEY_EVENT_YEAR' : eventDate.getFullYear(),
          'KEY_EVENT_HOUR' : eventDate.getHours(),
          'KEY_EVENT_MINUTE' : eventDate.getMinutes()
				};
				sendMessageToApp(event_info);
			}
			else {
				//TODO change this to more readable error
				sendMessageToApp({'KEY_EVENT_NAME' : 'No events'});
			}
		};

		var error = function(responseText) {};

		xhrRequest(google_calendar_url, "GET", params, header, success,error);
		//DATE=2015-06-16T10%3A13%3A15%2B02%3A00
		//GET https://www.googleapis.com/calendar/v3/calendars/primary/events?timeMin={DATE}&key={YOUR_API_KEY}
    });
}

// Retrieves the refresh_token and access_token.
// - code - the authorization code from Google.
function resolve_tokens(code) {
	console.log("resolve_tokens");
	var url = "https://accounts.google.com/o/oauth2/token";
	var header = ["Content-type", "application/x-www-form-urlencoded; charset=utf-8"];
	var params = ["code="+encodeURIComponent(code), "client_id="+GOOGLE_CLIENT_ID, 
				  "client_secret="+GOOGLE_CLIENT_SECRET, "redirect_uri="+GOOGLE_REDIRECT_TOKEN_URI,
				  "grant_type=authorization_code"];
	var success = function(responseText){
		var result = JSON.parse(responseText);
		if (result.refresh_token && result.access_token) {
			db.setItem("refresh_token", result.refresh_token);
			db.setItem("access_token", result.access_token);

			getCalendarData();
		}
	};

	var error = function(responseText){
		clean_database();
		db.setItem("code_error", "Unable to verify the your Google authentication.");
	};

	xhrRequest(url, "POST", params, header, success, error);
}

// Runs some code after validating and possibly refreshing the access_token.
// - success - code to run with the access_token, called like success(access_token)
function use_access_token(success) {
    var refresh_token = db.getItem("refresh_token");
    var access_token = db.getItem("access_token");

    if (!refresh_token) {
        console.log("there is not refresh token. Getting one.");

    }

    valid_token(access_token, success, function() {
        refresh_access_token(refresh_token, success);
    });
}

// Validates the access token.
// - access_token - the access_token to validate
// - good - the code to run when the access_token is good, run like good(access_token)
// - bad - the code to run when the access_token is expired, run like bad()
function valid_token(access_token, success, error) {

	var url = "https://www.googleapis.com/oauth2/v1/tokeninfo?access_token=" + access_token;
	var successRequest = function(responseText) {
		var result = JSON.parse(responseText);

		if (result.audience != GOOGLE_CLIENT_ID) {
			db.removeItem("code");
			db.removeItem("access_token");
			db.removeItem("refresh_token");
			db.setItem("code_error", "There was an error validating your Google Authentication. Please re-authorize access to your account.");
			return;
		}

		success(access_token);
	};
	var errorRequest = function (responseText){
		error();
	};
	
	xhrRequest(url, "GET", null, null, successRequest, errorRequest);
}

// Refresh a stale access_token.
// - refresh_token - the refresh_token to use to retreive a new access_token
// - success - code to run with the new access_token, run like success(access_token)
function refresh_access_token(refresh_token, success) {
	console.log("refresh_access_token");
	var url = "https://accounts.google.com/o/oauth2/token";
	var params = ["refresh_token="+encodeURIComponent(refresh_token), 
				  "client_id="+GOOGLE_CLIENT_ID, "client_secret="+GOOGLE_CLIENT_SECRET,
				  "grant_type=refresh_token"];
	var header = ["Content-type", "application/x-www-form-urlencoded; charset=utf-8"];

	var successRequest = function(responseText){
		var result = JSON.parse(responseText);
      	if (result.access_token) {
            db.setItem("access_token", result.access_token);
            success(result.access_token);
        }
        else {
        	clean_database();
			    db.setItem("code_error", "Error refreshing access_token");
        }
	};

	var errorRequest = function(responseText){
		clean_database();
		db.setItem("code_error", "Error refreshing access_token");
	};

	xhrRequest(url, "POST", params, header, successRequest, errorRequest);
}

// When you click on Settings in Pebble's phone app. Go to the configuration.html page.
function show_configuration() {
    clean_database();
    Pebble.openURL(CONFIG_URL);
}

// When you click Save on the configuration.html page, recieve the configuration response here.
function webview_closed(e) {
    var json = e.response;
    var config = JSON.parse(json);

    var code = config.code;
    var old_code = db.getItem("code");
    if (old_code != code) {
        db.setItem("code", code);
        db.removeItem("refresh_token");
        db.removeItem("access_token");
    }

    resolve_tokens(code);
}

// Setup the configuration events
Pebble.addEventListener("showConfiguration", show_configuration);
Pebble.addEventListener("webviewclosed", webview_closed);


// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getWeather();
    getCalendarData();

  }
);
