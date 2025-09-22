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
 * # NAME       = Motor driver		                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 27-7-2025	                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <event.h>
#include "motor.h"
#include "gpio.h"
#include "pwm.h"

const char *TAG = "motor";

void motor_task(void *args);

static const TickType_t interval = 150 / portTICK_PERIOD_MS;
// Compute this in from interval to use actual value, not requested value.
static const int interval_ms = interval * portTICK_PERIOD_MS;
static const int safe_step = 10;

// Note: Motor pins are reversed compared to data sheet.
static const int PIN2 = 38;
static const int PIN1 = 48;
static int MOTOR;
static volatile int target_on_time;
static volatile int step_per_iteration;

void motor_init(QueueHandle_t queue)
{
	target_on_time = 0;
	step_per_iteration = 0;

	// Register event for motor controls.
	MOTOR = event_new("motor",
		(const char *[6]) { "power", "time", NULL, NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(MOTOR, true, queue);

	xTaskCreate(&motor_task, "motor", 4096, NULL, 0, NULL);
}

bool motor_event(Event *event)
{
	if (event->eventcode != MOTOR)
		return false;

	// Pass values to motor task through volatile variables.
	// Power range is -256 to +256.
	target_on_time = event->i[0];
	//ESP_LOGI(TAG, "new target on-time: %d", target_on_time);
	int full_time = event->i[1];
	// Full range is F steps.
	// This should be reached in full_time ms.
	// So we need F / full_time steps/ms.
	// There are interval ms / iteration.
	// So F * interval / full_time steps per iteration.
	step_per_iteration = (interval_ms << 8) / full_time;
	// Make it at least 1 to make sure it always does something.
	// TODO: this makes it too fast; should be fixed.
	if (step_per_iteration == 0)
		step_per_iteration = 1;
	//ESP_LOGI(TAG, "steps per iteration: %d", step_per_iteration);

	return true;
}

void motor_task(void * /*args*/)
{
	// Monitor and adjust motor.
	int on_time = 0;

	bool pwm_active = false;
	TickType_t previous = xTaskGetTickCount();
	Event event1;
	Event event2;
	for (int i = 0; i < 3; ++i) {
		event1.s[i] = NULL;
		event2.s[i] = NULL;
	}

	vTaskDelay(0);	// Allow the watchdog to be refreshed.
	adc_cali_curve_fitting_config_t cali_config = {
		.unit_id = ADC_UNIT_1,
		.atten = ADC_ATTEN_DB_0,
		.bitwidth = ADC_BITWIDTH_12,
	};
	adc_cali_handle_t cali_handle;
	adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);

	vTaskDelay(0);	// Allow the watchdog to be refreshed.
	adc_oneshot_unit_init_cfg_t oneshot_config = {
		.unit_id = ADC_UNIT_1,
		.clk_src = 0,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	adc_oneshot_unit_handle_t oneshot_handle;
	adc_oneshot_new_unit(&oneshot_config, &oneshot_handle);
	adc_oneshot_chan_cfg_t channel_config = {
		.bitwidth = ADC_BITWIDTH_12,
		.atten = ADC_ATTEN_DB_0,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(oneshot_handle, ADC_CHANNEL_6,
		&channel_config));
	while (true) {
		int s = 0;
		while (xTaskDelayUntil(&previous, interval) == pdFALSE) { ++s; }
		if (s > 0)
			ESP_LOGI(TAG, "skipped %d steps", s);

		// We have target_on_time and step_per_iteration.
		// Before every iteration, the motor current is measured.
		// If it is invalid, the current is lowered by safe_step and
		// the target is ignored, unless the target is lower and the
		// step for reaching the target is higher than safe_step.

		int step;
		if (on_time > target_on_time) {
			if (on_time - target_on_time >= step_per_iteration)
				step = -step_per_iteration;
			else
				step = -(on_time - target_on_time);
		} else if (on_time < target_on_time) {
			if (target_on_time - on_time >= step_per_iteration)
				step = step_per_iteration;
			else
				step = target_on_time - on_time;
		} else {
			// on_time is what it should be.
			step = 0;
		}

		// Measure current power.
		int mV;
		vTaskDelay(0);	// Allow the watchdog to be refreshed.
		adc_oneshot_get_calibrated_result(oneshot_handle, cali_handle,
			ADC_CHANNEL_6, &mV);
		vTaskDelay(0);	// Allow the watchdog to be refreshed.
		if (on_time < 0)
			mV = -mV;

		// The signal is 369 mV / A. The threshold is 1.25 A, so that
		// corresponds to 461 mV.
		if (mV > 461) {
			// Limit the current (forward).
			if (safe_step > -step)
				step = -safe_step;
			// Breaking should not go further than 0.
			if (-step > on_time)
				step = -on_time;
		} else if (mV < -461) {
			// Limit the current (backward).
			if (safe_step > step)
				step = safe_step;
			// Breaking should not go further than 0.
			if (step > -on_time)
				step = on_time;
		}

		if (step == 0)
			continue;

		on_time += step;

		//ESP_LOGI(TAG, "new on-time: %d / %d", on_time, target_on_time);

		if (on_time > 0) {
			if (on_time > 0x100)
				on_time = 0x100;
			event1.eventcode = SET_PIN;
			event1.i[0] = PIN1;
			event1.i[1] = GPIO_HIGH;

			event2.eventcode = SET_PWM;
			event2.i[0] = 0;
			event2.i[1] = PIN2;
			event2.i[2] = (0xff - on_time) << 6;
			pwm_active = true;
		} else if (on_time < 0) {
			if (on_time < -0x100)
				on_time = -0x100;
			event1.eventcode = SET_PWM;
			event1.i[0] = 0;
			event1.i[1] = PIN1;
			event1.i[2] = (0xff - -on_time) << 6;
			pwm_active = true;

			event2.eventcode = SET_PIN;
			event2.i[0] = PIN2;
			event2.i[1] = GPIO_HIGH;
		} else {
			// on_time == 0.
			if (pwm_active) {
				pwm_active = false;
				event1.eventcode = SET_PWM;
				event1.i[0] = 0;
				event1.i[1] = -1;
				event_send(&event1);
			}

			event1.eventcode = SET_PIN;
			event1.i[0] = PIN1;
			event1.i[1] = GPIO_HIGH;

			event2.eventcode = SET_PIN;
			event2.i[0] = PIN2;
			event2.i[1] = GPIO_HIGH;
		}
		event_send(&event1);
		event_send(&event2);
	}
}
