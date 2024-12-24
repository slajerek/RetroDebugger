// Node.js WebSocket client to test the RetroDebugger server
const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:3563/stream');

ws.on('open', () => {
    console.log('Connected to WebSocket server');

	// note: token field is optional and always returned with response

	// // load
     jsonMessage = {
         "fn": "load",
         "params": {
             "path": "/Users/mars/develop/c64d/docs/tests/eob v1.00 20221121.crt"
         }
     };
     ws.send(JSON.stringify(jsonMessage));

	// hard reset
    // jsonMessage = {
    //     "fn": "c64/reset/hard",
	// 	"token": "blabla"
    // };
    // ws.send(JSON.stringify(jsonMessage));

	// pause/continue
	// jsonMessage = {
	// 	"fn": "c64/pause",
	// 	// "fn": "c64/continue",
	// };
	// ws.send(JSON.stringify(jsonMessage));

	// // cpu status
    // jsonMessage = {
    //     "fn": "c64/cpu/status",
    // };
    // ws.send(JSON.stringify(jsonMessage));

	// // writeBlock including I/O, how the CPU sees
    // jsonBinMessage = {
    //     "fn": "c64/cpu/memory/writeBlock",
    //     "params": {
    //         "address": 0xD82C,
    //     }
    // };
    // const binaryData = Buffer.from([0xDE, 0xAD, 0xBE, 0xEF]);
    // const messageWithBinary = Buffer.concat([Buffer.from(JSON.stringify(jsonBinMessage) + '\0'), binaryData]);
    // ws.send(messageWithBinary);

	// // readBlock read without I/O, directly from RAM
    // jsonMessage = {
    //     "fn": "c64/ram/readBlock",
    //     "params": {
    //         "address": 0xD800,
	// 		"size": 2024
	// 	}
    // };
    // ws.send(JSON.stringify(jsonMessage));

	// // VIC
	// jsonMessage = {
	// 	"fn": "c64/vic/write",
	// 	"params": {
	// 		"registers": {
	// 			"0xD020": 5,
	// 			"$21": 7,
	// 			"17" : "$3B"
	// 		}	
	// 	}
	// };
	// ws.send(JSON.stringify(jsonMessage));

	// // VIC
	// jsonMessage = {
	// 	"fn": "c64/vic/read",
	// 	"params": {
	// 		"registers": [
	// 			"0xD020", "$21", "17"
	// 		]
	// 	}
	// };
	// ws.send(JSON.stringify(jsonMessage));

	// // breakpoint PC
	// jsonMessage = {
	// 	"fn": "c64/cpu/breakpoint/add",
	// 	// "fn": "c64/cpu/breakpoint/remove",
	// 	"params": {
	// 		"addr" : 0xFEAF
	// 	}
	// };
	// ws.send(JSON.stringify(jsonMessage));
	
	// // breakpoint memory
	// jsonMessage = {
	// 	"fn": "c64/cpu/memory/breakpoint/add",
	// 	// "fn": "c64/cpu/memory/breakpoint/remove",
	// 	"params": {
	// 		"addr" : 0x1000,
	// 		"access" : "write",
	// 		"comparison" : "<",
	// 		"value" : 0x80
	// 	}
	// };
	// ws.send(JSON.stringify(jsonMessage));

	// breakpoint VIC raster line
	jsonMessage = {
		"fn": "c64/vic/breakpoint/add",
		// "fn": "c64/vic/breakpoint/remove",
		"params": {
			"rasterLine" : 0x32
		}
	};
	ws.send(JSON.stringify(jsonMessage));

	// // current segment
	 jsonMessage = {
	 	"fn": "c64/segment/read",
	 	//"fn": "c64/segment/write",
	 	"params" : {
	 		"segment" : "Code"
	 	}
	 };
	 ws.send(JSON.stringify(jsonMessage));


});

ws.on('message', (data) => {
    // Parse JSON response
    const nullPos = data.indexOf(0);
    let jsonResponse;
    let binaryData;

    if (nullPos !== -1) {
        // if there is a null byte, separate JSON and binary data
        jsonResponse = data.slice(0, nullPos).toString();
        binaryData = data.slice(nullPos + 1);
    } else {
        // if there is no null byte, the data is just JSON
        jsonResponse = data.toString();
    }

    try {
        const parsedResponse = JSON.parse(jsonResponse);
        console.log('Received:', parsedResponse);

        if (binaryData) {
            console.log('Received binary data:', binaryData);
        }

		// // on breakpoint - continue run
		// jsonMessage = {
		// 	"fn": "c64/continue",
		// };
		// ws.send(JSON.stringify(jsonMessage));
		
    } catch (e) {
        console.error('Error parsing JSON response:', e);
    }
});

ws.on('close', (code, reason) => {
    console.log(`WebSocket connection closed with code: ${code}, reason: ${reason}`);
});

ws.on('error', (error) => {
    console.error('WebSocket error:', error);
});
