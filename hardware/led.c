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
 * # NAME       = Neopixel LED driver                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

#include <neopixel.h>
#include <event.h>

#define NEOPIXEL_PIN GPIO_NUM_4

static tNeopixelContext neopixel;
static int SET_LED;
static int num_pixels;

void led_init(QueueHandle_t queue)
{
	num_pixels = 12;
	neopixel = neopixel_Init(num_pixels, NEOPIXEL_PIN);
	SET_LED = event_new("set_LED",
		(const char *[6]) { "pixel", "red", "green", "blue", NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(SET_LED, true, queue);
	//printf(_("set LED event code: %d\n"), SET_LED);
}

bool led_event(Event *event)
{
	if (event->eventcode != SET_LED)
		return false;
	int index = event->i[0];
	if (index == -1) {
		// Special case: update LEDs without changing any.
		neopixel_SetPixel(neopixel, NULL, 0);
		return true;
	}
	if (index < 0) {
		printf(_("Invalid pixel %d addressed; maximum is %d.\n"),
			index, num_pixels);
		return true;
	}
	if (index > num_pixels) {
		if (neopixel != NULL)
			neopixel_Deinit(neopixel);
		num_pixels = index + 1;
		neopixel = neopixel_Init(num_pixels, NEOPIXEL_PIN);
	}
	int red = event->i[1];
	int green = event->i[2];
	int blue = event->i[3];
	event_free(event);
	tNeopixel pixel = { .index = index, .rgb = NP_RGB(red, green, blue) };
	//printf(_("Set LED %d to %d, %d, %d\n"), index, red, green, blue);
	neopixel_SetPixel(neopixel, &pixel, 1);
	return true;
}
