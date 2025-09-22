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
 * # NAME       = HTTP server		                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

 /* HTTP Restful API Server

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
 */
#include <string.h>
#include <fcntl.h>
#include <esp_http_server.h>
#include <esp_chip_info.h>
#include <esp_random.h>
#include <esp_log.h>
#include <esp_vfs.h>
#include <cJSON.h>
#include <sdkconfig.h>
#include <driver/gpio.h>
 // #include <esp_vfs_semihost.h>
#include <esp_vfs_fat.h>
#include <esp_spiffs.h>
#include <sdmmc_cmd.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <sys/param.h>
#include <esp_netif.h>

#include <mdns.h>
#include <lwip/apps/netbiosns.h>

#include "event.h"

#define MAX_POST_SIZE (1024 * 16)

static ScriptTask *lua_context;
static httpd_handle_t websocket_hd;
static int websocket_fd;
static void lua_reply(const char *msg, size_t size, void *user_data);
// private prototypes

esp_err_t wss_luaJS_handler(uint8_t x, uint8_t y);

static const char *WEB_TAG = "esp-webserver";

#define REST_CHECK(a, str, goto_tag, ...) \
    do { \
        if (!(a)) { \
            ESP_LOGE(WEB_TAG, "%s(%d): " str, \
				__FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag; \
        } \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
	char base_path[ESP_VFS_PATH_MAX + 1];
	char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
	httpd_handle_t hd;
	int fd;
};

#define CHECK_FILE_EXTENSION(filename, ext) \
	(strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)
#define MDNS_INSTANCE "esp home web server"

static void wss_monitor_task(ScriptTask *self);
static esp_err_t start_server(const char *base_path);
static esp_err_t wss_message_handler(httpd_req_t *req);
static esp_err_t rest_common_get_handler(httpd_req_t *req);
static const char *parse_query(const char *uri, const char *key, size_t *size);
static esp_err_t api_file_list_handler(httpd_req_t *req);
static esp_err_t api_delete_handler(httpd_req_t *req);
static esp_err_t api_send_file_handler(httpd_req_t *req);
static esp_err_t set_content_type_from_file(httpd_req_t *req,
	const char *filepath);
static void initialise_mdns(void);

void wifi_connected(void)
{
	run_lua_command(lua_context, "wifi_connected()", NULL, NULL);
}

void wifi_disconnected(void)
{
	run_lua_command(lua_context, "wifi_disconnected()", NULL, NULL);
}

void initialize_spiffs(void)
{
	ESP_LOGI(WEB_TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
	  .base_path = "/www",
	  .partition_label = NULL,
	  .max_files = 5,
	  .format_if_mount_failed = false
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is an all-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(WEB_TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(WEB_TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(WEB_TAG, "Failed to initialize SPIFFS (%s)",
				esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(WEB_TAG, "Failed to get SPIFFS partition information (%s). "
			"Serious ERROR", esp_err_to_name(ret));
		// esp_spiffs_format(conf.partition_label);
		return;
	} else {
		ESP_LOGI(WEB_TAG, "Partition size: total: %d, used: %d", total, used);
	}
}

static void initialise_mdns(void)
{
	mdns_init();
	mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
	mdns_instance_name_set(MDNS_INSTANCE);

	mdns_txt_item_t serviceTxtData[] = {
		{"board", "esp32s3"},
		{"path", "/"}
	};

	ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80,
		serviceTxtData, sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req,
	const char *filepath)
{
	const char *type = "text/plain";
	if (CHECK_FILE_EXTENSION(filepath, ".html")) {
		type = "text/html";
	} else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
		type = "application/javascript";
	} else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
		type = "text/css";
	} else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
		type = "image/png";
	} else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
		type = "image/x-icon";
	} else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
		type = "image/svg+xml";
	}
	return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
	char filepath[FILE_PATH_MAX];

	rest_server_context_t *rest_context
		= (rest_server_context_t *)req->user_ctx;
	strlcpy(filepath, rest_context->base_path, sizeof(filepath));
	if (req->uri[strlen(req->uri) - 1] == '/') {
		strlcat(filepath, "/index.html", sizeof(filepath));
	} else {
		strlcat(filepath, req->uri, sizeof(filepath));
	}
	int fd = open(filepath, O_RDONLY, 0);
	if (fd == -1) {
		ESP_LOGE(WEB_TAG, "Failed to open file : %s", filepath);
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
			"Failed to read existing file");
		return ESP_FAIL;
	}

	set_content_type_from_file(req, filepath);

	char *chunk = rest_context->scratch;
	ssize_t read_bytes;
	do {
		/* Read file in chunks into the scratch buffer */
		read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
		if (read_bytes == -1) {
			ESP_LOGE(WEB_TAG, "Failed to read file : %s", filepath);
		} else if (read_bytes > 0) {
			/* Send the buffer contents as HTTP response chunk */
			if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
				close(fd);
				ESP_LOGE(WEB_TAG, "File %s sending failed! (len %d)", filepath,
					req->content_len);
				/* Abort sending file */
				httpd_resp_sendstr_chunk(req, NULL);
				/* Respond with 500 Internal Server Error */
				httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
					"Failed to send file");
				return ESP_FAIL;
			}
		}
	} while (read_bytes > 0);
	/* Close file after sending complete */
	close(fd);
	ESP_LOGI(WEB_TAG, "File %s sending complete", filepath);
	/* Respond with an empty chunk to signal HTTP response completion */
	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

