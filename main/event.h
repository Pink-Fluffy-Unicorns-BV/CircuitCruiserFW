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
#ifndef EVENT_H
#define EVENT_H

 // Allow strings to be marked for translation,
 // even though translating is not currently supported.
#define _(msg) msg

#include <freertos/FreeRTOS.h>
#include <lua.h>

// Defined in main.c (which doesn't have a header file).
void print_system_state(void);

// Size of all FreeRTOS Tasks (except main).
#define TASK_STACK_SIZE 8192

// Maximum number of tasks that can run simultaneously.
#define MAX_TASKS 10

// Maximum number of event types.
#define MAX_EVENTS 100

// Maximum total size of single reply.
#define REPLY_BUFFER_SIZE 500

typedef struct ScriptTask {
	QueueHandle_t queue;
	lua_State *thread;
	int ref;	// Ref in the registry for this thread object.
	char *lua_file;	// Only used during startup.
	bool active;
} ScriptTask;

typedef struct Event {
	int eventcode;
	int i[6];
	float f[3];
	const char *s[3];
} Event;

// Mostly for internal use, but also used by interrupt handlers.
typedef struct EventType {
	const char *name;
	int num_float, num_int, num_str;
	const char *i[6];
	const char *f[3];
	const char *s[3];
	QueueHandle_t queue;
	bool raw;
} EventType;

extern EventType event_defs[MAX_EVENTS];

typedef void (*ReplyCb)(const char *msg, size_t size, void *user_data);

/// @brief Initialize the event system.
/// @return False in case of error.
bool event_init();

/// @brief Start the event system.
/// This must be called after everything is initialized.
/// @return False in case of error.
bool event_start();

/// @brief Claim an event for the given queue.
/// @param eventcode The event to claim.
/// @param raw Whether the parameters are parsed. Only used by Lua tasks.
/// @param queue The queue to send it to.
/// @return False in case of error.
bool event_claim(int eventcode, bool raw, QueueHandle_t queue);

/// @brief Release an event, to be claimed by another queue.
/// @param eventcode The event to release.
/// @return False in case of error.
bool event_release(int eventcode);

/// @brief Look up event code.
/// @param name The event name to look up.
/// @return The event code, or 0 if it was not found.
int event_find(const char *name);

/// @brief Look up event name.
/// @param eventcode The evetn to look up.
/// @return The event name, or NULL if it is not defined.
const char *event_get_name(int eventcode);

/// @brief Define a new event. It is not claimed.
/// @param name The name of the new event. This must not be defined yet.
/// @param f The 3 names of float parameters, or NULL if they are not used.
/// @param i The 3 names of int parameters, or NULL if they are not used.
/// @param s The 3 names of string parameters, or NULL if they are not used.
/// @return The new event code, or 0 in case of error.
int event_new(const char *name, const char *i[6], const char *f[3],
	const char *s[3]);

/// @brief Send an event.
/// @param event The event to send.
/// @return False if the event did not exist, was not claimed, or another error
/// was encountered.
bool event_send(const Event *event);

/// @brief Wait for an event.
/// @param timeout The timeout in milliseconds.
/// @param event The event that is received.
/// @param queue The queue to listen on.
/// @return False if the timeout expired. In that case *event is not changed.
bool event_wait(int timeout, Event *event, QueueHandle_t queue);

/// @brief Free allocated members (strings).
/// @param event The event to free.
/// @return False in case of error.
bool event_free(Event *event);

/// @brief Create a new Lua coroutine.
/// This is used by launch_lua_task and to create other Lua contexts,
/// for example in the Cli.
ScriptTask *create_lua_task();

/// @brief Launch a new FreeRTOS task that runs a Lua coroutine.
/// @param lua_file The Lua code to run in the coroutine.
/// @return False in case of error.
ScriptTask *launch_lua_task(const char *lua_file);

/// @brief Set enum constants in lua; for use by device drivers.
/// @param tablename The name of the new global variable in Lua.
/// @param names NULL-terminated array of enum names. Values enumerate from 0.
/// @return False in case of error.
bool set_lua_constants(const char *tablename, const char **names);

/// @brief For internal use only: set up wifi credentials in Lua environment.
/// @param ssid The ssid of the hosted access point.
/// @param password The password to connect to the hosted access point.
void set_wifi_info(const char *ssid, const char *password);

/// @brief Run a Lua command. The command cannot yield.
/// @param self The context in which to run.
/// @param command The command to run.
/// @param reply_cb The callback where the reply is sent.
/// @param user_data Opaque value passed to the callback.
bool run_lua_command(ScriptTask *self, const char *command, ReplyCb reply_cb,
	void *user_data);

/// @brief Convert event to human-readable string.
/// @param event The event to print.
/// @return The human-readable string.
/// 	NOTE: the returned value must be freed by the caller.
char *print_event(Event *event);

#endif
