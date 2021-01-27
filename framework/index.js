console.log("HELLO")

var gpio = require("gpio")
var b = require("b")
var wifi = require("wifi")
wifi.connect("haiwei", "Seaway2019")

// var gpio_option = {
//     pin_number
// }

gpio.open(22, 1)

b()
var http = require("http")
var option = {
    url: "http://192.168.3.22:3000",
    method: "POST",
    data: {"boardname": "esp32", "language":"javascript", "picture" : "Camera.jpg"},
    header: {
        "Content-Type": "text/json"
    },
    success: function(res) {
        console.log("success callback");
        console.log(res.data);
    }
}

var req = http.request(option);

req.on("complete", function() {
    console.log("complete callback");
})

setInterval(function() {
    gpio.writeSync(22, 1)
    console.log('led on!')
    setTimeout(function() {
        gpio.writeSync(22, 0)
        console.log('led off!')
    }, 2000)
}, 4000)

console.log("JS END");
