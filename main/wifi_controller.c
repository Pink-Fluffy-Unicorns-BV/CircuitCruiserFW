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
 * # NAME       = Wifi handling		                	                     #
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
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_rom_crc.h>

#include <lwip/err.h>
#include <lwip/sys.h>

#include "event.h"
#include "webserver.h"

 /* WiFi Soft AP configuration */
#define WIFI_SSID              "Circuit Cruiser"
#define WIFI_CHANNEL           5
#define WIFI_MAX_STA_CONN      10

static const char *TAG = "WIFI_SOFT_AP";

void wifi_event_handler(void *arg, esp_event_base_t event_base,
	int32_t event_id, void *event_data)
{
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t *event
			= (wifi_event_ap_staconnected_t *)event_data;
		ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
			MAC2STR(event->mac), event->aid);
		wifi_connected();
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t *event
			= (wifi_event_ap_stadisconnected_t *)event_data;
		ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
			MAC2STR(event->mac), event->aid, event->reason);
		wifi_disconnected();
	}
}

void wifi_init_softap(void)
{
	uint8_t mac[6];
	esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&wifi_event_handler,
		NULL,
		NULL));

	wifi_config_t wifi_config = {
		.ap = {
			.ssid = {0,},
			.ssid_len = 0,
			.channel = WIFI_CHANNEL,
			.max_connection = WIFI_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
			.authmode = WIFI_AUTH_WPA3_PSK,
			.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
			.authmode = WIFI_AUTH_WPA2_PSK,
#endif
			.pmf_cfg = {
					.required = true,
			},
		},
	};
	wifi_config.ap.ssid_len = snprintf(
		(char *)wifi_config.ap.ssid,
		sizeof(wifi_config.ap.ssid),
		WIFI_SSID " %02X%02X%02X%02X",
		mac[2], mac[3], mac[4], mac[5]);
	assert(wifi_config.ap.ssid_len >= 0);

	uint32_t crc = esp_rom_crc32_le(0, wifi_config.ap.ssid,
		wifi_config.ap.ssid_len);
	snprintf((char *)wifi_config.ap.password, sizeof(wifi_config.ap.password),
		"%08lx", crc);
	if (strlen((char *)wifi_config.ap.password) == 0)
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
		wifi_config.ap.ssid, wifi_config.ap.password, WIFI_CHANNEL);

	set_wifi_info((char *)wifi_config.ap.ssid, (char *)wifi_config.ap.password);
}

void start_wifi(void)
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
		|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
	wifi_init_softap();
}