#if 0
/* Simple handler for light brightness control */
static esp_err_t light_brightness_post_handler(httpd_req_t *req)
{
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
			"content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
				"Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';

	cJSON *root = cJSON_Parse(buf);
	int red = cJSON_GetObjectItem(root, "red")->valueint;
	int green = cJSON_GetObjectItem(root, "green")->valueint;
	int blue = cJSON_GetObjectItem(root, "blue")->valueint;
	ESP_LOGI(WEB_TAG, "Light control: red = %d, green = %d, blue = %d",
		red, green, blue);
	cJSON_Delete(root);
	httpd_resp_sendstr(req, "Post control value successfully");
	return ESP_OK;
}
#endif

static const char *parse_query(const char *uri, const char *key, size_t *size)
{
	size_t len = strlen(key);
	char *query = strchrnul(uri, '?');
	while (*query != '\0') {
		++query;
		char *next = strchrnul(query, '&');
		if (strncmp(query, key, len) == 0 && query[len] == '=') {
			// Found the key.
			if (size != NULL)
				*size = next - query - (len + 1);
			return &query[len + 1];
		}
		query = next;
	}
	ESP_LOGI(WEB_TAG, "invalid query key not found");
	return NULL;
}

// Parse the header of a POST request. Entire header must fit in buffer.
static bool parse_post_header(const char *body, size_t chunk_size,
	size_t *boundary_size,
	char **filename, size_t *name_size,
	size_t *header_size)
{
	*name_size = 0;
	*header_size = 0;
	//printf("chunk (%d): %.*s\n", chunk_size, chunk_size, body);
	char *eol = strnstr(body, "\r\n", chunk_size);
	if (eol == NULL) {
		// No data received.
		*boundary_size = 0;
		return false;
	}
	// Boundary is from body to eol.
	*boundary_size = eol - body;
	size_t pos = *boundary_size + 2;
	//printf("boundary: %.*s\n", *boundary_size, body);
	while (true) {
		eol = strnstr(&body[pos], "\r\n", chunk_size - pos);
		//printf("eol: %p\n", eol);
		if (eol == NULL) {
			printf("invalid post header");
			return false;
		}
		size_t line_size = (eol - body) - pos;
		if (line_size == 0) {
			// End of header.
			*header_size = pos + 2;
			//printf("name size: %d\n", *name_size);
			return *name_size > 0;
		}
		// Header line received.
		//printf("Header line: %.*s\n", line_size, &body[pos]);
		// Dirty hack: only check for filename; ignore everythig else.
		char *fname =
			strnstr(&body[pos], "filename=\"", chunk_size - pos);
		if (fname != NULL) {
			char *end = strnstr(&fname[10], "\"", eol - fname - 10);
			if (end != NULL) {
				// File name found.
				*filename = &fname[10];
				*name_size = (end - fname) - 10;
			}
		}
		pos += line_size + 2;
	}
}

static esp_err_t reply_error(httpd_req_t *req)
{
	static const char error_reply[] = "{\"error\":\"invalid request\"}";
	httpd_resp_set_type(req, "text/plain");
	httpd_resp_sendstr(req, error_reply);
	return ESP_OK;
}

static esp_err_t reply_ok(httpd_req_t *req)
{
	static const char error_reply[] = "{\"result\":\"success\"}";
	httpd_resp_set_type(req, "text/plain");
	httpd_resp_sendstr(req, error_reply);
	return ESP_OK;
}

