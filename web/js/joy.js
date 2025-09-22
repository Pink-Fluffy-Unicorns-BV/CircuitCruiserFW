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
 * # NAME       = Joystick widget		                	                 #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser                         #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */

/*
 * Name          : joy.js
 * @author       : Roberto D'Amico (Bobboteck)
 * Last modified : 09.06.2020
 * Revision      : 1.1.6
 *
 * Modification History:
 * Date         Version     Modified By     Description
 * 2025-09-22   Unreleased  Bas Wijnen      Support multiple joysticks with separate touch events and allow single-axis joysticks.
 * 2021-12-21   2.0.0       Roberto D'Amico New version of the project that integrates the callback functions, while 
 *                                          maintaining compatibility with previous versions. Fixed Issue #27 too, 
 *                                          thanks to @artisticfox8 for the suggestion.
 * 2020-06-09   1.1.6       Roberto D'Amico Fixed Issue #10 and #11
 * 2020-04-20   1.1.5       Roberto D'Amico Correct: Two sticks in a row, thanks to @liamw9534 for the suggestion
 * 2020-04-03               Roberto D'Amico Correct: InternalRadius when change the size of canvas, thanks to 
 *                                          @vanslipon for the suggestion
 * 2020-01-07   1.1.4       Roberto D'Amico Close #6 by implementing a new parameter to set the functionality of 
 *                                          auto-return to 0 position
 * 2019-11-18   1.1.3       Roberto D'Amico Close #5 correct indication of East direction
 * 2019-11-12   1.1.2       Roberto D'Amico Removed Fix #4 incorrectly introduced and restored operation with touch 
 *                                          devices
 * 2019-11-12   1.1.1       Roberto D'Amico Fixed Issue #4 - Now JoyStick work in any position in the page, not only 
 *                                          at 0,0
 * 
 * The MIT License (MIT)
 *
 *  This file is part of the JoyStick Project (https://github.com/bobboteck/JoyStick).
 *	Copyright (c) 2015 Roberto D'Amico (Bobboteck).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @desc Principal object that draw a joystick, you only need to initialize the object and suggest the HTML container
 * @costructor
 * @param container {String} - HTML object that contains the Joystick
 * @param parameters (optional) - object with following keys:
 *  title {String} (optional) - The ID of canvas (Default value is 'joystick')
 *  width {Int} (optional) - The width of canvas, if not specified is setted at width of container object (Default value is the width of container object)
 *  height {Int} (optional) - The height of canvas, if not specified is setted at height of container object (Default value is the height of container object)
 *  internalFillColor {String} (optional) - Internal color of Stick (Default value is '#00AA00')
 *  internalLineWidth {Int} (optional) - Border width of Stick (Default value is 2)
 *  internalStrokeColor {String}(optional) - Border color of Stick (Default value is '#003300')
 *  externalLineWidth {Int} (optional) - External reference circonference width (Default value is 2)
 *  externalStrokeColor {String} (optional) - External reference circonference color (Default value is '#008000')
 *  autoReturnToCenter {Bool} (optional) - Sets the behavior of the stick, whether or not, it should return to zero position when released (Default value is True and return to zero)
 * @param callback {StickStatus} - 
 */
