M5Stack MAMORA is my acronym for: ```M5Stack Atom Matrix with Oled and Rtc on AtomPortABC```.

To the Port A of the AtomPortABC module are connected:
1. M5Stack OLED (SH1107);
2. M5Stack RTC (8563);

By Pressing the button on top (pressing the screen down) the color of the 5x5 LEDs will change. One of the LEDs colors is BLACK (screen off).

On reset the Arduino Sketch will try to connect to a WiFi Access Point. If successful the sketch will next connect to a NTP server and download the current datetime stamp.
If NTP is connected, the external RTC unit will be set to the NTP datetime stamp.
Next the sketch will display date and time (UTC), taken from the external RTC, onto the OLED display. Every second the date and time will be updated.
Because the external RTC Unit has a built-in battery, the datetime set will not be lost when power is lost.

File secret.h :

Update the file secret.h with your WiFi SSID and WiFi PASSWORD. Also set the NTP_TIMEZONE and the NTP_SERVER_1.

Docs

See the Monitor_output.txt

Images 

Image(s) and a short video impression.
