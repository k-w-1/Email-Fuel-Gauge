# Email Fuel Gauge

This is a fun project that uses any Arduino with a serial port & capability to run a few WS2812/NeoPixel addressable LEDs (although any chipset supported by the Adafruit NeoPixel library would work; or you could swap it for FastLED for an even more extensive list).

Essentially you use one of the widely available LED strips (originally designed for the 8x pixel boards available at the usual spots (eBay/Ali/Amazon etc) for <$5; however the number of pixels can be configured to any number supported by the Ardiuno). Using Exchange Web Services, the PowerShell script checks the number of unread messages in your Inbox and will draw a coloured graph representing how bad it looks! 

![foo](https://cdn-shop.adafruit.com/970x728/1426-01.jpg "Example strip")

Wiring is simple for these smaller boards; simply attach `vin` to the Arduino's 5v or an external power supply for larger strips, `gnd` to the Arduino's ground (and the external PSU's ground, as appropriate), and the strip's `din` to `A0` (or modify the Arduino code to your needs).

For ease of testing, I'd reccomend starting a new Arduino project and saving all the repo files in there -- the PowerShell script will happly run from inside there and Arduino doesn't mind either.

To compile the Arduino firmware, either put the `SerialConsole` files in your Arduino project folder, or in their own folder in your Arduino `libraries` folder. `SerialConsole` is as its name suggests, is a library I wrote for my Arduino projects that bootstraps an interactive console session à la IT infrastructure; with tab completion, backspace support (yes, that isn't rolled into Serial.read()!), and function listing.

At present it's a bit of a proof-of-concept and the PowerShell script does not run continually -- for now you can just setup a Scheduled Task to run it on whatever refresh interval suits your needs.

You will need to get a copy of `Microsoft.Exchange.WebServices.Auth.dll` and `Microsoft.Exchange.WebServices.dll` and place them in the same folder as the PowerShell script.
