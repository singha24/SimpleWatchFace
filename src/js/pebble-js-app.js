var myAPIKey = 'c6a2cf9b29b37700c6eab49cf9d48b0a';

var TESTPOSTURL = 'http://api.openweathermap.org/data/2.5/weather?lat=52.0373&lon=-0.7709310000000187&appid=c6a2cf9b29b37700c6eab49cf9d48b0a'

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
  var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude + '&appid=' + myAPIKey;
  
  //<><><><><><><><><><><>TESTING<><><><><><><><><><><><><>
  //var url = TESTPOSTURL;
  //

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log("Temperature is " + temperature);

      // Conditions
      var conditions = json.weather[0].main;      
      console.log("Conditions are " + conditions);
      
      // Location 
      var location = json.name;
      console.log("Location is " + location);
      
      // Sunset
      var sunset = convertUnixTimeStamp(json.sys.sunset);
      console.log("Sunset is " + convertUnixTimeStamp(json.sys.sunset));
      
      // Sunrise
      var sunrise = convertUnixTimeStamp(json.sys.sunrise);
      console.log("Sunrise is " + convertUnixTimeStamp(json.sys.sunrise));
      
      // Assemble dictionary using our keys
      var dictionary = {
        "KEY_TEMPERATURE": temperature,
        "KEY_CONDITIONS": conditions, 
        "KEY_LOCATION": location,
        "KEY_SUNSET": sunset, 
        "KEY_SUNRISE": sunrise
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("Weather info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending weather info to Pebble!");
        }
      );
    }      
  );
}

function convertUnixTimeStamp(timeStamp){
  // multiplied by 1000 so that the argument is in milliseconds, not seconds.
  var date = new Date(timeStamp*1000);
  // Hours part from the timestamp
  var hours = date.getHours();
  // Minutes part from the timestamp
  var minutes = "0" + date.getMinutes();
  // Seconds part from the timestamp
  //var seconds = "0" + date.getSeconds();
  
  var ampm = (hours >= 12) ? "pm" : "am";

  // Will display time in 10:30:23 format
  var formattedTime = hours + ':' + minutes.substr(-2) + ampm; //+ ':' + seconds.substr(-2); dont really need the seconds
  return formattedTime;
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getWeather();
  }                     
);
