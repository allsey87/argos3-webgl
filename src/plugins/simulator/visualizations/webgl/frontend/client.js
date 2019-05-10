const MAX_MANTISSA = 9223372036854775806;

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
        /*
         * Spawn the duck from gltf model
         */
        var loader = new THREE.GLTFLoader();
        const path = 'models/Duck.glb';
        this.url.pathname = path;

        loader.load(this.url.href, (e) => {
           this.loadCallback(e, name);
           console.log("Loaded");
        });
    }

    onClose(evt) {
        console.log("close");
    }

    spawnMessage(bin) {
        // cylinder and boxes are charterized by reals
        const typeId = bin.extractUint16();
        this.spawnCallback(typeId, bin);
    }

    updateMessage(bin) {
        const id = bin.extractUint32(0, false);
        this.updateCallback(id, bin);
    }

    onMessage(e) {
        let bin = new BinaryExtractor(e.data);
        const messageType = bin.extractUint8();

        switch (messageType) {
            case 0:
                this.spawnMessage(bin);
                break;
            case 1:
                this.updateMessage(bin);
                break;
        }
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