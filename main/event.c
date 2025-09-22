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
 * # NAME       = Tasks controller	                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <lualib.h>
#include <lauxlib.h>
#include <esp_rom_crc.h>
#include "event.h"
#include "cli.h"

#define QUEUE_LENGTH 10

static int run_lua(ScriptTask *self, int nargs, int *num_returns);
static void reply_lua_value(lua_State *L, int i);
static void print_lua_stack(lua_State *L);
static void script_task(void *arg);
static void handle_raw_event(ScriptTask *self);
static void handle_parsed_event(ScriptTask *self);
static int event_lua_claim(lua_State *L);
static int event_lua_release(lua_State *L);
static int event_lua_find(lua_State *L);
static int event_lua_get_name(lua_State *L);
static int event_lua_new(lua_State *L);
static int event_lua_send(lua_State *L);
static int event_lua_launch(lua_State *L);
static bool fill_names(lua_State *L, int idx, const char *names[], int size);
static int cleanup(lua_State *L, const char *i[6], const char *f[3],
	const char *s[3]);
static inline void dump_event(int eventcode);

static lua_State *main_lua_state;
static ScriptTask tasks[MAX_TASKS];
static ScriptTask *current_lua_thread;
static ScriptTask *startup_task;
static int max_event;	// Maximum event that has been defined, plus 1.

static char reply_buffer[REPLY_BUFFER_SIZE + 1];	// Add one for nul byte.
static size_t reply_size = 0;

EventType event_defs[MAX_EVENTS];

#if 0
// For debugging.
#include <sys/types.h>
#include <dirent.h>
static inline void print_dir(const char *path)
{
	DIR *dir = opendir("/www");
	struct dirent *f;
	while (true) {
		f = readdir(dir);
		if (!f)
			break;
		printf(_("file: %s\n"), f->d_name);
	}
}
#else
#define print_dir(path) do {} while(0)
#endif

static int compute_crc(lua_State *L)
{
	int n = lua_gettop(L);
	if (n < 1 || n > 2) {
		lua_settop(L, 0);
		lua_pushfstring(L, "Crc must have one or two arguments (got %d).", n);
		return 1;
	}
	if (!lua_isstring(L, 1)) {
		lua_settop(L, 0);
		lua_pushliteral(L, "First argument must be a string.");
		return 1;
	}
	uint32_t initial = 0;
	if (n == 2) {
		if (!lua_isinteger(L, 2)) {
			lua_settop(L, 0);
			lua_pushliteral(L,
				"Second argument must be an integer if present.");
			return 1;
		}
		initial = lua_tointeger(L, 2);
	}
	size_t length;
	const uint8_t *data = (const uint8_t *)lua_tolstring(L, 1, &length);

	uint32_t crc = esp_rom_crc32_le(initial, data, length);
	lua_settop(L, 0);
	lua_pushinteger(L, crc);
	return 1;
}

bool event_init()
{
	print_dir("/");
	max_event = 1;
	memset(event_defs, 0, sizeof(event_defs));
	main_lua_state = luaL_newstate();
	if (main_lua_state == NULL)
		return false;
	luaL_openlibs(main_lua_state);

	// Set up system globals in lua.
	lua_createtable(main_lua_state, 0, 7);
	lua_pushliteral(main_lua_state, "claim");
	lua_pushcfunction(main_lua_state, &event_lua_claim);
	lua_settable(main_lua_state, -3);
	lua_pushliteral(main_lua_state, "release");
	lua_pushcfunction(main_lua_state, &event_lua_release);
	lua_settable(main_lua_state, -3);
	lua_pushliteral(main_lua_state, "find");
	lua_pushcfunction(main_lua_state, &event_lua_find);
	lua_settable(main_lua_state, -3);
	lua_pushliteral(main_lua_state, "get_name");
	lua_pushcfunction(main_lua_state, &event_lua_get_name);
	lua_settable(main_lua_state, -3);
	lua_pushliteral(main_lua_state, "new");
	lua_pushcfunction(main_lua_state, &event_lua_new);
	lua_settable(main_lua_state, -3);
	lua_pushliteral(main_lua_state, "send");
	lua_pushcfunction(main_lua_state, &event_lua_send);
	lua_settable(main_lua_state, -3);
	lua_pushliteral(main_lua_state, "launch");
	lua_pushcfunction(main_lua_state, &event_lua_launch);
	lua_settable(main_lua_state, -3);

	lua_setglobal(main_lua_state, "event");

	// Add CRC function.
	lua_pushcfunction(main_lua_state, &compute_crc);
	lua_setglobal(main_lua_state, "crc");

	startup_task = launch_lua_task("startup.lua");

	return startup_task != NULL;
}

