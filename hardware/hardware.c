/*
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 * #                                                                         #
 * #                              +@@+    .-++-------=*=.                    #
 * #                             +%%@+-------------------+                   #
 * #                            =@@+*=---------------------==                #
 * #                         .%+-=@%*+=----------------------=*.             #
 * #                        =%---=@=***-------------------------+.           #
 * #                       .%----=@%#=+%=------------------------*           #
 * #                        *+---=@**#%-#=----------+:*------------          #
 * #                        .#+--=@%%#++++=-------+: :*-----------*          #
 * #                           +##@%#%%%#=#=----*:..:*------------#.         #
 * #             **%@@@@@@@#+-.                    -=------------+           #
 * #       .*@%+.         .                       =------------==            #
 * #    =@=.....         =@+=%%   +@   --        :*-------------             #
 * #   @*........                 .#@@@-          +------------=:            #
 * #   @=.......                                  -+---------------++*.      #
 * #   *%-...-%@.                     ...         .=------------------:      #
 * #     -@@=...                                   .+----------------#       #
 * #        =#@%+-..                               ..+-------------=.        #
 * #                  .:-=*#%%%@@@:                ...=----------+:  +=+:    #
 * #                            +%                  ...+---------= .-----:   #
 * #                           :@.                   ...++--------------+    #
 * #                           %*                     ....+@%#-----%+        #
 * #                          :@.                      .....+@:              #
 * #                                                                         #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 * # NAME       = Hardware drivers		                	                 #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser                         #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

#include "hardware.h"
#include "gpio.h"
#include "led.h"
#include "pwm.h"
#include "motor.h"
#include "i2c.h"
#include <event.h>

#define QUEUE_LENGTH 50

void hardware_task(void *arg)
{
	(void)&arg;
	QueueHandle_t queue = xQueueCreate(QUEUE_LENGTH, sizeof(Event));

	gpio_init(queue);
	led_init(queue, 1);
	pwm_init(queue);
	i2c_init(queue);
	motor_init(queue);

	while (true) {
		Event event;
		if (!event_wait(-1, &event, queue)) {
			// Should not happen.
			printf(_("hardware event timed out?!\n"));
			continue;
		}

		if (gpio_event(&event))
			continue;

		if (led_event(&event))
			continue;

		if (pwm_event(&event))
			continue;
		
		if (i2c_event(&event))
			continue;
		
		if (motor_event(&event))
			continue;

		// Unrecognized event.

		// Clean up.
		event_free(&event);
	}
}
