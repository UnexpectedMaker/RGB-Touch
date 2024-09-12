# RGB Touch Mini Firmware

This is where you'll find source code for various RGB Touch Mini projects.

Currently there is only Platform IO source available for the shipping firmware and for Tic Tac Toe, which is an example ESP-NOW based multi player game, you play with multiple RGB Touch Mini devices.

Over time, more projects will be added, including some simpler starting off templates.

CircuitPython and MicroPython examples will also be added soon.

#### IO Mapping List

IO7		IMU INT (LIS3DH)
IO8 	I2C SDA
IO9 	I2C SCL
IO15	ROW INT (MPR121)
IO16 	COL INT (MPR121)
IO18 	VBAT SENSE (ADC via voltage divider)
IO21 	PWR SHUTDOWN 
IO34	I2S AMP SELECT (MAX98357A)
IO35	I2S AMP DATA
IO36 	I2S AMP BCLK
IO37 	I2S AMP LRCLK
IO38 	LED MATRIX PWR
IO39	LED MATRIX DATA (12x12 SK6805)
IO48	VBUS SENSE