bool event_start()
{
	// Send empty event to startup task.
	// Note: The event type is never defined.

	if (startup_task == NULL || !startup_task->active)
		return false;

	Event event = { 0, };
	if (pdTRUE != xQueueSend(startup_task->queue, &event, 0))
		return false;

	return true;
}

bool event_claim(int eventcode, bool raw, QueueHandle_t queue)
{
	if (eventcode < 1 || eventcode >= max_event)
		return false;
	EventType *def = &event_defs[eventcode];
	if (def->name == NULL || def->queue != NULL)
		return false;
	def->queue = queue;
	def->raw = raw;
	return true;
}

bool event_release(int eventcode)
{
	if (eventcode < 1 || eventcode >= max_event)
		return false;
	EventType *def = &event_defs[eventcode];
	if (def->name == NULL || def->queue == NULL)
		return false;
	def->queue = NULL;
	return true;
}

int event_find(const char *name)
{
	int event;
	for (event = 1; event < max_event; ++event) {
		const char *event_name = event_defs[event].name;
		if (event_name == NULL)
			continue;
		if (strcmp(event_name, name) == 0)
			return event;
	}
	return 0;
}

const char *event_get_name(int eventcode)
{
	if (eventcode < 1 || eventcode >= max_event)
		return NULL;
	return event_defs[eventcode].name;
}

int event_new(const char *name, const char *i[6], const char *f[3],
	const char *s[3])
{
	if (max_event >= MAX_EVENTS)
		return 0;
	EventType *def = &event_defs[max_event];
	def->name = strdup(name);
	def->num_float = 0;
	def->num_int = 0;
	def->num_str = 0;
	def->queue = NULL;
	def->raw = true;
	int n;
	for (n = 0; n < 6; ++n) {
		def->i[n] = i[n];
		if (i[n] != NULL)
			def->num_int = n + 1;
	}
	for (n = 0; n < 3; ++n) {
		def->f[n] = f[n];
		if (f[n] != NULL)
			def->num_float = n + 1;
		def->s[n] = s[n];
		if (s[n] != NULL)
			def->num_str = n + 1;
	}
	//printf(_("created event\n"));
	//dump_event(max_event);
	return max_event++;
}

bool event_send(const Event *event)
{
	if (event->eventcode >= 1 && event->eventcode < max_event) {
		EventType *def = &event_defs[event->eventcode];
		//printf("Sending event %s, queue %p\n", def->name, def->queue);
		if (def->name != NULL && def->queue != NULL) {
			if (pdTRUE == xQueueSend(def->queue, event, 0))
				return true;
		}
	}
	event_free((Event *)event);
	return false;
}

bool event_wait(int timeout, Event *event, QueueHandle_t queue)
{
	TickType_t delay;
	if (timeout < 0)
		delay = portMAX_DELAY;
	else
		delay = timeout / portTICK_PERIOD_MS;
	return pdTRUE == xQueueReceive(queue, event, delay);
}

bool event_free(Event *event)
{
	int s;
	for (s = 0; s < 3; ++s) {
		if (event->s[s] != NULL)
			free((char *)event->s[s]);
	}
	return true;
}

