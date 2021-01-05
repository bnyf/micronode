var gpio = require("gpio");

gpio.open(21, 2);
gpio.writeSync(21, 1);
// gpio.close(21, 1);