# RGB Touch Mini Helper Library
# MIT license; Copyright (c) 2024 Seon Rozenblum - Unexpected Maker
#
# Project home:
#   https://rgbtouch.com

# Import required libraries
from micropython import const
from machine import Pin, ADC, I2S, I2C
import neopixel, time, mpr121

import time
import math
import struct

# RGB Touch Hardware Pin Assignments

# Sense Pins
VBUS_SENSE = const(48)
VBAT_SENSE = const(18)

# RGB LED Pins
MATRIX_PWR = const(38)
MATRIX_DATA = const(39)

# I2C
I2C_SDA = const(8)
I2C_SCL = const(9)

# I2S AMP
AMP_LRCLK = const(37)
AMP_BCLK = const(36)
AMP_DIN = const(35)
AMP_SD = const(34)
I2S_ID = 0
BUFFER_LENGTH_IN_BYTES = 2000

# ======= AUDIO CONFIGURATION =======
TONE_FREQUENCY_IN_HZ = 440
SAMPLE_SIZE_IN_BITS = 16
FORMAT = I2S.MONO  # only MONO supported in this example
SAMPLE_RATE_IN_HZ = 22_050
# ======= AUDIO CONFIGURATION =======

class Point:
    def __init__(self, pos_x, pos_y, col_index, fade):
        self.x = pos_x
        self.y = pos_y
        self.col = col_index
        self.fade_amount = fade
    
    def get_point_and_fade(self):
        self.fade_amount -= 0.1
        if self.fade_amount < 0:
            self.fade_amount = 0
            
        # returns pixel color wheel index, fade amount
        return self.col, self.fade_amount
        

