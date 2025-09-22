# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                         #
#                              +@@+    .-++-------=*=.                    #
#                             +%%@+-------------------+                   #
#                            =@@+*=---------------------==                #
#                         .%+-=@%*+=----------------------=*.             #
#                        =%---=@=***-------------------------+.           #
#                       .%----=@%#=+%=------------------------*           #
#                        *+---=@**#%-#=----------+:*------------          #
#                        .#+--=@%%#++++=-------+: :*-----------*          #
#                           +##@%#%%%#=#=----*:..:*------------#.         #
#             **%@@@@@@@#+-.                    -=------------+           #
#       .*@%+.         .                       =------------==            #
#    =@=.....         =@+=%%   +@   --        :*-------------             #
#   @*........                 .#@@@-          +------------=:            #
#   @=.......                                  -+---------------++*.      #
#   *%-...-%@.                     ...         .=------------------:      #
#     -@@=...                                   .+----------------#       #
#        =#@%+-..                               ..+-------------=.        #
#                  .:-=*#%%%@@@:                ...=----------+:  +=+:    #
#                            +%                  ...+---------= .-----:   #
#                           :@.                   ...++--------------+    #
#                           %*                     ....+@%#-----%+        #
#                          :@.                      .....+@:              #
#                                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# NAME       = Build rules for top level                                  #
# PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	              #
# DATE       = 11-12-2024                        	                      #
# AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema                             #
# WEBSITE    = https://pinkfluffyunicorns.nl                              #
# COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS "hardware" "lua")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Circuit-Cruiser)

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/web")
spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT)