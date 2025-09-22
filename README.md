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