class RGBTouch:
    """Library for the RGB Touch Mini."""

    def __init__(self):
        # Peripheral Setup
        # Neopixels
        self.matrix = neopixel.NeoPixel(Pin(MATRIX_DATA), 144)
        # I2C
        self.i2c = I2C(0, scl=Pin(I2C_SCL), sda=Pin(I2C_SDA))
        # MPR121 for cols and rows
        self.mpr_cols = mpr121.MPR121(self.i2c, address=0x5A)
        self.mpr_rows = mpr121.MPR121(self.i2c, address=0x5B)
        self.mpr_cols.set_thresholds(1,0)
        self.mpr_rows.set_thresholds(1,0)
        # IMU
        #

        # I2S Audio
        self.audio_out = I2S(
            I2S_ID,
            sck=Pin(AMP_BCLK),
            ws=Pin(AMP_LRCLK),
            sd=Pin(AMP_DIN),
            mode=I2S.TX,
            bits=SAMPLE_SIZE_IN_BITS,
            format=FORMAT,
            rate=SAMPLE_RATE_IN_HZ,
            ibuf=BUFFER_LENGTH_IN_BYTES,
        )
        
        # General
        
        # using a dict for touches so we can easily identify if the touch exists already via the key and
        # so we can use the key as the x,y index of the LED matrix saving calculating it each time. 
        self.touches = {}
        self.current_color_index = 0
    
    
    # Touch stuff
    
    def add_touch(self, col, row, color):
        index = 12 * row + col
        if index in self.touches.keys():
            self.touches[index].fade_amount = 1.0
            # print(f"Updated touch {index}")
        else:
            self.touches[index] = Point(col, row, color, 1.0)
            # print(f"Added new touch {index} at {col}, {row}")
    
    

    # Neopixel Matrix Stuff
        
    def set_matrix_power(self, state):
        """Enable or Disable power to the onboard NeoPixel to either show colour, or to reduce power for deep sleep."""
        Pin(MATRIX_PWR, Pin.OUT).value(state)  
    
    
    def rgb_color_wheel(self, wheel_pos):
        """Color wheel to allow for cycling through the rainbow of RGB colors."""
        wheel_pos = wheel_pos % 255

        if wheel_pos < 85:
            return 255 - wheel_pos * 3, 0, wheel_pos * 3
        elif wheel_pos < 170:
            wheel_pos -= 85
            return 0, wheel_pos * 3, 255 - wheel_pos * 3
        else:
            wheel_pos -= 170
            return wheel_pos * 3, 255 - wheel_pos * 3, 0
        
    def rgb_color_wheel_fade(self, wheel_pos, fade_amount):
        """Color wheel to allow for cycling through the rainbow of RGB colors."""
        wheel_pos = wheel_pos % 255

        if wheel_pos < 85:
            return round((255 - wheel_pos * 3) * fade_amount), 0, round((wheel_pos * 3) * fade_amount)
        elif wheel_pos < 170:
            wheel_pos -= 85
            return 0, round((wheel_pos * 3) * fade_amount), round((255 - wheel_pos * 3) * fade_amount)
        else:
            wheel_pos -= 170
            return round((wheel_pos * 3) * fade_amount), round((255 - wheel_pos * 3) * fade_amount), 0
        
        
        
    # Audio Stuff
    
    def play_tone(self, rate, bits, frequency):

        # create a buffer containing the pure tone samples
        samples_per_cycle = rate // frequency
        sample_size_in_bytes = bits // 8
        samples = bytearray(samples_per_cycle * sample_size_in_bytes)
        volume_reduction_factor = 32
        range = pow(2, bits) // 2 // volume_reduction_factor
        
        if bits == 16:
            format = "<h"
        else:  # assume 32 bits
            format = "<l"
        
        for i in range(samples_per_cycle):
            sample = range + int((range - 1) * math.sin(2 * math.pi * i / samples_per_cycle))
            struct.pack_into(format, samples, i * sample_size_in_bytes, sample)
            
            # samples = make_tone(SAMPLE_RATE_IN_HZ, SAMPLE_SIZE_IN_BITS, TONE_FREQUENCY_IN_HZ)

            # continuously write tone sample buffer to an I2S DAC
            print("==========  START PLAYBACK ==========")
            try:
                while True:
                    num_written = self.audio_out.write(sample)

            except (KeyboardInterrupt, Exception) as e:
                print("caught exception {} {}".format(type(e).__name__, e))

            # cleanup
            self.audio_out.deinit()
            print("Done")
         
         
    # General Stuff
    
    def battery_voltage(self):
        """
        Returns the current battery voltage. If no battery is connected, returns 4.2V which is the charge voltage
        This is an approximation only, but useful to detect if the charge state of the battery is getting low.
        """
        adc = ADC(Pin(VBAT_SENSE))  # Assign the ADC pin to read
        adc.atten(ADC.ATTN_2_5DB)  # Needs 2.5DB attenuation for max voltage of 1.116V w/batt of 4.2V
        measuredvbat = adc.read()
        measuredvbat /= 3657  # Divide by 3657 as we are using 2.5dB attenuation, which is max input of 1.25V = 4095 counts
        measuredvbat *= 4.2  # Multiply by 4.2V, our max charge voltage for a 1S LiPo
        return round(measuredvbat, 2)


    def vbus_present(self):
        """Detect if VBUS (5V) power source is present"""
        return Pin(VBUS_SENSE, Pin.IN).value() == 1
    
    def process(self):

        # Identify any current touches
        for x in range(12):
            if self.mpr_cols.is_touched(x):
                for y in range(12):
                    if self.mpr_rows.is_touched(y):
                        self.add_touch(x, y, self.current_color_index)

                        # Cycle the colour wheel index because we got a touch
                        self.current_color_index += 1
                        if self.current_color_index > 255:
                            self.current_color_index = 0
        
        # Process all touches and fade them as we display them
        for pixel_index, touch in self.touches.items():
            color_wheel_index, fade_amount = touch.get_point_and_fade()
            self.matrix[pixel_index] = self.rgb_color_wheel_fade(color_wheel_index, fade_amount)
            
            # If the touch we just displayed had faded down to 0, we remove it from the dict
            if fade_amount == 0:
                self.touches.pop(pixel_index)
             
        self.matrix.write()    
