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

    firstMessage(evt) {
        console.log(`First message ${evt.data}`);
        let promises = [];
        evt.data.split('|').map((name) => ({path: `models/${name}.glb`, name}))
            .forEach(({path, name}) => {
            console.log("element is ");
            console.log(`Foreach Path=(${path}) Name=(${name})`);
            this.url.pathname = path;
            var loader = new THREE.GLTFLoader();
            promises.push(new Promise((resolve) => {
                loader.load(this.url.href, (e) => {
                    this.loadCallback(e, name);
                    resolve();
                });
            }));
        });
        this.loadPromise = Promise.all(promises);
        this.websocket.onmessage = this.secondMessage.bind(this);
    }

    secondMessage(evt) {
        this.loadPromise.then(() => {
            evt.data.split('|')
            .map((el, networkId) => {
                let [argosId, type] = el.split('=');
                this.spawnCallback(networkId, argosId, type);
            });
            this.websocket.onmessage = this.onMessage.bind(this);
        });
        this.websocket.onmessage = null;
    }

    getReal(dataView) {
        let nMantissa = dataView.getBigInt64(0, false);
        if (nMantissa.toString() == "0") {
            return 0;
        } else {
            let nExponent = dataView.getInt32(8, false);
            let significant = Number(lAbs(nMantissa) - BigInt(1)) / MAX_MANTISSA / 2 + 0.5;
            return significant * Math.pow(2, nExponent) * Math.sign(Number(nMantissa));
        }
    }

    spawnMessage(dataView) {
        // cylinder and boxes are charterized by reals
        const typeId = dataView.getUint16(0, false);
        console.log("Spawn id" + typeId);
        dataView = decal(dataView, 2);
        const reals = [];
        while (dataView.byteLength) {
            reals.push(this.getReal(dataView));
            dataView = decal(dataView, 12)
        }
        this.spawnCallback(typeId, reals);
    }

    updateMessage(dataView) {
        const id = dataView.getUint32(0, false);
        dataView = decal(dataView, 4);

        const reals = [];
        while (dataView.byteLength) {
            reals.push(this.getReal(dataView));
            dataView = decal(dataView, 12)
        }
        console.assert(reals.length == 7); // X Y Z and W X Y Z
        this.updateCallback(id, reals.splice(0, 3));
    }

    onMessage(e) {
        let dv = new DataView(e.data);
       // console.log(new Uint8Array(e.data));
       // console.log("Message size" + e.data.byteLength);
        const messageType = dv.getUint8(0);
       // console.log("Message type" + messageType);

        switch (messageType) {
            case 0:
                this.spawnMessage(decal(dv, 1));
                break;
            case 1:
                this.updateMessage(decal(dv, 1));
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