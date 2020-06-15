# timonel-ota-demo #
Timonel bootloader use-case demo: Shows a series of steps performed by an ESP8266 I2C master to check for updates, retrieve a new firmware file from the internet, and update an ATtiny85 slave microcontroller over the I2C bus. The Tiny85 runs the [Timonel bootloader](https://github.com/casanovg/timonel), while the master uses the [TimonelTWIM Arduino library](https://github.com/casanovg/Nb_TimonelTwiM) and its dependencies to control it.

There is a simple [flowchart](https://github.com/casanovg/timonel-ota-demo/tree/master/diagrams) showing the workflow implemented by this use-case. The ATtiny85 reset is made through I2C commands, so the user application should support it. Otherwise, a hardware reset solution and a slightly different workflow are needed.
