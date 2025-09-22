-- # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
-- #                                                                         #
-- #                              +@@+    .-++-------=*=.                    #
-- #                             +%%@+-------------------+                   #
-- #                            =@@+*=---------------------==                #
-- #                         .%+-=@%*+=----------------------=*.             #
-- #                        =%---=@=***-------------------------+.           #
-- #                       .%----=@%#=+%=------------------------*           #
-- #                        *+---=@**#%-#=----------+:*------------          #
-- #                        .#+--=@%%#++++=-------+: :*-----------*          #
-- #                           +##@%#%%%#=#=----*:..:*------------#.         #
-- #             **%@@@@@@@#+-.                    -=------------+           #
-- #       .*@%+.         .                       =------------==            #
-- #    =@=.....         =@+=%%   +@   --        :*-------------             #
-- #   @*........                 .#@@@-          +------------=:            #
-- #   @=.......                                  -+---------------++*.      #
-- #   *%-...-%@.                     ...         .=------------------:      #
-- #     -@@=...                                   .+----------------#       #
-- #        =#@%+-..                               ..+-------------=.        #
-- #                  .:-=*#%%%@@@:                ...=----------+:  +=+:    #
-- #                            +%                  ...+---------= .-----:   #
-- #                           :@.                   ...++--------------+    #
-- #                           %*                     ....+@%#-----%+        #
-- #                          :@.                      .....+@:              #
-- #                                                                         #
-- # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
-- # NAME       = Startup code.                                              #
-- # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
-- # DATE       = 11-12-2024                        	                     #
-- # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema                             #
-- # WEBSITE    = https://pinkfluffyunicorns.nl                              #
-- # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
-- # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

-- Configurable settings.
center = 1000
amplitude = 250
throttle = 0.6  -- 100% draws too much current.
reverse_factor = 0.9
whitebalance = {3700, 6175, 13836}  -- Measured color value for white.

disable_color_sensor = true

math = require "math"

-- Pin definitions.
PIN_BUTTON = 0
PIN_COLOR_SENSOR = 21
PIN_5V_GOOD = 39
PIN_5V = 40
PIN_SERVO = 41
PIN_COLOR_LIGHT = 47

-- List of browser connections for debugging.
debuggers = {}

