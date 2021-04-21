console.log("HELLO")

var gpio = require("gpio")
var b = require("b")
var wifi = require("wifi")

// wifi.connect("haiwei", "Seaway2019")
wifi.connect("haiwei", "Seaway2019")

b()

var http = require("http")

var option = {
    url: "http://192.168.3.55:3000",
    method: "POST",
    data: {"board" : "esp32"},
    header: {
        "Content-Type": "text/json"
    },
    // $success: {
    //     namespace: "home/bedroom/temperature",
    //     priority: 3,
    //     args: {"board" : "esp32"},
    //     callback: function(res, args) {}
    // },
    success: function(res) {
        console.log("success callback");
        console.log(res.data);
    }
}
http.request(option);

setInterval(function() {
    console.log("led on!");
}, 2000)

console.log("JS END");

