Pebble.addEventListener('ready',
	function(e) {
		console.log("App ready");
		return;
	}
);

// Config
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Weather
var cacheTime = 840000; // Time to cache temp in ms (14 mins)
//var owmapikey = "23789eef1c47852e89dc06b532451573";

function retrieve_weather() {
	console.log("Updating Weather...");
	var locationOptions = {
		enableHighAccuracy: false, // Quick and battery efficient plox
		maximumAge: 600000, // We'll accept any location from the last 10 minutes
		timeout: 120000 // Return a result within 2 minutes max
	};
	
	navigator.geolocation.getCurrentPosition(function(pos) {
		if (pos.coords.latitude && pos.coords.longitude) {
			// Successfully loaded pos
			console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude + ' time: ' + new Date().getMinutes());

			var xmlhttp = new XMLHttpRequest();
			//url = "http://api.openweathermap.org/data/2.5/weather?lat="+ pos.coords.latitude +"&lon="+ pos.coords.longitude +"&units=metric&APPID="+ owmapikey;
			url = "https://api.open-meteo.com/v1/forecast?latitude="+ pos.coords.latitude +"&longitude="+ pos.coords.longitude +"&current=temperature_2m";

			xmlhttp.onreadystatechange = function() {
					if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
						var response = JSON.parse(xmlhttp.responseText);
						if (response && response.main && response.main.temp) {
							console.log(url);
							console.log(JSON.stringify(response));
							console.log(response.main.temp);

							const temp = response.current.temperature_2m;
							
							localStorage.setItem("storedTemp", temp);
							localStorage.setItem("tempTime", new Date().getTime());
							
							Pebble.sendAppMessage( {'WeatherResponse': String(Math.round(temp)) +"°C"},
								function(e) {
									console.log('Successfully delivered message with transactionId='+ e.data.transactionId);
								},
								function(e) {
									console.log('Unable to deliver message with transactionId='+ e.data.transactionId +' Error is: '+ e.error.message);
								}
							);
							
						}
					}
			};
			xmlhttp.open("GET", url, true);
			//xmlhttp.setRequestHeader('Accept', 'text/plain');
			xmlhttp.send();
		}
	}, function(err) {
		// Error loading pos
		console.log('location error (' + err.code + '): ' + err.message);
	}, locationOptions);
}

function js_update_weather() {
	console.log("js_update_weather");
	// Send our stored value if we have one
	if (localStorage.getItem("storedTemp") !== null) {
		Pebble.sendAppMessage( {'WeatherResponse': String(Math.round(localStorage.getItem("storedTemp"))) +"°C"},
			function(e) {
				console.log('Successfully delivered message with transactionId='+ e.data.transactionId);
			},
			function(e) {
				console.log('Unable to deliver message with transactionId='+ e.data.transactionId +' Error is: '+ e.error.message);
			}
		);
	}
	// Update our stored value if it is old
	if (!(localStorage.getItem("tempTime")) || localStorage.getItem("tempTime") <= (new Date().getTime() - cacheTime)) {
		retrieve_weather(); // Get updated weather
		console.log("Weather too old");
	}
}

Pebble.addEventListener("appmessage", function(e) {
	console.log("appmessage received: ", e.payload);
	if (e.payload["WeatherRequest"] == 1) {
		js_update_weather();
	}
});