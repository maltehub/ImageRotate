var xhrRequest = function (url, type, success, error) {
  console.log("Sending request to: "+url);
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (this.readyState == 4 && this.status == 200) {
        console.log("Success: "+ this.responseText);
        success(this.responseText);
    }
    else {
      console.log("error: "+ this.responseText);
      error(this);
    }
  };
  xhr.open(type, url);
  xhr.send();
};

var db = window.localStorage;

function clean_database(){
  // Clean the database
  db.removeItem("code");
  db.removeItem("access_token");
  db.removeItem("refresh_token");
  db.removeItem("code_error");
}


function sendMessageToApp(dictionary){
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Message sent to Pebble successfully! ' + dictionary);
        },
        function(e) {
          console.log('Error sending message to Pebble! ' + dictionary);
        }
      );
}

function locationSuccess(pos) {
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
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
    }      
  );
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


// Google Calendar FUNCTIONS

var GOOGLE_CLIENT_ID = '1000865298828-ee7o3g9jsdimltbk9futjt6pp13vaav4.apps.googleusercontent.com';
var GOOGLE_CLIENT_SECRET = 'QtGvlFxQGpdshPoqaiXS_gOD';
var GOOGLE_REDIRECT_TOKEN_URI = 'http://vieju.net/misato/pebbleWear/configuration.php';
var CONFIG_URL = 'http://vieju.net/misato/pebbleWear/configuration.php';
var GOOGLE_API_KEY = 'AIzaSyADXDNNK8F-Q6tucJRzx0ecFB-yQe1k-gM';

// Get the next Google Calendar event
function getCalendarData(){
	use_access_token(function(access_token) {
		var today = new Date();
		var offset = today.getTimezoneOffset();
		var hours_offset = Math.abs(offset) / 60;
		var hours = '' + hours_offset;
		if (hours_offset < 10) {
			hours = '0' + hours_offset;
		}

		var dateString = today.toISOString();
		var eventMinDate = dateString.substring(0, dateString.indexOf('.')) + "+"+ hours + ":00";
		var calendar_id = encodeURIComponent("misato.vk@gmail.com");
		//var google_calendar_url = "https://www.googleapis.com/calendar/v3/calendars/"+calendar_id+"/events?orderBy=startTime&maxResults=1&timeMin="+encodeURIComponent(eventMinDate) +"&key="+GOOGLE_API_KEY;
		var google_calendar_url = "https://www.googleapis.com/calendar/v3/calendars/"+calendar_id+"/events?timeMin="+encodeURIComponent(eventMinDate) +"&key="+GOOGLE_API_KEY;
	
		var xhr = new XMLHttpRequest();
		xhr.onload = function () {
			if (this.readyState == 4 && this.status == 200) {
				console.log("Success: "+ this.responseText);
				var events = JSON.parse(this.responseText);
				
				if (events && events.items && events.items.length > 0){
					var nextEvent = events.items[0];
					var event_info = {
						'KEY_NAME_EVENT' : nextEvent.summary,
						'KEY_TIME_EVENT' : nextEvent.start.datetime
					};
					sendMessageToApp(event_info);
				}
				else {
					//TODO change this to more readable error
					sendMessageToApp({'KEY_NAME_EVENT' : 'No events'});
				}
			}
			else {
				console.log("error: "+ this.responseText);
			}
		};
		xhr.open("GET", google_calendar_url);
		xhr.setRequestHeader("Authorization","Bearer "+db.getItem("access_token"));
		xhr.send();
		
		//DATE=2015-06-16T10%3A13%3A15%2B02%3A00
		//GET https://www.googleapis.com/calendar/v3/calendars/primary/events?timeMin={DATE}&key={YOUR_API_KEY}
    });
}

// Retrieves the refresh_token and access_token.
// - code - the authorization code from Google.
function resolve_tokens(code) {
  var url = "https://accounts.google.com/o/oauth2/token?code="+encodeURIComponent(code)+"&client_id="+GOOGLE_CLIENT_ID+"&client_secret="+GOOGLE_CLIENT_SECRET+"&redirect_uri="+GOOGLE_REDIRECT_TOKEN_URI+"&grant_type=authorization_code";
  xhrRequest(url, "POST", function(responseText) {
        var result = JSON.parse(responseText);

        if (result.refresh_token && result.access_token) {
            db.setItem("refresh_token", result.refresh_token);
            db.setItem("access_token", result.access_token);

            getCalendarData();
        }
  }, function(request){
        clean_database();
        db.setItem("code_error", "Unable to verify the your Google authentication.");
  });

}

// Runs some code after validating and possibly refreshing the access_token.
// - success - code to run with the access_token, called like success(access_token)
function use_access_token(success) {
    var refresh_token = db.getItem("refresh_token");
    var access_token = db.getItem("access_token");

    if (!refresh_token) return;

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
	xhrRequest(url, 'GET', function(responseText) {
				var result = JSON.parse(responseText);

				if (result.audience != GOOGLE_CLIENT_ID) {
					db.removeItem("code");
					db.removeItem("access_token");
					db.removeItem("refresh_token");
					db.setItem("code_error", "There was an error validating your Google Authentication. Please re-authorize access to your account.");
					return;
				}

				success(access_token);
			},error);
}

// Refresh a stale access_token.
// - refresh_token - the refresh_token to use to retreive a new access_token
// - success - code to run with the new access_token, run like success(access_token)
function refresh_access_token(refresh_token, success) {
  var url = "https://accounts.google.com/o/oauth2/token?refresh_token="+encodeURIComponent(refresh_token)+"&client_id="+GOOGLE_CLIENT_ID+ "&client_secret="+GOOGLE_CLIENT_SECRET+ "&grant_type=refresh_token";

    xhrRequest(url, "POST", function(responseText) {
            var result = JSON.parse(responseText);

            if (result.access_token) {
                db.setItem("access_token", result.access_token);
                success(result.access_token);
            }
    }, function(request) {
          db.setItem("code_error", "Error refreshing access_token");
    });
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
    if (!db.getItem("code") && !db.getItem("access_token")) {
      show_configuration();
    }
    else {
      getCalendarData();
    }

  }
);
