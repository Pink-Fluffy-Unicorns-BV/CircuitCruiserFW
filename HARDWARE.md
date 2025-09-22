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

This document describes how the system is organized and how to use it.

## Hardware
This code is meant to run on an ESP32S3 that is the heart of the car.
Some parts are provided on the device, more can be added by the user.

## Software
The code consists of several parts:

  - The wifi controller allows users to connect to the hardware.
  - The web server handles requests from the user.
  - The controller ties everything together.
  - The lua engine allows users to modify the behaviour through scripts.

### Using the device
When powered on, the device hosts a wifi network which uses part of the
mac address in its SSID. The password is the Crc of the SSID, in hex.

When running "make monitor", the device will report its SSID and password.
By typing "return wifi" it will report it again.

When connected, the controls can be accessed by pointing a browser at
https://192.168.4.1/.

On the web page, the device can be controlled manually, scripts can be edited,
and Lua commands can be sent.

When using the webpage on an iOS device, you can add a link to the homescreen
and the page will be usable as a web-app.
This will make the web page fullscreen on your phone.

### Scripting
The code uses Lua to allow users to easily modify the device, which includes
adding support for newly connected hardware, through Lua scripts. The default
hardware support is also implemented using Lua scripts. Those scripts can
be used as examples for how to implement drivers.

## Lua Interface
The following commands can be sent from Lua scripts:

  - event.claim(eventcode, raw): After this call, the events of the selected
  type will be sent to the calling script. Default events are listed below.
  If raw is true, parameters are not parsed. See below.
  - event.release(eventcode): After this call, the selected event will be
  ignored until it is claimed again.
  - event.find(name): Return the event code of the named event.
  - event.get_name(eventcode): Return the name of the selected event.
  - event.new(name, floats, ints, strings): Create a new event type. The event
  code is returned. The parameters (other than name) must all be nil, or tables
  of up to 3 strings. Those are the names of the parameters of those types.
  Every name may only be used in one of the 3 tables, and it must not be used
  more than once in that table. The name *event* is reserved. A named parameter
  may not follow a nil parameter.
  - event.send(eventcode, floats..., ints..., strings...):
  send an event. The number of floats, ints and strings must match the event
  definition.
  - event.send{event = name, *parameter* = *value*, ...}: Syntactic sugar for
  event.send(). The eventname and parameter names and types must match
  the values from the definition. This is slower than the other version, because
  the names are looked up.
  - event.wait(timeout): wait for a maximum of timeout milliseconds (or
  forever, if timeout is nil or missing) until an event is received.

Lua scripts can receive events that they claimed. These are returned from
event.wait(). This returns 2 values: the event code (or nil if the timeout
expired), and a table with the named parameters from the event definition (or
nil if the timeout expired).

### Pre-defined events
This function is defined in startup.lua and can be called by the user interface:

  - js(float x, float y): specify a new joystick position.

These events can be sent by the hardware:

  - pin_change(int pin, int state): a pin that was set up for interrupts has
  changed value.

These events are claimed by the hardware:

  - set_pin(int pin, int mode): set up a pin for GPIO_LOW, GPIO_HIGH,
  GPIO_FLOAT, GPIO_PULLUP, or GPIO_PULLDOWN for non-interrupt states, or
  GPIO_RISING, GPIO_FALLING, or GPIO_CHANGE for generating interrupts.
  - pin_read(int pin, int reply): read the current value of a gpio pin and
  send it to the reply event using the pin number as the first int parameter,
  and the state as the second. The event must be defined to accept only those
  two parameters.
  - i2c_new(int addr, int speed, int timeout, int reply): Register new i2c
  device. The new device id is sent as an event to the reply event.
  - i2c_read(int target, int reply):
  Read num items of bytes bytes each from device dev (which has been returned
  from i2c_new), starting at register reg. This will send byte reg to device
  addr, after which it will read num times bytes bytes from device addr.
  The result is returned as an event with code reply. Its payload is num ints.
  dev, reg, num, bytes are packed into target; dev is the MSB, bytes is the LSB.
  - i2c_write(int target, int data0, int data1, int data2, int data3,
  int data4): Send num times bytes bytes to dev at register reg. Each data
  argument encodes bytes bytes.
  dev, reg, num, bytes are packed into target; dev is the MSB, bytes is the LSB.

## New Lua Versions
If a new Lua version should be installed, the steps to follow are:

  - Rename the current *lua* folder to *lua.backup*.
  - Download and unpack the new Lua release.
  - Rename the unpacked folder to *lua*.
  - Set *LUA_32BITS* to *1* in *lua/src/luaconf.h*.
  - Copy *lua.backup/CMakeLists.txt* into the new *lua* folder.
  - Test if everything works. If not, fix it.
  - Remove *lua.backup*.
  

# building a release
make all
  will build the complete project
make build
  will do the same as above, build the complete project

make flash
  will flash the project to the configured serial port. In VSCode combined with
  the ESP-IDF plugin you can set the port to flash and the port to monitor. When
  you use the usb port on the CircuitCruiser, make sure these are set to the
  same port.

make monitor
  opens the idf.py monitor on the configured monitor port.

make release
  after a build and merge copies all the binaries for the release to the release/xxx folder.
  The merged binary contains all other parts; it should be flashed to address 0.

# Devcontainer
The project is developed in VSCode, using a Docker devcontainer on Windows.

While it may be possible to build the project using other configurations, those
have not been tested.

The devcontainer that we used is defined in the
[.devcontainer folder](.devcontainer).

You need to install Docker Desktop and VSCode to compile the project using the
devcontainer. Simply opening the projectfolder in VSCode should give you the
option to run the project in the container.

If you want to program the device from the devcontainer on a Windows machine,
you can use [WSL-USB](https://gitlab.com/alelec/wsl-usb-gui) to make the USB
device accessible on WSL and Docker. Start the tool, make sure the device is
forwarded, and restart the container.
