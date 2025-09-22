-- # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
-- #                                                                         #
-- #                              +@@+    .-++-------=*=.                    #
-- #                             +%%@+-------------------+                   #
-- #                            =@@+*=---------------------==                #
-- #                         .%+-=@%*+=----------------------=*.             #
-- #                        =%---=@=***-------------------------+.           #
-- #                       .%----=@%#=+%=------------------------*           #
-- #                        *+---=@**#%-#=----------+:*------------          #
-- #                        .#+--=@%%#++++=-------+: :*-----------*          #
-- #                           +##@%#%%%#=#=----*:..:*------------#.         #
-- #             **%@@@@@@@#+-.                    -=------------+           #
-- #       .*@%+.         .                       =------------==            #
-- #    =@=.....         =@+=%%   +@   --        :*-------------             #
-- #   @*........                 .#@@@-          +------------=:            #
-- #   @=.......                                  -+---------------++*.      #
-- #   *%-...-%@.                     ...         .=------------------:      #
-- #     -@@=...                                   .+----------------#       #
-- #        =#@%+-..                               ..+-------------=.        #
-- #                  .:-=*#%%%@@@:                ...=----------+:  +=+:    #
-- #                            +%                  ...+---------= .-----:   #
-- #                           :@.                   ...++--------------+    #
-- #                           %*                     ....+@%#-----%+        #
-- #                          :@.                      .....+@:              #
-- #                                                                         #
-- # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
-- # NAME       = Example code.                                              #
-- # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
-- # DATE       = 02-09-2025                        	                     #
-- # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema                             #
-- # WEBSITE    = https://pinkfluffyunicorns.nl                              #
-- # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
-- # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

-- Demo LED animation.
for n = 1, 2 do
    for i = 6, 11 do
        set_LED(i, BRIGHTRED)
        event.wait(math.floor(1000 / 15))
        set_LED(i, BLACK)
    end
    for i = 11, 6, -1 do
        set_LED(i, BRIGHTRED)
        event.wait(math.floor(1000 / 15))
        set_LED(i, BLACK)
    end
    event.wait(500)
end

-- Turn on normal lights again.
reset_LEDs()

-- Wake up startup code. If it was launched from elsewhere, this event still
-- ends up in the queueu of startup.lua. That task doesn't check for events
-- while running, so that is harmless.
event.send{MY_EVENT, 0}