ScriptTask *create_lua_task()
{
	int task;
	for (task = 0; task < MAX_TASKS; ++task) {
		if (!tasks[task].active)
			break;
	}
	if (task >= MAX_TASKS) {
		// Creation failed.
		printf(_("Unable to create Lua task\n"));
		return NULL;
	}
	ScriptTask *self = &tasks[task];
	self->active = true;
	self->queue = xQueueCreate(QUEUE_LENGTH, sizeof(Event));
	self->thread = lua_newthread(main_lua_state);
	self->ref = luaL_ref(main_lua_state, LUA_REGISTRYINDEX);
	return self;
}

char *destroy_lua_task(ScriptTask *self)
{
	if (self == NULL)
		return _("Attempt to destroy NULL script task.\n");
	if (self == startup_task)
		startup_task = NULL;
	self->active = false;
	luaL_unref(self->thread, LUA_REGISTRYINDEX, self->ref);
	lua_closethread(self->thread, NULL);
	size_t q;
	for (q = 0; q < max_event; ++q) {
		if (event_defs[q].queue == self->queue)
			event_defs[q].queue = NULL;
	}
	vQueueDelete(self->queue);
	return NULL;
}

ScriptTask *launch_lua_task(const char *lua_file)
{
	//printf(_("launching lua task %s\n"), lua_file);
	ScriptTask *self = create_lua_task();
	if (self == NULL) {
		printf(_("Failed to launch %s\n"), lua_file);
		return NULL;
	}
	if (asprintf(&self->lua_file, "/www/lua/%s", lua_file) < 0) {
		printf(_("Failed to allocate lua filename for launch\n"));
		destroy_lua_task(self);
		return NULL;
	}
	xTaskCreate(&script_task, lua_file, TASK_STACK_SIZE, self, 0, NULL);
	return self;
}

bool set_lua_constants(const char *tablename, const char **names)
{
	int num;
	const char **name;
	for (num = 0, name = names; *name != NULL; ++num, ++name) {}
	lua_createtable(main_lua_state, 0, num);
	int n;
	for (n = 0; n < num; ++n) {
		lua_pushstring(main_lua_state, names[n]);
		lua_pushinteger(main_lua_state, n);
		lua_rawset(main_lua_state, -3);
	}
	lua_setglobal(main_lua_state, tablename);
	return true;
}

void set_wifi_info(const char *ssid, const char *password)
{
	lua_createtable(main_lua_state, 0, 2);

	lua_pushstring(main_lua_state, "ssid");
	lua_pushstring(main_lua_state, ssid);
	lua_rawset(main_lua_state, -3);

	lua_pushstring(main_lua_state, "password");
	lua_pushstring(main_lua_state, password);
	lua_rawset(main_lua_state, -3);

	lua_setglobal(main_lua_state, "wifi");
}

void vararg_reply_queue(const char *fmt, va_list args)
{
	int len = vsnprintf(&reply_buffer[reply_size],
		REPLY_BUFFER_SIZE - reply_size, fmt, args);
	if (len < 0) {
		printf("unable to write to reply buffer.\n");
		return;
	}
	reply_size += len;
	if (reply_size > REPLY_BUFFER_SIZE) {
		printf("truncating reply\n");
		reply_size = REPLY_BUFFER_SIZE;
	}
}

void reply_queue(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vararg_reply_queue(fmt, args);
	va_end(args);
}

void reply_send(ReplyCb reply_cb, void *user_data, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vararg_reply_queue(fmt, args);
	va_end(args);

	// Send reply queue.
	reply_cb(reply_buffer, reply_size, user_data);
	reply_size = 0;
}

