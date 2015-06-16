var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

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
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
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


function getCalendarData(){
  var today = new Date();
  var offset = today.getTimezoneOffset();
  var hours_offset = Math.abs(offset) / 60;
  var hours = '' + hours_offset;
  if (hours_offset < 10) {
    hours = '0' + hours_offset;
  }

  var dateString = today.toISOString();
  var readableDate = dateString.substring(0, dateString.indexOf('.')) + "+"+ hours + ":00";

  console.log(readableDate);
}

// Second part of Google auth flow
function resolve_access_token(code) {
	var client_id = '1000865298828-ee7o3g9jsdimltbk9futjt6pp13vaav4.apps.googleusercontent.com';
	var client_secret = 'QtGvlFxQGpdshPoqaiXS_gOD';
	var redirect_uri = 'http://vieju.net/misato/pebbleWear/token.html';
	
	var url = 'https://accounts.google.com/o/oauth2/token?code='+encodeURIComponent(code)+'&client_id='+client_id+'&client_secret='+client_secret+'&redirect_uri='+redirect_uri +'&grant_type=authorization_code';
	var db = window.localStorage;

	xhrRequest(url, 'POST', 
	    function(responseText) {
			var result = JSON.parse(responseText);
			if (result.refresh_token && result.access_token){
				db.setItem("refresh_token", result.refresh_token);
				db.setItem("access_token", result.access_token);
				return;
			}
			else {
				db.removeItem("code");
			}
		}
	);
	
	getCalendarData();	
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getWeather();
    getCalendarData();
  }
);

// Show configuration page
Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('http://vieju.net/misato/pebbleWear/configuration.php');
});

// Configuration closed
Pebble.addEventListener('webviewclosed',
  function(e) {
    console.log('Configuration window returned: ' + e.response);
	var config = JSON.parse(e.response);
    var code = config.code;
	var db = window.localStorage;
	var old_code = db.getItem("code");
	if (code != old_code) {
		db.setItem("code", code);
		resolve_access_token(code);
	}
  }
);


