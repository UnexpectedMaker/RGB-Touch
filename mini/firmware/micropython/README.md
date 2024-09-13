# RGB Touch Mini MicroPython Support

Please keep in mind this is all a work in progress. Not all peripherals and functionality ishave been implemented, but touch and display of the touches is working like `Play Mode` in the shipping C++ firmware. 

More functionality will be added over time, but this is a good starting off point for those that want to jump immediately into using MicroPython on their RGB Touch Mini.

### MicroPython Firmware for RGB Touch Mini
Eventually MicroPython firmware will be available from the MicroPython downloads website, but for now you can grab the firmware from the firmware folder here.

To flash the MicroPython firmware, follow these steps:

#### Erase the flash on your device

You may need to put your RGB Touch Mini into download mode first before Erasing and Flashing. See instructions here: https://help.unexpectedmaker.com/index.php/knowledge-base/how-to-put-your-board-into-download-mode/

##### Linux
```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 erase_flash
```

##### Mac
Please do a `ls /dev/cu.usbm*` to determine the port your board has enumerated as.
```bash
esptool.py --chip esp32s3 --port /dev/cu.usbmodem01 erase_flash
```

##### Windows
Change (X) to whatever COM port is being used by the board
```bash
esptool --chip esp32s3 --port COM(X) erase_flash
```

#### Flash the MicroPython firmware

##### Linux
```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash -z 0x0 rgb_touch_mini_firmware.bin
```

##### Mac
Please do a `ls /dev/cu.usbm*` to determine the port your board has enumerated as.
```bash
esptool.py --chip esp32s3 --port /dev/cu.usbmodem01 write_flash -z 0x0 rgb_touch_mini_firmware.bin
```

##### Windows
Change (X) to whatever COM port is being used by the board
```bash
esptool --chip esp32s3 --port COM(X) write_flash -z 0x0 rgb_touch_mini_firmware.bin
```


# Supporting Libraries
Usually I would build the libraries directly into the firmware, but at this early stage it is better to keep them separate so they can be updated without re-flashing your RGB Touch Mini.

Supporting files are a work in progress and the API and structure is likely to change until everything is implemented.

# Contributing
Please feel free to contribute to the MicroPython support if you are able, thanks!