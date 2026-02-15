Pebble.addEventListener('ready',
	function(e) {
		return;
	}
);

// Config
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Weather
var cacheTime = 840000; // Time to cache temp in ms (14 mins)
var owmapikey = "23789eef1c47852e89dc06b532451573";

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
			//var url = "https://beatles1-open-weather-map-v1.p.mashape.com/?lat="+ pos.coords.latitude +"&lon="+ pos.coords.longitude +"&units=metric&APPID="+ owmapikey;
			url = "http://api.openweathermap.org/data/2.5/weather?lat="+ pos.coords.latitude +"&lon="+ pos.coords.longitude +"&units=metric&APPID="+ owmapikey;
			
			xmlhttp.onreadystatechange = function() {
					if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
						var response = JSON.parse(xmlhttp.responseText);
						if (response && response.main && response.main.temp) {
							console.log(url);
							console.log(JSON.stringify(response));
							console.log(response.main.temp);
							
							localStorage.setItem("storedTemp", response.main.temp);
							localStorage.setItem("tempTime", new Date().getTime());
							
							Pebble.sendAppMessage( {'2': String(Math.round(response.main.temp)) +"°C"},
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
			xmlhttp.setRequestHeader('X-Mashape-Key', 'y8xvtmL8AMmshgxF1lQPlC5FJjrqp1W01WCjsnGrc01VPZt9er');
			xmlhttp.setRequestHeader('Accept', 'text/plain');
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
		Pebble.sendAppMessage( {'2': String(Math.round(localStorage.getItem("storedTemp"))) +"°C"},
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
	if (e.payload["1"] == 1) {
		js_update_weather();
	}
});