bool run_lua_command(ScriptTask *self, const char *command, ReplyCb reply_cb,
	void *user_data)
{
	current_lua_thread = self;
	if (LUA_OK == luaL_loadstring(self->thread, command)) {
		int n;
		int r = run_lua(self, 0, &n);
		if (reply_cb == 0) {
			// Discard pending reply.
			reply_size = 0;
			return true;
		}
		if (r == LUA_OK || r == LUA_YIELD) {
			if (n == 1) {
				reply_queue("R: ");
				reply_lua_value(self->thread, -1);
				reply_send(reply_cb, user_data, "\n");
				lua_pop(self->thread, 1);
			} else if (n != 0) {
				reply_queue(_("R: %d results:\n"), n);
				int v;
				for (v = 1; v <= n; ++v) {
					reply_queue(_("\t%d: "), v);
					reply_lua_value(self->thread, -n + v - 1);
					reply_queue("\n");
				}
				lua_pop(self->thread, n);
				reply_send(reply_cb, user_data, "");
			} else {
				reply_send(reply_cb, user_data,
					_("R"));
			}
			return true;
		}
	}
	reply_queue(_("R: Error in Lua command: "));
	reply_lua_value(self->thread, -1);
	reply_send(reply_cb, user_data, "\n");
	lua_pop(self->thread, 1);
	// If the function aborted, it cannot be resumed next time.
	// So discard the previous thread and create a new one.
	lua_closethread(self->thread, NULL);
	self->thread = lua_newthread(main_lua_state);
	lua_rawseti(main_lua_state, LUA_REGISTRYINDEX, self->ref);
	return false;
}

static int run_lua(ScriptTask *self, int nargs, int *num_returns)
{
	current_lua_thread = self;
	return lua_resume(self->thread, NULL, nargs, num_returns);
}

static void reply_lua_value(lua_State *L, int i)
{
	if (lua_isboolean(L, i))
		reply_queue("%s", lua_toboolean(L, i) ? "true" : "false");
	else if (lua_iscfunction(L, i))
		reply_queue(_("[cfunction: %p]"), lua_tocfunction(L, i));
	else if (lua_isfunction(L, i))
		reply_queue(_("[function]"));
	else if (lua_isinteger(L, i))
		reply_queue("%d", lua_tointeger(L, i));
	else if (lua_isnil(L, i))
		reply_queue("nil");
	else if (lua_isstring(L, i))
		reply_queue("'%s'", lua_tostring(L, i));
	else if (lua_isnumber(L, i))
		reply_queue("%f", lua_tonumber(L, i));
	else if (lua_istable(L, i)) {
		reply_queue("{");
		bool first = true;
		size_t s = lua_rawlen(L, i);
		size_t j;
		for (j = 1; j <= s; ++j) {
			if (!first)
				reply_queue(", ");
			first = false;
			lua_rawgeti(L, i, j);
			reply_lua_value(L, -1);
			lua_pop(L, 1);
		}
		lua_pushnil(L);
		while (lua_next(L, (i < 0 ? i - 1 : i)) != 0) {
			if (lua_isinteger(L, -2)) {
				int key = lua_tointeger(L, -2);
				if (key >= 1 && key <= s) {
					// This key has already been handled above.
					lua_pop(L, 1);
					continue;
				}
			}
			if (!first)
				reply_queue(_(", "));
			first = false;
			if (lua_isstring(L, -2)) {
				const char *key = lua_tostring(L, -2);
				reply_queue(_("%s = "), key);
			} else {
				reply_queue(_("["));
				reply_lua_value(L, -2);
				reply_queue(_("] = "));
			}
			reply_lua_value(L, -1);
			lua_pop(L, 1);
		}
		reply_queue(_("}"));
	} else if (lua_isthread(L, i))
		reply_queue(_("[thread]"));
	else
		reply_queue(_("[unknown]"));
}

static void print_lua_stack(lua_State *L)
{
	size_t size = lua_gettop(L);
	size_t i;
	for (i = 1; i <= size; ++i) {
		reply_queue(_("%d: "), i);
		reply_lua_value(L, i);
		reply_queue(_("\n"));
	}
}

