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
 * # NAME       = Web page logic	                	                     #
 * # PROJECT    = Beursgadget 2025 - Circuit-Cruiser         	             #
 * # DATE       = 11-12-2024                        	                     #
 * # AUTHOR     = Bas Wijnen & Jan-Cees Tjepkema       	                     #
 * # WEBSITE    = https://pinkfluffyunicorns.nl                              #
 * # COPYRIGHT(C) Pink Fluffy Unicorns 2025                                  #
 * # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
 */
"use strict";
const joy_size = 250;
let receivedAck = 1;
let lastTime = 0;
let pendingData = [null, null];
let gateway = `ws://${window.location.hostname}/api/ws`;
let websocket;
let joyX = null;
let joyY = null;
window.addEventListener("load", initPage);

function initPage() {
    initWebSocket();
    initJoysticks();
    // Run a timer to force data every 50 ms.
    setInterval(systick, 50);

    send_request("api/file-list?folder=lua", fm_processFileList);
}

function attemptSend() {
    if (pendingData[0] == null && pendingData[1] == null)
        return;

    let now = Date.now();
    if (receivedAck == 1 || now - lastTime >= 100) {
        // Data should be sent.
        var x, y;
        if (pendingData[0] === null)
            x = joyX.GetX();
        else
            x = pendingData[0];
        if (pendingData[1] === null)
            y = joyY.GetY();
        else
            y = pendingData[1];
        pendingData = [null, null];

        receivedAck = 0;
        websocket.send("js(" + x + "," + y + ")");
        lastTime = now;
    }
}

function systick() {
    // This runs 20 times per second.
    attemptSend();
}

function initWebSocket() {
    //console.log("Trying to open a WebSocket connection...");
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}

function initJoysticks() {
    var joyXParam = {
        title: "JoystickX",
        width: joy_size,
        height: joy_size,
        autoReturnToCenter: [true, null],
        internalFillColor: "#d87dd3",
        internalStrokeColor: "#d87dd3",
        externalStrokeColor: "#d87dd3"
    };
    var joyYParam = {
        title: "JoystickY",
        width: joy_size,
        height: joy_size,
        autoReturnToCenter: [null, true],
        internalFillColor: "#d87dd3",
        internalStrokeColor: "#d87dd3",
        externalStrokeColor: "#d87dd3"
    };

    joyX = new Joystick("joyX", joyXParam, function (stickData) {
        pendingData[0] = stickData.x;
        attemptSend();
    });

    joyY = new Joystick("joyY", joyYParam, function (stickData) {
        pendingData[1] = stickData.y;
        attemptSend();
    });
}

function onOpen() {
    console.log("Connection opened");
    document.body.className = "connected";
    websocket.send("register_event()");
}
function onClose() {
    console.log("Connection closed");
    document.body.className = "disconnected";
    setTimeout(initWebSocket, 2000);
}

function debug_show_text(msg) {
    const box = document.getElementById("debugTextBox");
    box.value += "\r\n " + msg;
    box.scrollTop = box.scrollHeight;   // Scroll to bottom.
}

function onMessage(event) {
    let receivedMessage = event.data;
    //console.log(Date.now(), receivedMessage);
    if (receivedMessage.length > 0) {
        if (receivedMessage[0] == "R") {
            receivedAck = 1;
            if (receivedMessage.length > 1) {
                // A value was returned; show it to the user.
                debug_show_text(" Result: " + receivedMessage);
            }
        } else if (receivedMessage[0] == "E") {
            const m = /^E: header\(msg=(.*)\)$/.exec(receivedMessage);
            if (m !== null) {
                // Header event received; change text in header.
                const h = document.getElementById("header");
                while (h.childNodes.length > 0)
                    h.removeChild(h.firstChild);
                h.appendChild(document.createTextNode(m[1]));
            } else {
                // Other event received. Not handled; send to log.
                debug_show_text("Event: " + receivedMessage);
            }
        } else {
            // Invalid message received. Cannot handle; show in log.
            debug_show_text("Unrecognized message received: " + receivedMessage);
        }
    }
}

function ui_DebugEnterHandler(event) {
    if (event.key == "Enter") {
        ws_sendTextBoxCommand();
    }
}

function lua_clear_output() {
    document.getElementById("debugTextBox").value = "";
}

function ws_sendTextBoxCommand() {
    /* get text from textbox */
    var textBoxCommand = document.getElementById("command").value;

    if (textBoxCommand.length != 0) {
        /* empty textbox */
        document.getElementById("command").value = "";
        /* copy textbox line as echo in textarea (for the "terminal" feeling) */
        debug_show_text("Send:" + textBoxCommand);
        /* actually send the message over the websocket */
        websocket.send(textBoxCommand);
    } else {
        debug_show_text("Error: empty command.");
    }
}

