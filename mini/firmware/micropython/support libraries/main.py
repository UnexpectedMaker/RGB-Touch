# RGB Touch Mini Basic Example
# MIT license; Copyright (c) 2024 Seon Rozenblum - Unexpected Maker
#
# Project home:
#   https://rgbtouch.com

import rgbtouchmini, time

# Create a reference to the RGB Touch Mini device
device = rgbtouchmini.RGBTouch()

# Endlessly process and display the users touch on the matrix
while True:
    device.process()
    time.sleep_ms(20)