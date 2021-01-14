var gpio = require("gpio");

gpio.open(21, 1);
gpio.open(22, 1);
gpio.open(15, 2, 1, 2);
gpio.writeSync(21, 1);
var pin_num = 21;
gpio.installIntr(15, function() {
    gpio.writeSync(pin_num, 0);
    pin_num = (pin_num - 20) % 2 + 21;
    gpio.writeSync(pin_num, 1);
});