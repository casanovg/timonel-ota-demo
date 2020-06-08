# timonel-ota-demo #
Timonel bootloader use-case demo: Updating an Tiny85 firmware over-the-air through an ESP8266 I2C master.

It shows a series of steps made by an I2C ESP8266 master to update the firmware running on ATtiny85 slave. The Tiny85 runs the [Timonel bootloader](https://github.com/casanovg/timonel), while the master uses the [TimonelTWIM library](https://github.com/casanovg/Nb_TimonelTwiM) and its dependencies to control it.

There is a simple [flowchart](https://github.com/casanovg/timonel-ota-demo/tree/master/diagrams) showing the workflow implemented by this use-case. The ATtiny85 reset is made through I2C commands, so the user application should support it. Otherwise, a hardware reset solution and a slightly different workflow are needed.