static esp_err_t api_file_list_handler(httpd_req_t *req)
{
	size_t dirlen;
	const char *folder = parse_query(req->uri, "folder", &dirlen);
	if (folder == NULL)
		return reply_error(req);
	// Add nul-terminator and apply prefix to folder name.
	char fbuf[dirlen + 6];
	snprintf(fbuf, sizeof(fbuf), "/www/%s", folder);

	//printf("opening folder: %s\n", fbuf);

	DIR *dir = opendir(fbuf);
	if (dir == NULL)
		return reply_error(req);

	struct dirent *file;

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();
	cJSON *files = cJSON_AddArrayToObject(root, "files");

	while (true) {
		file = readdir(dir);
		if (file == NULL)
			break;

		// This never happens on this file system, but a check does not hurt.
		if (file->d_type & DT_DIR) {
			printf("Skipping directory\n");
			continue;
		}

		struct stat data;
		size_t namelen = strlen(file->d_name);
		char buf[namelen + dirlen + 10];
		sprintf(buf, "%s/%s", fbuf, file->d_name);
		errno = 0;
		if (stat(buf, &data) < 0) {
			printf("failed to stat file %s: %s\n", buf, strerror(errno));
			continue;
		}

		cJSON *target = files;
		cJSON *entry = cJSON_CreateObject();
		cJSON_AddItemToArray(target, entry);
		cJSON_AddStringToObject(entry, "name", file->d_name);
		cJSON_AddNumberToObject(entry, "size", data.st_size);
		//printf("adding file: %s\n", file->d_name);
	}

	const char *reply = cJSON_Print(root);
	httpd_resp_sendstr(req, reply);
	free((void *)reply);
	cJSON_Delete(root);
	return ESP_OK;
}

static esp_err_t api_delete_handler(httpd_req_t *req)
{
	size_t size;
	const char *name = parse_query(req->uri, "file", &size);
	if (name == NULL)
		return reply_error(req);

	// Add nul-terminator and apply prefix to folder name.
	char buf[size + 10];
	snprintf(buf, size + 9, "/www/%s", name);
	printf("deleting: %s\n", buf);

	if (unlink(buf) < 0)
		return reply_error(req);
	return reply_ok(req);
}

static esp_err_t api_send_file_handler(httpd_req_t *req)
{
	int remaining = req->content_len;
	char *buf = malloc(MAX_POST_SIZE);
	if (buf == NULL) {
		printf("Unable to allocate buffer for POST\n");
		free(buf);
		return reply_error(req);
	}
	size_t pos = 0;
	char *filename;
	size_t boundary_size;
	size_t name_size;
	size_t header_size;
	while (remaining > 0 && pos < MAX_POST_SIZE) {
		int ret = httpd_req_recv(req, &buf[pos], MIN(remaining, MAX_POST_SIZE));
		if (ret < 0) {
			printf("Unable to receive POST data.\n");
			free(buf);
			return reply_error(req);
		}
		pos += ret;
		remaining -= ret;
	}
	bool ret = parse_post_header(buf, pos, &boundary_size, &filename,
		&name_size, &header_size);
	if (!ret) {
		// POST failed.
		printf("Failed to parse POST request.\n");
		free(buf);
		return reply_error(req);
	}
	char name[50];
	snprintf(name, sizeof(name), "/www/%.*s", name_size, filename);
	char boundary[boundary_size + 1];
	memcpy(boundary, buf, boundary_size);
	boundary[boundary_size] = '\0';
	const char *e = strnstr(&buf[header_size], boundary, pos - header_size);
	if (e == NULL) {
		printf("No final boundary found");
		free(buf);
		return reply_error(req);
	}
	size_t data_size = (e - buf) - header_size;
	FILE *f = fopen(name, "w");
	if (f == NULL) {
		printf("Unable to open file '%s'.\n", name);
		free(buf);
		return reply_error(req);
	}
	fprintf(f, "%.*s", data_size, &buf[header_size]);
	fclose(f);
	free(buf);
	return reply_ok(req);
}

static esp_err_t wss_message_handler(httpd_req_t *req)
{
	// Store last request struct for asynchronous events.
	websocket_hd = req->handle;
	websocket_fd = httpd_req_to_sockfd(req);

	if (req->method == HTTP_GET) {
		ESP_LOGI(WEB_TAG, "Handshake done, the new connection was opened");
		return ESP_OK;
	}
	httpd_ws_frame_t ws_pkt;
	char *buf = NULL;
	memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
	ws_pkt.type = HTTPD_WS_TYPE_TEXT;
	/* Set max_len = 0 to get the frame len */
	esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
	if (ret != ESP_OK) {
		ESP_LOGE(WEB_TAG, "httpd_ws_recv_frame failed to get frame len with %d",
			ret);
		return ret;
	}
	//ESP_LOGI(WEB_TAG, "frame len is %d", ws_pkt.len);
	if (ws_pkt.len) {
		/* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
		buf = calloc(1, ws_pkt.len + 1);
		if (buf == NULL) {
			ESP_LOGE(WEB_TAG, "Failed to calloc memory for buf");
			return ESP_ERR_NO_MEM;
		}
		ws_pkt.payload = (uint8_t *)buf;
		/* Set max_len = ws_pkt.len to get the frame payload */
		ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
		if (ret != ESP_OK) {
			ESP_LOGE(WEB_TAG, "httpd_ws_recv_frame failed with %d", ret);
			free(buf);
			return ret;
		}
		//ESP_LOGI(WEB_TAG, "Got packet with message: %s", ws_pkt.payload);
		vTaskDelay(0);	// Allow wdt to be fed.
		/* Place code to start here for doing something with the websocket packet */
	}
	//ESP_LOGI(WEB_TAG, "Packet type: %d", ws_pkt.type);

	run_lua_command(lua_context, buf, lua_reply, req);
	free(buf);
	return ESP_OK;
}

