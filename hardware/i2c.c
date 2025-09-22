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
 * # NAME       = I2C driver                        	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 2025-07-07                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "driver/i2c_master.h"

#include <event.h>

#define MAX_I2C_DEVICES 10
typedef struct i2c_dev {
	i2c_master_dev_handle_t handle;
	int timeout;
} i2c_dev;
static i2c_dev i2c_devices[MAX_I2C_DEVICES] = { NULL, };
static int I2C_NEW, I2C_READ, I2C_WRITE;

static i2c_master_bus_config_t i2c_mst_config = {
	.clk_source = I2C_CLK_SRC_DEFAULT,
	.i2c_port = -1,
	.scl_io_num = 9,
	.sda_io_num = 3,
	.glitch_ignore_cnt = 7,
	.flags.enable_internal_pullup = true,
};

static i2c_master_bus_handle_t bus_handle;

void i2c_init(QueueHandle_t queue)
{
	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
	//printf(_("new i2c device registered!\n"));
	// Register a new i2c device.
	I2C_NEW = event_new("i2c_new",
		(const char *[6]) { "addr", "speed", "timeout", "reply", NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(I2C_NEW, true, queue);

	// Read from a registered i2c device.
	I2C_READ = event_new("i2c_read",
		(const char *[6]) { "target", "reply", NULL, NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(I2C_READ, true, queue);

	// Write to a registered i2c device.
	I2C_WRITE = event_new("i2c_write", (const char *[6])
	{
		"target", "data0", "data1", "data2", "data3", "data4"
	},
		(const char *[3]) { NULL, NULL, NULL },
		(const char *[3]) { NULL, NULL, NULL });
	event_claim(I2C_WRITE, true, queue);
}

static void reply_event(int id, int ret)
{
	Event revent = {
		.eventcode = id,
		.i = {ret,},
	};
	event_send(&revent);
}

bool i2c_event(Event *event)
{
	if (event->eventcode == I2C_NEW) {
		i2c_device_config_t dev_cfg = {
			.dev_addr_length = I2C_ADDR_BIT_LEN_7,
			.device_address = event->i[0],
			.scl_speed_hz = event->i[1],
		};
		int dev;
		for (dev = 0; dev < MAX_I2C_DEVICES; ++dev) {
			if (i2c_devices[dev].handle == NULL)
				break;
		}
		if (dev == MAX_I2C_DEVICES) {
			// Failed.
			reply_event(event->i[3], -1);
			return true;
		}

		ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg,
			&i2c_devices[dev].handle));
		i2c_devices[dev].timeout = event->i[2];
		reply_event(event->i[3], dev);
		return true;
	} else if (event->eventcode == I2C_READ) {
		uint32_t target = (uint32_t)event->i[0];
		uint8_t reg = target >> 24;
		uint8_t num = target >> 16;
		uint8_t bytes = target >> 8;
		uint8_t dev = target;
		int reply = event->i[1];
		if (dev >= MAX_I2C_DEVICES || num < 1 || num > 6 || bytes > 4) {
			printf("Invalid i2c data; not reading.\n");
			return true;
		}

		uint8_t data[24];

		ESP_ERROR_CHECK(i2c_master_transmit_receive(i2c_devices[dev].handle,
			&reg, 1, data, num * bytes, i2c_devices[dev].timeout));

		// printf(_("i2c read event, %x,%x,%x,%x,%x,%x,%x,%x,%x,%x -> %d\n"),
		// 	data[0], data[1], data[2], data[3], data[4],
		// 	data[5], data[6], data[7], data[8], data[9], reply);
		if (reply >= 0) {
			Event revent = { .eventcode = reply, };
			for (int i = 0; i < num; ++i) {
				revent.i[i] = 0;
				for (int b = 0; b < bytes; ++b)
					revent.i[i] |= data[i * bytes + b] << (8 * b);
			}
			event_send(&revent);
		}
	} else if (event->eventcode == I2C_WRITE) {
		uint32_t target = (uint32_t)event->i[0];
		uint8_t reg = target >> 24;
		uint8_t num = target >> 16;
		uint8_t bytes = target >> 8;
		uint8_t dev = target;
		if (dev >= MAX_I2C_DEVICES || num > 5 || bytes > 4) {
			printf("Invalid i2c data; not writing.\n");
			return true;
		}
		uint8_t data[num * bytes + 1];
		data[0] = reg;
		for (int i = 0; i < num; ++i) {
			for (int b = 0; b < bytes; ++b)
				data[i * bytes + b + 1] = event->i[i + 1] >> (8 * b);
		}
		// printf(_("i2c write event, %x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n"),
		// 	data[0], data[1], data[2], data[3], data[4],
		// 	data[5], data[6], data[7], data[8], data[9]);
		ESP_ERROR_CHECK(i2c_master_transmit(i2c_devices[dev].handle, data,
			num * bytes + 1, i2c_devices[dev].timeout));
		return true;
	}
	return false;
}
