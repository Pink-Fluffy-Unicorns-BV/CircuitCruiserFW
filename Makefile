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
# NAME       = Documentation                                              #
# PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	              #
# DATE       = 11-12-2024                        	                      #
# AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema                             #
# WEBSITE    = https://pinkfluffyunicorns.nl                              #
# COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

all: build

build:
	idf.py build

flash:
	idf.py flash

monitor:
	idf.py monitor

DATE := $(shell TZ=Europe/Amsterdam date "+%Y-%m-%d %H-%M %Z")

merge: build
	idf.py merge-bin -o merged-binary.bin

release: build merge
	mkdir "release/${DATE}"
	cp build/flash_args "release/${DATE}"
	cp build/merged-binary.bin "release/${DATE}"
	@for f in `tail -n +2 build/flash_args | cut -f2 -d' '` ; do \
		echo "Copying $$f" ; \
		cp build/$$f "release/${DATE}" ; \
	done
	cd release; zip "${DATE}.zip" "${DATE}"/*
	@echo "Created release ${DATE}."

.PHONY: all build flash monitor release
