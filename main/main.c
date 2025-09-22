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
 * # NAME       = Startup code		                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

#include <string.h>
#include <stdio.h>

#include <led.h>
#include <wifi_controller.h>
#include <webserver.h>
#include <event.h>
#include <cli.h>
#include <hardware.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "esp_err.h"

void app_main(void)
{
	// Set pins to GPIO (not JTAG).
	gpio_reset_pin(GPIO_NUM_39);
	gpio_reset_pin(GPIO_NUM_40);
	gpio_reset_pin(GPIO_NUM_41);
	gpio_reset_pin(GPIO_NUM_42);

	initialize_spiffs();
	event_init();
	start_wifi();
	web_main();

	xTaskCreate(&hardware_task, "hardware", TASK_STACK_SIZE, NULL, 0, NULL);
	xTaskCreate(&cli_task, "cli", TASK_STACK_SIZE, NULL, 0, NULL);

	// Yield so that started tasks can do their thing.
	vTaskDelay(100 / portTICK_PERIOD_MS);

	// Resume the startup task.
	event_start();

	print_system_state();
}

void print_system_state()
{
	unsigned num = uxTaskGetNumberOfTasks();
	TaskStatus_t *buffer = malloc(sizeof(TaskStatus_t) * num);
	if (buffer == NULL) {
		printf("Error: unable to allocate buffer for system task info\n");
		return;
	}
	uxTaskGetSystemState(buffer, num, NULL);
	printf("Base     Margin   Name\n");
	printf("======================\n");
	size_t t;
	for (t = 0; t < num; ++t) {
		printf("%8p %8lu %s\n", buffer[t].pxStackBase,
			buffer[t].usStackHighWaterMark, buffer[t].pcTaskName);
	}
	free(buffer);
}