static void script_task(void *arg)
{
	ScriptTask *self = arg;

	// Load target code.
	//printf(_("running lua file: %s: %p \n"), self->lua_file, self);
	if (LUA_OK != luaL_loadfile(self->thread, self->lua_file)) {
		// Error.
		const char *msg = lua_tostring(self->thread, -1);
		printf(_("Unable to load lua file %s: %s\n"), self->lua_file, msg);
		free(self->lua_file);
		self->lua_file = NULL;
		destroy_lua_task(self);
		vTaskDelete(NULL);
		return;
	}
	free(self->lua_file);
	self->lua_file = NULL;

	// Handle script commands.
	int n;
	int r = run_lua(self, 0, &n); // Start script.
	int delay = -1;	// Use endless delay by default.
	while (true) {
		if (r != LUA_YIELD) {
			// Thread returned or produced an error. Ignore return value.
			if (r != LUA_OK) {
				// Thread returned error.
				printf(_("Thread returned error: %s\n"),
					lua_tostring(self->thread, -1));
				print_lua_stack(self->thread);
				reply_send(cli_reply_cb, NULL, "");
			}
			lua_settop(self->thread, 0);
			destroy_lua_task(self);
			vTaskDelete(NULL);
			return;
		}
		if (n > 1 || (!lua_isinteger(self->thread, -1) &&
			!lua_isnil(self->thread, -1))) {
			if (n != 1) {
				printf(_("%d arguments yielded\n"), n);
				print_lua_stack(self->thread);
				reply_send(cli_reply_cb, NULL, "");
				while (true) {}
			}
			printf(_("invalid value yielded: must be one integer or nil.\n"));
			print_lua_stack(self->thread);
			reply_send(cli_reply_cb, NULL, "");
			lua_settop(self->thread, 0);
			r = run_lua(self, 0, &n);
			continue;
		}
		// Thread yielded or returned a value; respond to it.
		delay = (n < 1 || lua_isnil(self->thread, -1)) ? -1 :
			lua_tointeger(self->thread, -1);
		lua_settop(self->thread, 1);

		Event event;
		if (!event_wait(delay, &event, self->queue)) {
			r = run_lua(self, 0, &n);
			continue;
		}
		if (event.eventcode == 0) {
			// Special case for startup.lua resume after setup.
			r = run_lua(self, 0, &n);
			continue;
		}
		assert(event.eventcode >= 1 && event.eventcode < max_event);
		EventType *def = &event_defs[event.eventcode];
		if (def->raw) {
			// Handle event raw.
			lua_createtable(self->thread,
				1 + def->num_float + def->num_int + def->num_str, 0);
			lua_pushinteger(self->thread, event.eventcode);
			lua_rawseti(self->thread, -2, 1);
			int pos = 2;
			int num;
			for (num = 0; num < def->num_float; ++num) {
				lua_pushnumber(self->thread, event.f[num]);
				lua_rawseti(self->thread, -2, pos);
				++pos;
			}
			for (num = 0; num < def->num_int; ++num) {
				lua_pushinteger(self->thread, event.i[num]);
				lua_rawseti(self->thread, -2, pos);
				++pos;
			}
			for (num = 0; num < def->num_float; ++num) {
				lua_pushstring(self->thread, event.s[num]);
				lua_rawseti(self->thread, -2, pos);
				++pos;
			}
		} else {
			// Parse event.
			lua_createtable(self->thread,
				0, 1 + def->num_float + def->num_int + def->num_str);
			const char *event_name = event_get_name(event.eventcode);
			lua_pushliteral(self->thread, "event");
			lua_pushstring(self->thread, event_name);
			lua_rawset(self->thread, -3);
			int num;
			for (num = 0; num < def->num_float; ++num) {
				lua_pushstring(self->thread, def->f[num]);
				lua_pushnumber(self->thread, event.f[num]);
				lua_rawset(self->thread, -3);
			}
			for (num = 0; num < def->num_int; ++num) {
				lua_pushstring(self->thread, def->i[num]);
				lua_pushinteger(self->thread, event.i[num]);
				lua_rawset(self->thread, -3);
			}
			for (num = 0; num < def->num_float; ++num) {
				lua_pushstring(self->thread, def->s[num]);
				lua_pushstring(self->thread, event.s[num]);
				lua_rawset(self->thread, -3);
			}
		}
		event_free(&event);
		r = run_lua(self, 1, &n);
	}
}