function enable_dbg()
    local e = event.new("debug" .. tostring(#debuggers), {}, {}, {"str"})
    event.claim(e)
    table.insert(debuggers, e)
end

function dbg(msg)
    -- Send message to serial monitor.
    print(msg)

    -- Send message to all debuggers.
    for i, d in ipairs(debuggers) do
        event.send{d, msg}
    end
end

function trim(c, a)
    if c ~= nil then
        center = c
    end
    if s ~= nil then
        aplitude = a
    end
    js(0, 0)
end

function parse_color(evt)
    local e, ir, g, b, r = table.unpack(evt)
    r = r / whitebalance[1]
    g = g / whitebalance[2]
    b = b / whitebalance[3]
    return r, g, b
end

function event.wait(timeout)
    return coroutine.yield(timeout)
end

-- First event is fake; it means the system has started.
event.wait()

-- Define some constants for faster event handling.
PWM_EVENT = event.find('pwm')
GETPIN_EVENT = event.find('get_pin')
SETPIN_EVENT = event.find('set_pin')
SETLED_EVENT = event.find('set_LED')
MOTOR_EVENT = event.find('motor')

-- Reset to good state. This may not happen for all reset types.
event.send {SETPIN_EVENT, PIN_5V, gpio.LOW}
event.send {MOTOR_EVENT, 0, 100}
event.send {PWM_EVENT, 1, PIN_SERVO, center}

--print("Received events: pwm = " .. tostring(PWM_EVENT) .. ",
-- setpin = " .. tostring(SETPIN_EVENT) .. ", setled = " ..
-- tostring(SETLED_EVENT))

print("Wifi credentials:")
print("\tSSID: " .. wifi.ssid)
print("\tPassword: " .. wifi.password)
-- Convenience code for LED.
function set_LED(n, rgb)
    event.send {SETLED_EVENT, n, rgb[1], rgb[2], rgb[3]}
end
BLACK = {0, 0, 0}
BRIGHTRED = {255, 0, 0}
RED = {100, 0, 0}
BRIGHTWHITE = {255, 255, 255}
WHITE = {100, 100, 100}
GREEN = {0, 100, 0}
BLUE = {0, 0, 100}

function s(x)
    event.send {PWM_EVENT, 1, PIN_SERVO, x}
end

-- Handle joystick.
local dirlookup = {
    [false] = 1,
    [true] = -1
}
local old_motor = 0
local reverse = false
function js(x, y)
    local motor = 0
    if y <= -10 or y >= 10 then
        -- Range is -256 to 256.
        motor = math.floor(y * 2.56 * throttle + .5);
    end
    local time -- approximate time to accelerate.
    if math.abs(motor) > math.abs(old_motor) then
        time = 500
    else
        time = 100
    end
    old_motor = motor
    -- Use fast event format.
    -- PWM uses channel, pin, on-time as arguments.
    -- PWM channel 0 is in use by the motor driver.
    -- Minimum: 600; maximum: 1200. (Measured on device.)
    -- Center: (1200 + 600) / 2 = 900.
    -- 100 js steps is 900 - 600 (== 1200 - 900) = 300.

    event.send {PWM_EVENT, 1, PIN_SERVO,
        math.floor(center + amplitude * x / 100)}
    if motor < 0 then
        motor = math.floor(motor * reverse_factor) -- Slower in reverse.
    end
    event.send {MOTOR_EVENT, motor, time}
    local new_reverse = motor < 0
    if new_reverse ~= reverse then
        reverse = new_reverse
        if reverse then
            set_LED(8, BRIGHTWHITE)
            set_LED(9, BRIGHTWHITE)
        else
            set_LED(8, BLACK)
            set_LED(9, BLACK)
        end
    end
end

function flash(on, off, t)
    if on == nil then
        on = BRIGHTWHITE
    end
    if off == nil then
        off = BLACK
    end
    if t == 0 then
        t = 100
    end
    for i = 0, 5 do
        set_LED(i, on)
    end
    event.wait(t)
    for i = 0, 5 do
        set_LED(i, off)
    end
    set_color()
end

function set_color()
    -- Throttle is 0.4, 0.6 or 0.8.
    local c
    if throttle  < 0.5 then
        c = RED
    elseif throttle > 0.7 then
        c = GREEN
    else
        c = WHITE
    end
    set_LED(1, c)
    set_LED(4, c)
end

num_connected = 0
function wifi_connected()
    -- Enable 5V.
    event.send {SETPIN_EVENT, PIN_5V, gpio.HIGH}
    num_connected = num_connected + 1;
    print("connected; now: %d\n", num_connected);
end

function wifi_disconnected()
    num_connected = num_connected - 1;
    print("disconnected; now: %d\n", num_connected);
    -- In case of a desync, recover.
    if num_connected <= 0 then
        num_connected = 0
        -- Disable 5V.
        event.send {SETPIN_EVENT, PIN_5V, gpio.LOW}
    end
end

MY_EVENT = event.new("startup_reply", {"num"}, {}, {})
event.claim(MY_EVENT)

-- Handle event for website.
WEB_EVENT = nil
function register_event()
    if WEB_EVENT == nil then
        WEB_EVENT = event.new("header", {}, {}, {"msg"})
    else
        event.release(WEB_EVENT)
    end
    event.claim(WEB_EVENT);
end

if not disable_color_sensor then
    -- Pins:
    -- Leds for color sensor: IO47
    -- Color sensor interrupt: IO21
    -- Color sensor SCL: IO9
    -- Color sensor SDA: IO3
    -- Color sensor address: 1000111: 0x47
    event.send {
        event = 'i2c_new',
        addr = 0x52,
        speed = 100000,
        reply = MY_EVENT,
        timeout = -1
    }
    I2C_DEV = event.wait().num

    I2C_READ = event.find('i2c_read')
    I2C_WRITE = event.find('i2c_write')
    MY_I2C = event.new("startup_i2c_reply", {'ir', 'g', 'b', 'r'}, {}, {})
    event.claim(MY_I2C, true)

    -- Initialize some stuff.
    -- Enable color sensor LEDs.
    event.send {SETPIN_EVENT, PIN_COLOR_LIGHT, gpio.HIGH}

    event.send {I2C_WRITE, 0x00010100 | I2C_DEV, 0x06}  -- Enable RGB+IR sensor.
    event.send {I2C_WRITE, 0x04010100 | I2C_DEV, 0x40}  -- Measure every 25 ms.
    event.send {I2C_WRITE, 0x19010100 | I2C_DEV, 0x1c}  -- Enable interrupt.
    event.send {I2C_WRITE, 0x27010100 | I2C_DEV, 0x05}  -- Large changes.
end

-- Launch a separate task. This runs the file /lua/demo.lua .
event.launch('demo.lua')
-- The above is asynchronous, so we need to wait for it to complete.
-- The MY_EVENT variable is a global and can be used to send this task a
-- wake up signal. If demo.lua does not do that, the next line will time out.
event.wait(5000) -- 5 seconds is more than enough.

-- Set LEDs for driving.
function reset_LEDs()
    for i = 0, 5 do
        set_LED(i, WHITE)
    end
    for i, led in ipairs {6, 7, 10, 11} do
        set_LED(led, RED)
    end
end
reset_LEDs()

PIN_5V_CHANGE = event.new("pin_5v", {"pin"}, {}, {})
event.claim(PIN_5V_CHANGE, true)
event.send{SETPIN_EVENT, PIN_5V_GOOD, gpio.FALLING, PIN_5V_CHANGE}

PIN_SENSOR_INTERRUPT = event.new("color_interrupt", {"pin"}, {}, {})
event.claim(PIN_SENSOR_INTERRUPT, true)
event.send{SETPIN_EVENT, PIN_COLOR_SENSOR, gpio.FALLING, PIN_SENSOR_INTERRUPT}

-- Example for using the button.
BUTTON_CHANGE = event.new("button_change", {"pin"}, {}, {})
event.claim(BUTTON_CHANGE, true)
event.send{SETPIN_EVENT, PIN_BUTTON, gpio.CHANGE, BUTTON_CHANGE}

BUTTON_READ = event.new("button", {"value"}, {}, {})
event.claim(BUTTON_READ, true)

while true do
    local e = event.wait()
    if not disable_color_sensor and e[1] == MY_I2C then
        -- Sensor data received after interrupt or at request.
        local r, g, b = parse_color(e)
        if r < .4 and g < .4 and b < .4 then
            -- Black.
            dbg('Black')
            throttle = 0.5  -- Normal speed
        elseif r > b and r > g then
            -- Red tile.
            dbg('Red')
            throttle = 0.4  -- Slow
        elseif b > g then
            -- Blue tile.
            dbg('Blue')
            throttle = 0.5  -- Normal speed
        else
            -- Green tile.
            dbg('Green')
            throttle = 0.6  -- Fast
        end
        set_color()
        dbg('ir: ' .. tostring(ir))
        dbg('r: ' .. tostring(r))
        dbg('g: ' .. tostring(g))
        dbg('b: ' .. tostring(b))
        -- Read status register to reset interrupt.
        event.send {I2C_READ, 0x07010100 | I2C_DEV, -1}
    elseif e[1] == PIN_SENSOR_INTERRUPT then
        -- Request sensor data.
        event.send {I2C_READ, 0x0a040300 | I2C_DEV, MY_I2C}
    elseif e[1] == PIN_5V_CHANGE then
        event.send{SETLED_EVENT, -1, 0, 0, 0}
    elseif e[1] == BUTTON_CHANGE then
        event.send{GETPIN_EVENT, PIN_BUTTON, BUTTON_READ}
    elseif e[1] == BUTTON_READ then
        dbg('button state: ' .. tostring(e[2]))
    else
        dbg('event: ' .. tostring(event))
    end
end