var Joystick = (function(container, parameters, callback)
{
    parameters = parameters || {};
    var title = (parameters.title === undefined ? "joystick" : parameters.title),
        width = (parameters.width === undefined ? 0 : parameters.width),
        height = (parameters.height === undefined ? 0 : parameters.height),
        internalFillColor = (parameters.internalFillColor === undefined ? "#00AA00" : parameters.internalFillColor),
        internalLineWidth = (parameters.internalLineWidth === undefined ? 2 : parameters.internalLineWidth),
        internalStrokeColor = (parameters.internalStrokeColor === undefined ? "#003300" : parameters.internalStrokeColor),
        externalLineWidth = (parameters.externalLineWidth === undefined ? 2 : parameters.externalLineWidth),
        externalStrokeColor = (parameters.externalStrokeColor ===  undefined ? "#008000" : parameters.externalStrokeColor),
        autoReturnToCenter = (parameters.autoReturnToCenter === undefined ? [true, true] : parameters.autoReturnToCenter);

    callback = callback || function(StickStatus) {};

    // Create Canvas element and add it in the Container object
    var objContainer = document.getElementById(container);
    
    // Fixing Unable to preventDefault inside passive event listener due to target being treated as passive in Chrome [Thanks to https://github.com/artisticfox8 for this suggestion]
    objContainer.style.touchAction = "none";

    var canvas = document.createElement("canvas");
    canvas.id = title;
    if(width === 0) { width = objContainer.clientWidth; }
    if(height === 0) { height = objContainer.clientHeight; }
    canvas.width = width;
    canvas.height = height;
    objContainer.appendChild(canvas);
    var context=canvas.getContext("2d");

    var pressed = {}; // keys are touch ids or -button for mouse.
    var numPressed = 0; // Number of active pressing touches (or mouse).
    var circumference = 2 * Math.PI;
    var internalRadius = (canvas.width-((canvas.width/2)+10))/2;
    var maxMoveStick = internalRadius + 5;
    var externalRadius = internalRadius + 30;
    var centerX = canvas.width / 2;
    var centerY = canvas.height / 2;
    // Used to save current position of stick
    var movedX = centerX;
    var movedY = centerY;

    // Check if the device support the touch or not
    if("ontouchstart" in document.documentElement)
    {
        canvas.addEventListener("touchstart", onTouchStart, false);
        document.addEventListener("touchmove", onTouchMove, false);
        document.addEventListener("touchend", onTouchEnd, false);
        document.addEventListener("touchcancel", onTouchEnd, false);
    }
    else
    {
        canvas.addEventListener("mousedown", onMouseDown, false);
        document.addEventListener("mousemove", onMouseMove, false);
        document.addEventListener("mouseup", onMouseUp, false);
    }
    // Draw the object
    draw();

    /******************************************************
     * Private methods
     *****************************************************/

    /**
     * @desc Draw the joystick on the canvas.
     */
    function draw()
    {
        // Clear canvas
        context.clearRect(0, 0, canvas.width, canvas.height);

        // External circle.
        context.beginPath();
        const rx = (autoReturnToCenter[0] === null ? .5 : 1) * externalRadius;
        const ry = (autoReturnToCenter[1] === null ? .5 : 1) * externalRadius;
        context.ellipse(centerX, centerY, rx, ry, 0, 0, circumference);
        context.lineWidth = externalLineWidth;
        context.strokeStyle = externalStrokeColor;
        context.stroke();

        // Internal parts.
        context.beginPath();
        if(movedX<internalRadius) { movedX=maxMoveStick; }
        if((movedX+internalRadius) > canvas.width) { movedX = canvas.width-(maxMoveStick); }
        if(movedY<internalRadius) { movedY=maxMoveStick; }
        if((movedY+internalRadius) > canvas.height) { movedY = canvas.height-(maxMoveStick); }
        context.arc(movedX, movedY, internalRadius, 0, circumference, false);
        // create radial gradient
        var grd = context.createRadialGradient(centerX, centerY, 5, centerX, centerY, 200);
        // Light color
        grd.addColorStop(0, internalFillColor);
        // Dark color
        grd.addColorStop(1, internalStrokeColor);
        context.fillStyle = grd;
        context.fill();
        context.lineWidth = internalLineWidth;
        context.strokeStyle = internalStrokeColor;
        context.stroke();
    }

    function finish()
    {
        // Redraw object
        draw();

        // Set attribute of callback
        callback({
            xPosition: movedX,
            yPosition: movedY,
            x: (100*((movedX - centerX)/maxMoveStick)).toFixed(),
            y: ((100*((movedY - centerY)/maxMoveStick))*-1).toFixed()
        });
    }

    function startMove(touchId)
    {
        if (pressed[touchId])
            return;
        pressed[touchId] = true;
        numPressed += 1;
    }

    function move(touchId, x, y)
    {
        if(!pressed[touchId])
            return;

        // Manage offset
        let element = canvas;
        while (element && element !== document.body) {
            x -= element.offsetLeft - element.scrollLeft + element.clientLeft;
            y -= element.offsetTop - element.scrollTop + element.clientTop;
            element = element.offsetParent;
        }

        //console.info(touchId);
        if (autoReturnToCenter[0] !== null)
            movedX = x;
        if (autoReturnToCenter[1] !== null)
            movedY = y;
        finish();
    }

    function release(touchId)
    {
        //console.info(touchId);
        if (!pressed[touchId])
            return;
        pressed[touchId] = false;
        numPressed -= 1;
        // Only return if no other touches are still active.
        if (numPressed == 0) {
            // If required reset position store variable
            if (autoReturnToCenter[0])
                movedX = centerX;
            if (autoReturnToCenter[1])
                movedY = centerY;
        }
        finish();
    }

    /**
     * @desc Events for manage touch
     */
    function onTouchStart(event)
    {
        for (var t = 0; t < event.changedTouches.length; ++t)
            startMove(event.changedTouches[t].identifier);
    }

    function onTouchMove(event)
    {
        for (var t = 0; t < event.changedTouches.length; ++t) {
            var touch = event.changedTouches[t];
            move(touch.identifier, touch.pageX, touch.pageY);
        }
    }

    function onTouchEnd(event)
    {
        for (var t = 0; t < event.changedTouches.length; ++t)
            release(event.changedTouches[t].identifier);
    }

    /**
     * @desc Events for manage mouse
     */
    function onMouseDown(event)
    {
        startMove(-(event.button + 1));
    }

    function onMouseMove(event) 
    {
        move(-(event.button + 1), event.pageX, event.pageY);
    }

    function onMouseUp(event) 
    {
        release(-(event.button + 1));
    }

    /******************************************************
     * Public methods
     *****************************************************/

    /**
     * @desc The width of canvas
     * @return Number of pixel width 
     */
    this.GetWidth = function () 
    {
        return canvas.width;
    };

    /**
     * @desc The height of canvas
     * @return Number of pixel height
     */
    this.GetHeight = function () 
    {
        return canvas.height;
    };

    /**
     * @desc The X position of the cursor relative to the canvas that contains it and to its dimensions
     * @return Number that indicate relative position
     */
    this.GetPosX = function ()
    {
        return movedX;
    };

    /**
     * @desc The Y position of the cursor relative to the canvas that contains it and to its dimensions
     * @return Number that indicate relative position
     */
    this.GetPosY = function ()
    {
        return movedY;
    };

    /**
     * @desc Normalizzed value of X move of stick
     * @return Integer from -100 to +100
     */
    this.GetX = function ()
    {
        return (100*((movedX - centerX)/maxMoveStick)).toFixed();
    };

    /**
     * @desc Normalizzed value of Y move of stick
     * @return Integer from -100 to +100
     */
    this.GetY = function ()
    {
        return ((100*((movedY - centerY)/maxMoveStick))*-1).toFixed();
    };
});
