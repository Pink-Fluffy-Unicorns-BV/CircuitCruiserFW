```
. # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
. #                                                                         #
. #                              +@@+    .-++-------=*=.                    #
. #                             +%%@+-------------------+                   #
. #                            =@@+*=---------------------==                #
. #                         .%+-=@%*+=----------------------=*.             #
. #                        =%---=@=***-------------------------+.           #
. #                       .%----=@%#=+%=------------------------*           #
. #                        *+---=@**#%-#=----------+:*------------          #
. #                        .#+--=@%%#++++=-------+: :*-----------*          #
. #                           +##@%#%%%#=#=----*:..:*------------#.         #
. #             **%@@@@@@@#+-.                    -=------------+           #
. #       .*@%+.         .                       =------------==            #
. #    =@=.....         =@+=%%   +@   --        :*-------------             #
. #   @*........                 .#@@@-          +------------=:            #
. #   @=.......                                  -+---------------++*.      #
. #   *%-...-%@.                     ...         .=------------------:      #
. #     -@@=...                                   .+----------------#       #
. #        =#@%+-..                               ..+-------------=.        #
. #                  .:-=*#%%%@@@:                ...=----------+:  +=+:    #
. #                            +%                  ...+---------= .-----:   #
. #                           :@.                   ...++--------------+    #
. #                           %*                     ....+@%#-----%+        #
. #                          :@.                      .....+@:              #
. #                                                                         #
. # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
. # NAME       = Documentation                                              #
. # PROJECT    = Beursgadget 2025 - Circuit-Cruiser                         #
. # DATE       = 11-12-2024                                                 #
. # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema                             #
. # WEBSITE    = https://pinkfluffyunicorns.nl                              #
. # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
. # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
```

# Beursgadget: Wifi controlled car
This repository provides the code that runs on the Wifi-controlled car that
is given out at the FHI Electronics gathering of 2025.

# Some extra info

On the FHI website is a FAQ with questions and parts that you could order 
if you are missing them:
[FHI E&A 2025 FAQ](https://fhi.nl/eabeurs/gadget-ea-2025-faq/)

In the 3d-files folder in this repo is also the file 
[`circuit_cruiser_servoarmsok_v2.stl`](3d-files/circuit_cruiser_servoarmsok_v2.stl).
This file could be 3-D printed and placed on the steering rack. It will increase
the steering performance of the cruiser.

# Updating the firmware!

The device can be updated with the USB-C port on the device. This is 
connected to the Espressif JTAG debugger that is build into the ESP32S3 chip
on the Circuit Cruiser. You can either flash the merged-binary from the 
release section on this github-page or compile the firmware from this 
repository. 

## Flashing the merged binary

Using the ESP Flash Download Tool you can flash the firmware on the ESP32S3.
Using the following link: 
[ESP Flash Download Tool instructions](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32s3/production_stage/tools/flash_download_tool.html)
You can follow the instructions to use the tool. The ChipType is the ESP32-S3,
The WorkMode is Develop (unless you want to factory flash a lot of devices),
LoadMode should be USB. Then you should use the first line to select the
binary. Use the 3 dots button and the end of the line to select the 
merged-binary. Use adress 0x0 in the next textbox. Make sure the checkbox
at the beginning of the line is checked.

Select the COM to the correct comport and the baudrate can stay at 115200.
When you press start it will start flashing the Circuit Cruiser. 

## Compiling the firmware
You can compile the firmware by installing Microsoft Visual Studio Code 
(VSCode) (__NOT__ Visual Studio!) and installing the EPS-IDF plugin in VSCode. 
The Plugin will help you install the ESP-IDF. When the plugin is installed you
can set the target to ESP32S3 (Using build in USB JTAG debugger), set the COM 
port ('Select Port to Use' and 'Select Monitor Port to Use') to your device.
Then you can build the project, flash the project and monitor the device from
the plugin.

# I bricked my car! What should I do now?
The interface for the car allows you to play with `startup.lua`. If you do the
wrong things with this, it will break the car, in that it can no longer be
controlled. However, the interface will still work.

To fix this, copy the contents of a valid startup.lua file into the editing
area, select `startup.lua` as the filename and hit save. Reset the car and it
works again.

When editing, it is a good idea to keep a known good version of startup.lua on
your system for this purpose. If you do not have one, you can use
[the default file](web/lua/startup.lua) from this repository.

# How can I add more bling?
You are in luck! The car was designed to be modified by the users. For advanced
modifications, you can use the source code from this repository. However, for
most changes that is not required. Instead, you can add and modify Lua code
right from the interface, no installation of development tools required!

For example, you can change what the lights do and when. And you can automate
driving. If you add your own hardware on GPIO pins or I2C, you can control that
from Lua as well. The possibilities are endless! Read [HARDWARE.md](HARDWARE.md)
for information on how the Lua interface works.

More information about the Lua language can be found
[here](https://www.lua.org/manual/5.4/manual.html).

If you make something worth sharing, please let us know by tagging us in your
social media posts! Credits are also appreciated.

# License
This project is licensed under the Affero General Public License version 3, or
any later version published by the Free Software Foundation.

This means you are free to use it for anything you like. You are also allowed to
redistribute it, if you provide access to the source code and you do not include
code that is not licensed as AGPL3+ (or a compatible license) itself.

When using the code in a Cloud service, you also need to make the source code
available to the users of that service.

For the full terms of the license, see [LICENSE](LICENSE).
