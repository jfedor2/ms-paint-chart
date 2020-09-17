# Plotting sensor data with MS Paint and Bluetooth HID

This is an Arduino sketch that uses the Bluetooth mouse protocol to plot sensor data on a computer running no custom software, only MS Paint.

[Demo video.](https://www.youtube.com/watch?v=6ci6fllYJLU)

It's designed to run on Adafruit's [Feather nRF52840 Sense](https://www.adafruit.com/product/4516) board, but it will run on any of their nRF52-based boards and can easily be modified to use a different sensor.

The trick here is that we're using absolute mouse cursor positioning and therefore don't have to worry about mouse sensitivity and acceleration settings on the user's system.
