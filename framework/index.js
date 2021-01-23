console.log("hello")

var gpio = require("gpio")
gpio.open(22, 1)
gpio.writeSync(22, 1)

var http = require("http")
var option = {
    url: "http://192.168.0.108:3000",
    method: "GET",
    success: function(res) {
        console.log("success callback");
        console.log(res.data);
    }
}

var req = http.request(option);

req.on("complete", function() {
    console.log("complete callback");
})


console.log("JS END");