static void handle_raw_event(ScriptTask *self)
{
	// This is a "raw" event. Don't attempt parameter lookup.
	// The first element is the event code.
	Event event;
	if (!lua_istable(self->thread, 1)) {
		printf("invalid event; argument is not a table\n");
		print_lua_stack(self->thread);
		return;
	}
	lua_pushinteger(self->thread, 1);
	lua_gettable(self->thread, 1);
	event.eventcode = lua_tointeger(self->thread, -1);
	lua_pop(self->thread, 1);
	if (event.eventcode < 1 || event.eventcode >= max_event ||
		event_defs[event.eventcode].name == NULL) {
		// Invalid event code; ignore.
		printf(_("invalid event code %d\n"), event.eventcode);
		return;
	}
	EventType *def = &event_defs[event.eventcode];
	if (def->name == NULL || def->queue == NULL) {
		// Invalid or unclaimed event; ignore.
		printf(_("invalid or unclaimed event code %d\n"), event.eventcode);
		return;
	}
	int pos = 2;
	int num;

	for (num = 0; num < def->num_int; ++num) {
		lua_geti(self->thread, 1, pos);
		event.i[num] = lua_tointeger(self->thread, -1);
		lua_pop(self->thread, 1);
		++pos;
	}
	for (; num < 6; ++num)
		event.i[num] = 0;

	for (num = 0; num < def->num_float; ++num) {
		lua_geti(self->thread, 1, pos);
		event.f[num] = lua_tonumber(self->thread, -1);
		lua_pop(self->thread, 1);
		++pos;
	}
	for (; num < 3; ++num)
		event.f[num] = NAN;

	for (num = 0; num < def->num_str; ++num) {
		lua_geti(self->thread, 1, pos);
		const char *str = lua_tostring(self->thread, -1);
		if (str == NULL) {
			// Invalid event. Send with empty string.
			printf(_("invalid string in event\n"));
			str = "";
		}
		event.s[num] = strdup(str);
		lua_pop(self->thread, 1);
		++pos;
	}
	for (; num < 3; ++num)
		event.s[num] = NULL;

	event_send(&event);
}

static void handle_parsed_event(ScriptTask *self)
{
	if (!lua_istable(self->thread, 1)) {
		printf("invalid event; argument is not a table\n");
		print_lua_stack(self->thread);
		return;
	}
	lua_pushliteral(self->thread, "event");
	lua_gettable(self->thread, 1);
	const char *event_name = lua_tostring(self->thread, -1);
	Event event;
	event.eventcode = event_find(event_name);
	if (event.eventcode == 0) {
		// Event name was not found; ignore.
		printf(_("unrecognized event name %s\n"), event_name);
		return;
	}
	lua_pop(self->thread, 1);
	EventType *def = &event_defs[event.eventcode];
	int num;

	for (num = 0; num < def->num_int; ++num) {
		lua_pushstring(self->thread, def->i[num]);
		lua_gettable(self->thread, 1);
		event.i[num] = lua_tointeger(self->thread, -1);
		lua_pop(self->thread, 1);
	}
	for (; num < 6; ++num)
		event.i[num] = 0;

	for (num = 0; num < def->num_float; ++num) {
		lua_pushstring(self->thread, def->f[num]);
		lua_gettable(self->thread, 1);
		event.f[num] = lua_tonumber(self->thread, -1);
		lua_pop(self->thread, 1);
	}
	for (; num < 3; ++num)
		event.f[num] = 0;

	for (num = 0; num < def->num_str; ++num) {
		lua_pushstring(self->thread, def->s[num]);
		lua_gettable(self->thread, 1);
		const char *str = lua_tostring(self->thread, -1);
		if (str == NULL) {
			// Invalid event. Send with empty string.
			printf(_("invalid named string in event\n"));
			str = "";
		}
		event.s[num] = strdup(str);
		lua_pop(self->thread, 1);
	}
	for (; num < 3; ++num)
		event.s[num] = NULL;

	event_send(&event);
}

