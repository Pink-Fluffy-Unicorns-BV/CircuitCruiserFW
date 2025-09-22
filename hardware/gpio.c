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
 * # NAME       = Gpio driver			                	                 #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser                         #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

 #include "gpio.h"
#include <driver/gpio.h>
#include <event.h>

int SET_PIN;
int GET_PIN;

static int pin_event[GPIO_PIN_COUNT];

void gpio_init(QueueHandle_t queue)
{
	SET_PIN = event_new("set_pin",
		(const char *[6]) { "pin", "mode", "event", NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(SET_PIN, true, queue);

	GET_PIN = event_new("get_pin",
		(const char *[6]) { "pin", "event", NULL, NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(GET_PIN, true, queue);

	const char *constants[]
		= { "LOW", "HIGH", "FLOATING", "PULLUP", "PULLDOWN",
			"RISING", "FALLING", "CHANGE", NULL };
	set_lua_constants("gpio", constants);
	gpio_install_isr_service(0);
	for (int i = 0; i < GPIO_PIN_COUNT; ++i)
		pin_event[i] = -1;
}

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
	int pin = (int)args;
	//printf("interrupt for pin %d\n", pin);
	if (pin_event[pin] < 0)
		return;

	Event event = {
		.eventcode = pin_event[pin],
	};
	event.i[0] = pin;
	if (event.eventcode >= 1 && event.eventcode < MAX_EVENTS) {
		EventType *def = &event_defs[event.eventcode];
		if (def->name != NULL && def->queue != NULL) {
			xQueueSendFromISR(def->queue, &event, NULL);
		}
	}
}

static bool register_interrupt(int pin, bool rising, bool falling, int e)
{
	if (pin < 0 || pin >= GPIO_PIN_COUNT || pin_event[pin] >= 0 || e < 0 ||
		(!rising && !falling)) {
			printf("not really; event %d, e %d\n", pin_event[pin], e);
		return false;
	}

	//gpio_pad_select_gpio(pin);
	gpio_set_direction(pin, GPIO_MODE_INPUT);

	if (falling) {
		gpio_pullup_en(pin);
		gpio_pulldown_dis(pin);
		if (rising)
			gpio_set_intr_type(pin, GPIO_INTR_ANYEDGE);
		else
			gpio_set_intr_type(pin, GPIO_INTR_NEGEDGE);
	} else {
		gpio_pulldown_en(pin);
		gpio_pullup_dis(pin);
		gpio_set_intr_type(pin, GPIO_INTR_POSEDGE);
	}

	gpio_isr_handler_add(pin, gpio_interrupt_handler, (void *)pin);
	pin_event[pin] = e;
	return true;
}

bool gpio_event(Event *event)
{
	if (event->eventcode == GET_PIN) {
		int pin = event->i[0];
		int cb = event->i[1];
		int value;
		if (pin < 0 || pin >= GPIO_PIN_COUNT) {
			// invalid pin.
			value = -1;
		} else {
			value = gpio_get_level(pin);
		}
		Event event = { .eventcode = cb, };
		event.i[0] = value;
		event_send(&event);
		return true;
	} else if (event->eventcode == SET_PIN) {
		int pin = event->i[0];
		int mode = event->i[1];
		int e = event->i[2];	// Only used for interrupts.
		//printf(_("dbg: gpio event %d for pin %d\n"), mode, pin);
		event_free(event);
		switch (mode) {
		case GPIO_LOW:
			gpio_set_direction(pin, GPIO_MODE_OUTPUT);
			gpio_set_level(pin, 0);
			gpio_set_pull_mode(pin, GPIO_FLOATING);
			break;
		case GPIO_HIGH:
			gpio_set_direction(pin, GPIO_MODE_OUTPUT);
			gpio_set_level(pin, 1);
			gpio_set_pull_mode(pin, GPIO_FLOATING);
			break;
		case GPIO_FLOAT:
			gpio_set_direction(pin, GPIO_MODE_DISABLE);
			gpio_set_pull_mode(pin, GPIO_FLOATING);
			break;
		case GPIO_PULLUP:
			gpio_set_direction(pin, GPIO_MODE_INPUT);
			gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
			break;
		case GPIO_PULLDOWN:
			gpio_set_direction(pin, GPIO_MODE_INPUT);
			gpio_set_pull_mode(pin, GPIO_PULLDOWN_ONLY);
			break;
		case GPIO_RISING:
			register_interrupt(pin, true, false, e);
			break;
		case GPIO_FALLING:
			register_interrupt(pin, false, true, e);
			break;
		case GPIO_CHANGE:
			register_interrupt(pin, true, true, e);
			break;
		default:
			// Unknown event.
			printf(_("unknown gpio event %d for pin %d\n"), mode, pin);
		}
		return true;
	}
	return false;
}