function ws_sendCommand(commandString) {
    /* copy textbox line as echo in textarea (for the "terminal" feeling) */
    debug_show_text("Send:" + commandString);
    /* actually send the message over the websocket */
    websocket.send(commandString);
}

function fm_processFileList(fileList) {
    // First of all, clear current file list.
    const table = document.getElementById("fileTable");
    while (table.childElementCount > 0)
        table.removeChild(table.children[0]);

    const tr = document.createElement('tr');
    table.appendChild(tr);
    const headers = ["ID", "Name", "Size", "Options"];
    for (let i = 0; i < headers.length; ++i) {
        const td = document.createElement('td');
        tr.appendChild(td);
        td.appendChild(document.createTextNode(headers[i]));
    }

    const fileListJson = JSON.parse(fileList);
    //console.info(fileList, fileListJson);

    const amountFiles = fileListJson.files.length;
    //alert("json contains "+amountFiles+" files");
    for (let i = 0; i < amountFiles; i++) {
        var fileNameString = fileListJson.files[i].name;
        var fileSize = fileListJson.files[i].size;

        fm_addFile(fileNameString, fileSize);
    }
}

function fm_makeButton(parent, filename, cb, img) {
    const button = document.createElement("button");
    const image = document.createElement("img");
    parent.appendChild(button);
    button.class = "fm_buttonClass";
    image.src = "/img/" + img + "_logo.svg";
    button.appendChild(image);
    button.addEventListener("click", function () { cb(filename); });
}

function fm_addFile(fileNameString, fileSize) {
    const table = document.getElementById("fileTable");
    var rows = document.getElementById("fileTable").rows;
    if (fileNameString.length != 0) {
        var row = table.insertRow(rows.length);
        var cellId = row.insertCell(0);
        var cellName = row.insertCell(1);
        var cellSize = row.insertCell(2);
        var cellOptions = row.insertCell(3);
        cellId.appendChild(document.createTextNode(rows.length - 1));
        cellName.appendChild(document.createTextNode(fileNameString));
        cellSize.appendChild(document.createTextNode(fileSize + " bytes"));
        fm_makeButton(cellOptions, fileNameString, fm_editFile, "edit");
        fm_makeButton(cellOptions, fileNameString, fm_removeFile, "delete");
        fm_makeButton(cellOptions, fileNameString, fm_runFile, "run");
    } else {
        console.log("Error: FileName is empty");
    }
}

function editor_save() {
    const filename = document.getElementById("fileName").value;
    if (filename == "") {
        alert("You need to set a filename");
        return;
    }
    fm_putFile("lua/" + filename,
        document.getElementById('editFileTextBox').value);
}

function fm_putFile(fileName, contents) {
    let xhr = new XMLHttpRequest();
    xhr.addEventListener('load', function () {
        send_request("api/file-list?folder=lua", fm_processFileList);
    });
    xhr.addEventListener('error', function () {
        alert('Failed to upload file');
        send_request("api/file-list?folder=lua", fm_processFileList);
    });
    xhr.open('POST', '/api/send-file', true);
    const form_data = new FormData();
    const blob = new Blob([contents],
        { type: "application/json" });
    form_data.append("file", blob, fileName);
    xhr.addEventListener('readystatechange', function() {
        if (this.readyState != 4) {
            // Transfer is not complete yet.
            return;
        }
        if (this.status != 200)
            alert('Saving file FAILED!');
        else
            alert('File saved.');
    });
    xhr.send(form_data);
}

function send_request(url, cb) {
    const xhr = new XMLHttpRequest();
    xhr.addEventListener("load", function () { cb(xhr.responseText); });
    xhr.open('GET', url, true);
    xhr.send();
}

function fm_removeFile(fileName) {
    console.log("remove: ", fileName);
    if (!confirm("Are you sure you want to delete file: " + fileName + "?")) {
        // Do nothing.
        return;
    }
    send_request("api/delete?file=lua/" + fileName, function () {
        send_request("api/file-list?folder=lua", fm_processFileList);
    });
}

function fm_editFile(fileName) {
    console.log("edit file: ", fileName);
    send_request("lua/" + fileName, function (contents) {
        const box = document.getElementById("editFileTextBox");
        box.value = contents;
        document.getElementById("fileName").value = fileName;
    });
}

function fm_runFile(fileName) {
    console.log("run file: ", fileName);
    const safe_filename = fileName.replace(/\s|['"]/g, ' ');
    websocket.send("event.launch('" + safe_filename + "')");
}