static int event_lua_claim(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (nargs < 1) {
		printf(_("claim called without arguments\n"));
		return 0;
	}
	bool raw = false;
	if (nargs >= 2) {
		if (!lua_isboolean(L, 2)) {
			printf(_("raw argument to claim is not a boolean\n"));
			return 0;
		}
		raw = lua_toboolean(L, 2);
	}
	int eventcode;
	if (lua_isinteger(L, 1)) {
		eventcode = lua_tointeger(L, 1);
	} else {
		const char *name = lua_tostring(L, 1);
		eventcode = event_find(name);
	}
	lua_settop(L, 0);
	event_claim(eventcode, raw, current_lua_thread->queue);
	return 0;
}

static int event_lua_release(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (nargs < 1) {
		printf(_("release called without arguments\n"));
		return 0;
	}
	int eventcode;
	if (lua_isinteger(L, 1)) {
		eventcode = lua_tointeger(L, 1);
	} else {
		const char *name = lua_tostring(L, 1);
		eventcode = event_find(name);
	}
	lua_settop(L, 0);
	event_release(eventcode);
	return 0;
}

static int event_lua_find(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (nargs < 1) {
		printf(_("find called without arguments\n"));
		return 0;
	}
	const char *name = lua_tostring(L, 1);
	lua_settop(L, 0);
	int ret = event_find(name);
	lua_pushinteger(L, ret);
	return 1;
}

static int event_lua_get_name(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (nargs < 1) {
		printf(_("get_name called without arguments\n"));
		return 0;
	}
	int eventcode = lua_tointeger(L, 1);
	lua_settop(L, 0);
	const char *ret = event_get_name(eventcode);
	lua_pushstring(L, ret);
	return 1;
}

static bool fill_names(lua_State *L, int idx, const char *names[], int size)
{
	if (lua_isnil(L, idx))
		return true;
	if (!lua_istable(L, idx)) {
		printf(_("argument list for event.new is not a table\n"));
		return false;
	}
	int num = lua_rawlen(L, idx);
	if (num < 0)
		num = 0;
	if (num > size)
		num = size;
	int i;
	for (i = 0; i < num; ++i) {
		lua_geti(L, idx, i + 1);
		const char *name = lua_tostring(L, -1);
		names[i] = (const char *)strdup(name);
		lua_pop(L, 1);
	}
	return true;
}

static int cleanup(lua_State *L, const char *i[6], const char *f[3],
	const char *s[3])
{
	for (int n = 0; n < 6; ++n) {
		if (i[n] != NULL)
			free((char *)i[n]);
	}
	for (int n = 0; n < 3; ++n) {
		if (f[n] != NULL)
			free((char *)f[n]);
		if (s[n] != NULL)
			free((char *)s[n]);
	}
	lua_settop(L, 0);
	return 0;
}

