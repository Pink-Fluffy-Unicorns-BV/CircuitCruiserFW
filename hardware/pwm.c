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
 * # NAME       = PWM driver                        	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 2025-03-12                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/ledc.h>

#include <event.h>

int SET_PWM;
static const int num_channels = LEDC_CHANNEL_MAX;

static ledc_channel_config_t channel_config[LEDC_CHANNEL_MAX];

void pwm_init(QueueHandle_t queue)
{
	// configure timers.

	// Use a separate higher frequency timer for the motor.
	ledc_timer_config_t motor_config = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = LEDC_TIMER_14_BIT,
		.timer_num = LEDC_TIMER_0,
		.freq_hz = 1000,
		.clk_cfg = LEDC_AUTO_CLK,
		.deconfigure = false
	};
	ledc_timer_config(&motor_config);

	ledc_timer_config_t config = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = LEDC_TIMER_14_BIT,
		.timer_num = LEDC_TIMER_1,
		.freq_hz = 1000 / 20,	// 20 ms, compatible with many servo motors.
		.clk_cfg = LEDC_AUTO_CLK,
		.deconfigure = false
	};
	ledc_timer_config(&config);

	int c;
	for (c = 0; c < num_channels; ++c) {
		channel_config[c].gpio_num = -1;
		channel_config[c].speed_mode = LEDC_LOW_SPEED_MODE;
		channel_config[c].channel = LEDC_CHANNEL_0 + c;
		channel_config[c].intr_type = LEDC_INTR_DISABLE;
		channel_config[c].timer_sel = c == 0 ? LEDC_TIMER_0 : LEDC_TIMER_1;
		channel_config[c].duty = 0;
		channel_config[c].hpoint = 0;
		channel_config[c].sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
		channel_config[c].flags.output_invert = 0;
	}

	SET_PWM = event_new("pwm",
		(const char *[6]) { "channel", "pin", "on", NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(SET_PWM, true, queue);
}

bool pwm_event(Event *event)
{
	if (event->eventcode != SET_PWM)
		return false;
	int channel = event->i[0];
	if (channel < 0) {
		printf(_("Invalid pwm channel %d addressed (max is %d).\n"), channel,
			num_channels);
		return true;
	}
	int pin = event->i[1];
	int on = event->i[2];
	event_free(event);

	if (channel_config[channel].gpio_num >= 0) {
		// PWM already active.
		if (pin < 0) {
			ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
			gpio_reset_pin(channel_config[channel].gpio_num);
			channel_config[channel].gpio_num = -1;
			return true;
		}
		// Change frequency

		ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, on);
		ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
		return true;
	}

	// PWM not active yet.
	if (pin < 0) {
		ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
		gpio_reset_pin(channel_config[channel].gpio_num);
		channel_config[channel].gpio_num = -1;
		return true;
	}

	channel_config[channel].duty = on;
	channel_config[channel].gpio_num = pin;
	ledc_channel_config(&channel_config[channel]);
	//print_system_state();
	return true;
}
