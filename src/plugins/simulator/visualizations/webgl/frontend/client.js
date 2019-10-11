const MAX_MANTISSA = 9223372036854775806;

const PAUSE=0;
const AUTO=1;
const STEP=2;

class Client {
    constructor(uri, loadCallback, spawnCallback, updateCallback) {
        this.websocket = new WebSocket(uri, "spawn-objects" /* libwebsocket protocol */);
        this.websocket.onopen = this.onOpen.bind(this);
        this.websocket.onclose = this.onClose.bind(this);
        this.websocket.onmessage = this.onMessage.bind(this);
        this.websocket.binaryType = "arraybuffer";

        // Same uri as given but with http
        // wil be used to load models
        let url = new URL(uri);
        url.protocol = "http";
        this.url = url;
        this.loadCallback = loadCallback;
        this.spawnCallback = spawnCallback;
        this.updateCallback = updateCallback;
        this.loadPromise = null;
    }

    onOpen(evt) {
        console.log("open");
    }

    onClose(evt) {
        console.log("close");
    }

    spawnMessage(data) {
        this.spawnCallback(data);
    }

    updateMessage(bin) {
        const id = bin.extractUint32(0, false);
        this.updateCallback(id, bin);
    }

    onMessage(e) {
        if (typeof e.data === "string") {
            const data = JSON.parse(e.data);
            switch (data.messageType) {
                case "spawn":
                    this.spawnMessage(data);
                    break;
                default:
                    console.warn("Recieved a unkown text message type " + data.messageType);
            }
        } else {
            let bin = new BinaryExtractor(e.data);
            const messageType = bin.extractUint8();
            switch (messageType) {
                case 1:
                    this.updateMessage(bin);
                    break;
                default:
                    console.warn("Recieved a unkown binary message type " + messageType);
            }
        }
    }

    sendBuffer(buffer) {
        this.websocket.send(buffer);
    }

    sendStep() {
        isAuto = false;
        let data = new ArrayBuffer(1);
        (new Uint8Array(data))[0] = STEP;
        this.websocket.send(data);
    }
    
    sendAuto() {
        isAuto = true;
        updateButtonGray();
        let data = new ArrayBuffer(1);
        (new Uint8Array(data))[0] = AUTO;
        this.websocket.send(data);
    }
    
    sendPause() {
        isAuto = false;
        updateButtonGray();
        this.websocket.send(new ArrayBuffer(1));
    }
}

/**
 * Returns a new DataView of the same buffer but adding offset
 * @param {*} dv 
 * @param {*} offset 
 */
function decal(dv, offset) {
    return new DataView(dv.buffer, dv.byteOffset + offset);
}