static int event_lua_new(lua_State *L)
{
	int nargs = lua_gettop(L);
	if (nargs < 4) {
		printf(_("new called with fewer than 4 arguments\n"));
		return 0;
	}
	const char *name = lua_tostring(L, 1);
	const char *i[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
	const char *f[3] = { NULL, NULL, NULL };
	const char *s[3] = { NULL, NULL, NULL };
	if (!fill_names(L, 2, i, 6))
		return cleanup(L, i, f, s);
	if (!fill_names(L, 3, f, 3))
		return cleanup(L, i, f, s);
	if (!fill_names(L, 4, s, 3))
		return cleanup(L, i, f, s);
	int eventcode = event_new(name, i, f, s);
	lua_settop(L, 0);
	lua_pushinteger(L, eventcode);
	return 1;
}

static int event_lua_send(lua_State *L)
{
	if (current_lua_thread->thread != L) {
		printf("wrong thread?!\n");
		return 0;
	}
	//print_lua_stack(L);
	if (lua_istable(L, 1)) {
		// Value is a table; send the event.
		unsigned len = lua_rawlen(L, 1);
		if (len > 0)
			handle_raw_event(current_lua_thread);
		else
			handle_parsed_event(current_lua_thread);
	} else {
		// Invalid value; ignore.
		printf(_("Invalid argument for send\n"));
	}
	lua_settop(L, 0);
	return 0;
}

static int event_lua_launch(lua_State *L)
{
	const char *name = lua_tostring(L, 1);
	launch_lua_task(name);
	lua_settop(L, 0);
	return 0;
}

static inline void dump_event(int eventcode)
{
	EventType *def = &event_defs[eventcode];
	printf(_("Event: %s\nInts (%d):"), def->name, def->num_int);
	int t;
	for (t = 0; t < 6; ++t) {
		if (def->i[t] == NULL)
			printf(_(" -"));
		else
			printf(_(" %s"), def->i[t]);
	}
	printf(_("\nFloats (%d):"), def->num_float);
	for (t = 0; t < 3; ++t) {
		if (def->f[t] == NULL)
			printf(_(" -"));
		else
			printf(_(" %s"), def->f[t]);
	}
	printf(_("\nStrings (%d):"), def->num_str);
	for (t = 0; t < 3; ++t) {
		if (def->s[t] == NULL)
			printf(_(" -"));
		else
			printf(_(" %s"), def->s[t]);
	}
	printf(_("\n"));
	if (def->raw)
		printf(_("raw\n"));
}

char *print_event(Event *event)
{
	if (event == NULL)
		return strdup("Attempt to print NULL event!");
	if (event->eventcode < 0 || event->eventcode >= max_event ||
		event_defs[event->eventcode].name == NULL) {
		char *msg;
		asprintf(&msg, "Attempt to print invalid event %d!", event->eventcode);
		return msg;
	}
	EventType *def = &event_defs[event->eventcode];
	int i;
	// +3 should be enough; use a bit more to be sure.
	size_t nameslen = strlen(def->name) + 10;
	for (i = 0; i < def->num_int; ++i) {
		// An int is " = " plus optional '-' plus max 7 digits.
		nameslen += strlen(def->i[i]) + 12;
	}
	for (i = 0; i < def->num_float; ++i) {
		// Exact size of a float representation is not known.
		// 20 characters should hopefully be enough.
		nameslen += strlen(def->f[i]) + 20;
	}
	for (i = 0; i < def->num_str; ++i) {
		if (event->s[i] == NULL)
			return strdup("Event has NULL string parameter!");
		nameslen += strlen(def->s[i]) + 4 + strlen(event->s[i]);
	}
	// Output: EventName: IntName1 = ..., IntName2 = ..., ...
	char *buffer = malloc(nameslen + 3);
	if (buffer == NULL)
		return NULL;
	// Prefix for sending to browser.
	buffer[0] = 'E';
	buffer[1] = ':';
	buffer[2] = ' ';
	size_t p = 3;
	p += snprintf(&buffer[p], nameslen, "%s(", def->name);
	bool first = true;
	for (i = 0; i < def->num_int; ++i) {
		if (!first)
			p += snprintf(&buffer[p], nameslen - p, ", ");
		else
			first = false;
		p += snprintf(&buffer[p], nameslen - p,
			"%s=%d", def->i[i], event->i[i]);
	}
	for (i = 0; i < def->num_float; ++i) {
		if (!first)
			p += snprintf(&buffer[p], nameslen - p, ", ");
		else
			first = false;
		p += snprintf(&buffer[p], nameslen - p,
			"%s=%f", def->f[i], event->f[i]);
	}
	for (i = 0; i < def->num_str; ++i) {
		if (!first)
			p += snprintf(&buffer[p], nameslen - p, ", ");
		else
			first = false;
		p += snprintf(&buffer[p], nameslen - p,
			"%s=%s", def->s[i], event->s[i]);
	}
	p += snprintf(&buffer[p], nameslen - p, ")");
	return buffer;
}
