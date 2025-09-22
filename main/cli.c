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
 * # NAME       = Commandline handler                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 29-1-2025		                      	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

#include <stdio.h>
#include <event.h>
#include <soc/soc_caps.h>
#include <driver/uart.h>
#include <driver/usb_serial_jtag.h>
#include <driver/usb_serial_jtag_vfs.h>
#include "cli.h"

#define LINE_SIZE 500

static void monitor_task(ScriptTask *self)
{
	while (true) {
		Event event;
		if (!event_wait(-1, &event, self->queue)) {
			// Should not happen.
			printf(_("cli event timed out?!\n"));
			continue;
		}
		char *buffer = print_event(&event);
		event_free(&event);
		printf("Event received: %s\n", buffer);
		free(buffer);
	}
}

void cli_reply_cb(const char *msg, size_t size, void * /*user_data*/)
{
	if (msg[size - 1] == '\n')
		printf("%s", msg);
	else
		printf("%s\n", msg);
}

void cli_task(void * /*arg*/)
{
	// Configure a temporary buffer for the incoming data
	char *line = malloc(LINE_SIZE);
	if (line == NULL) {
		printf("Unable to allocate cli buffer; disabling cli.\n");
		return;
	}

	setvbuf(stdout, NULL, _IONBF, 0);

	usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
		.rx_buffer_size = LINE_SIZE,
		.tx_buffer_size = LINE_SIZE,
	};

	usb_serial_jtag_driver_install(&usb_serial_jtag_config);
	usb_serial_jtag_vfs_use_driver();

	ScriptTask *self = create_lua_task();

	// Use a separate task to monitor incoming events.
	xTaskCreate((TaskFunction_t)&monitor_task, "cli-monitor", 2048, self,
		0, NULL);

	int p = 0;
	while (true) {
		int c = getchar();
		if (c == EOF) {
			printf(_("reading usb serial line failed!\n"));
			vTaskDelay(pdMS_TO_TICKS(500));
			continue;
		}
		if (c == '\x08' || c == '\x7f') {
			printf("\x08 \x08");
			--p;
			continue;
		}
		printf("%c", c);
		if (c != '\n') {
			line[p++] = c;
			if (p >= LINE_SIZE) {
				printf(_("CLI buffer overflow\n"));
				p = 0;
			}
			continue;
		}
		line[p] = '\0';
		p = 0;
		//printf(_("running command %s\n"), line);
		run_lua_command(self, line, cli_reply_cb, NULL);
	}
}
