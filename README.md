# Workshop-Assistant
My workshop assistant demo, controlling the lights, kettle, heaters, reading sensors, etc.

1. Every device is controlled remotely (with a separate ESP32/ESP8266 controller) using wifi network and MQTT protokol (broker and client) 

2. There is a "Voice assistant" device (called Emma), built on Raspberry Pi 4, 4Gb (but it should work on 3B+ too). It uses Rhino Text-to-intent technology (Pico Voice) (running localy), and Google Text-to-speech API with WaveNet voice (it needs internet connection, but provide best quality for a small device like raspberry). Note that you have to create and use your own authentification, both for PicoVoice and Google Text-to-speech APIs! 

3. MQTT broker is made on raspberry pi zero w board, where most of the sensors are also connected. There is a MQTT client also running on this device, posting updates from all sensors, so the voice assistant and other devices can receive and show the info.

4. the "ID-box" has a built-in TOF sensor, so when a person cross the line near the workshop tables, it sends a comand over MQTT, where a camera (mounted on the Pi Zero W) is taking shots and record video for several seconds.

Please Note: The project is on early stage. I built this very fast and dirty, while making my new workshop, just to be able to control my assistant-lights, making hot water to speciffic temperature (using my cheep kettle), reading some environmental sensors and add some security.<br>
The script (mainly the python one) will be cleared over the time, structurig the functionalities in classes, replacing the countless "if" statements with an adaptive neural-network model, adding more finctions such as weather and forecast, person recognition, etc.

Enjoy!