static void lua_reply(const char *msg, size_t size, void *user_data)
{
	// For memory debugging.
	//heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);

	httpd_req_t *req = (httpd_req_t *)user_data;
	httpd_ws_frame_t ws_pkt;
	ws_pkt.type = HTTPD_WS_TYPE_TEXT;
	ws_pkt.fragmented = false;
	ws_pkt.payload = (uint8_t *)msg;
	ws_pkt.len = size;
	esp_err_t ret = httpd_ws_send_frame(req, &ws_pkt);
	//ESP_LOGI(WEB_TAG, "the frame contains: %s", ws_pkt.payload);
	if (ret != ESP_OK) {
		ESP_LOGE(WEB_TAG, "httpd_ws_send_frame failed with %d", ret);
	}
}

static esp_err_t start_server(const char *base_path)
{
	REST_CHECK(base_path, "wrong base path", err);
	rest_server_context_t *rest_context
		= calloc(1, sizeof(rest_server_context_t));
	REST_CHECK(rest_context, "No memory for rest context", err);
	strlcpy(rest_context->base_path, base_path,
		sizeof(rest_context->base_path));

	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI(WEB_TAG, "Starting HTTP Server");
	REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed",
		err_start);

	/* URI handlers for web API */
	httpd_uri_t api_file_list_uri = {
		.uri = "/api/file-list",
		.method = HTTP_GET,
		.handler = api_file_list_handler,
		.user_ctx = rest_context
	};
	httpd_register_uri_handler(server, &api_file_list_uri);

	httpd_uri_t api_delete_uri = {
		.uri = "/api/delete",
		.method = HTTP_GET,
		.handler = api_delete_handler,
		.user_ctx = rest_context
	};
	httpd_register_uri_handler(server, &api_delete_uri);

	httpd_uri_t api_send_file_uri = {
		.uri = "/api/send-file",
		.method = HTTP_POST,
		.handler = api_send_file_handler,
		.user_ctx = rest_context
	};
	httpd_register_uri_handler(server, &api_send_file_uri);

	httpd_uri_t ws = {
			.uri = "/api/ws",
			.method = HTTP_GET,
			.handler = wss_message_handler,
			.user_ctx = NULL,
			.is_websocket = true
	};
	httpd_register_uri_handler(server, &ws);

	/* URI handler for getting web server files */
	httpd_uri_t common_get_uri = {
		.uri = "/*",
		.method = HTTP_GET,
		.handler = rest_common_get_handler,
		.user_ctx = rest_context
	};
	httpd_register_uri_handler(server, &common_get_uri);

	ESP_LOGI(WEB_TAG, "Registerd URL's to handler");

	return ESP_OK;
err_start:
	free(rest_context);
err:
	return ESP_FAIL;
}

void web_main(void)
{
	lua_context = create_lua_task();
	// Use a separate task to monitor incoming events.
	xTaskCreate((TaskFunction_t)&wss_monitor_task, "wss-monitor",
		4096, lua_context, 0, NULL);
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_LOGI(WEB_TAG, "NVS_Flash initialized");
	ESP_ERROR_CHECK(esp_netif_init());
	// ESP_ERROR_CHECK(esp_event_loop_create_default());
	initialise_mdns();
	ESP_LOGI(WEB_TAG, "MDNS initialized");
	netbiosns_init();
	ESP_LOGI(WEB_TAG, "netbiosns initialized");
	netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);
	ESP_LOGI(WEB_TAG, "netbiosns configured");
	ESP_ERROR_CHECK(start_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
	ESP_LOGI(WEB_TAG, "rest server started");
}

static void wss_monitor_task(ScriptTask *self)
{
	while (true) {
		Event event;
		if (!event_wait(-1, &event, self->queue)) {
			// Should not happen.
			ESP_LOGI(WEB_TAG, _("websocket event timed out?!\n"));
			continue;
		}
		char *prt = print_event(&event);
		event_free(&event);
		ESP_LOGI(WEB_TAG, "Event received for websocket: %s\n", prt);
		httpd_ws_frame_t ws_pkt;
		ws_pkt.type = HTTPD_WS_TYPE_TEXT;
		ws_pkt.fragmented = false;
		ws_pkt.payload = (uint8_t *)prt;
		ws_pkt.len = strlen(prt);
		httpd_ws_send_frame_async(websocket_hd, websocket_fd, &ws_pkt);
		free(prt);
	}
}
