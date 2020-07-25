var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(
    75, window.innerWidth / window.innerHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer({ canvas: document.getElementById("canvas")});
renderer.setSize(window.innerWidth, window.innerHeight);

renderer.domElement.oncontextmenu = (m) => false;
renderer.domElement.setAttribute("tabindex", 1);

// for the chrome extension
// https://chrome.google.com/webstore/detail/threejs-inspector/dnhjfclbfhcbcdfpjaeacomhbdfjbebi
window.scene = scene;
window.THREE = THREE;

scene.background = new THREE.Color(0x454545);


camera.position.z = 1;
camera.position.y = 0;
camera.position.set(0.9933580715826136,0.4927882869044301, 2.209845773317536);
camera.rotation.setFromVector3(new THREE.Vector3(-0.170027967880398, 0.03996200716897759, 0.001199698853828924));
var uri = document.getElementById("uri");
var websocket;
var editor = new Editor(luaEditor, document.getElementById("editor-save"));

class MainLoop {
    constructor() {
        this.animate = this.animate.bind(this);
        this.clock = new THREE.Clock();
        this.callbacks = [];
    }

    animate() {
        requestAnimationFrame(this.animate);
        const deltaTime = this.clock.getDelta();
        this.callbacks.forEach(func => func(deltaTime));
        renderer.render(scene, camera);
    }

    addCallback(callback) {
        this.callbacks.push(callback);
    }
}

class ObjectsContainer {
    constructor() {
        this.objects = [];
        this.typeIds = [];
        this.populateScene();
    }

    populateScene() {
        scene.add(new THREE.AmbientLight(0xababab));
        scene.add(new THREE.HemisphereLight(0xababab, 0x080820, 0.9));
        scene.add(new THREE.DirectionalLight(0xffffff, 0.5));
    }

    reset() {
        let notLight = scene.children.filter((el) => !(el instanceof THREE.Light));
        scene.remove.apply(scene, notLight);
        this.objects = [];
        this.typeIds = [];
    }

    spawn = (data) => {
        const object = SPAWNS[data.type](data);
        object.name = data.name;
        this.typeIds.push(TYPES_IDS[data.type]);
        object.netId = this.objects.length;
        this.objects.push(object);
        scene.add(object);
    }

    updateCallback = (networkId, bin) => {
        const type = this.typeIds[networkId];
        let updateMethod;
        switch(type) {
            case 0: // BOX
            case 1: // CYLINDER
                updateMethod = this.basicUpdateCallback;
                break;
            case 3: // PROTOTYPE
                updateMethod = this.prototypeUpdateCallback;
                break;
            default:
                console.warn("Unknown type id:" + type);
                return;
        }
        updateMethod(networkId, bin);
    }

    updateTextCallback = (networkId, obj) => {
        const type = this.typeIds[networkId];
        let updateMethod;
        switch(type) {
            case 0:
            case 1:
                updateMethod = this.basicUpdateTextCallback;
                break;
            case 3:
                updateMethod = this.prototypeUpdateTextCallback;
                break;
            default:
                console.warn("Unknown type id:" + type);
                return;
        }
        updateMethod(networkId, obj);
    }

    prototypeUpdateCallback = (networkId, bin) => {
        if (!!bin.extractUint8()) {
            this.basicUpdateCallback(networkId, bin);
        }
        const parent = this.objects[networkId];
        while (!bin.isConsumed()) {
            const mesh = parent.children[bin.extractUint8()];
            setTransformFromBin(mesh, bin);
        }
    }

    prototypeUpdateTextCallback = (networkId, data) => {
        if (data.hasOwnProperty('position')) {
            this.basicUpdateTextCallback(networkId, data);
        }

        const parent = this.objects[networkId];
        for (let {id, ...coordinates} of data.children) {
            setTransform(parent.children[id], coordinates);
        }
    }

    basicUpdateCallback = (networkId, bin) => {
        setTransformFromBin(this.objects[networkId], bin);
    }

    basicUpdateTextCallback = (networkId, obj) => {
        setTransform(this.objects[networkId], obj);
    }
}

const MAINLOOP = new MainLoop();
MAINLOOP.animate();


const LOADED_GLTF = {}
const OBJECTS = new ObjectsContainer();


function load(e, name) {
    LOADED_GLTF[name] = e;
    e.scene.scale.set(0.1, 0.1, 0.1);
    e.scene.position.set(0.4, 0, 0);
    scene.add(e.scene);
    obj = e.scene;
}

const TYPES_IDS = {
    "box": 0,
    "cylinder": 1,
    "prototype": 3,
}

const PROTOTYPE_GEOMETRIES = {
    "box": THREE.BoxGeometry,
    "cylinder": THREE.CylinderGeometry,
    "sphere": THREE.SphereGeometry
};

const SPAWNS = {
    "box" : /* CUBE */ (data) => {
        const geometry = new THREE.BoxGeometry(...data.scale);
        const material = new THREE.MeshStandardMaterial({ color: 0xff00ff });

        const mesh = new THREE.Mesh(geometry, material);
        setTransform(mesh, data);
        return mesh;
    },
    "cylinder" : /* CYLINDER */ (data) => {
        const {scale} = data;
        const geometry = new THREE.CylinderGeometry(scale[0], ...scale);
        const material = new THREE.MeshStandardMaterial({ color: 0xff0000 });


        const mesh = new THREE.Mesh(geometry, material);
        setTransform(mesh, data);
        return mesh;
    },
    "prototype" : /* PROTOTYPE */ (data) => {

        let parent = new THREE.Group();
        setTransform(parent, data);

        for (let child of data.children) {
            const {type, scale, position, rotation, name} = child;
            const geometry = new PROTOTYPE_GEOMETRIES[type](...scale);
            const material = new THREE.MeshStandardMaterial({ color: 0x1af055 });

            const mesh = new THREE.Mesh(geometry, material);
            mesh.matrix.makeRotationFromEuler(new THREE.Euler(...rotation));
            mesh.position.set(...position);
            mesh.name = name;
            parent.add(mesh);
        }

        parent.luaScript = data.luaScript;
        return parent;
    },
}

function setTransformFromBin(mesh, bin) {
    let position = [0, 0, 0].map(() => bin.extractReal());
    let rotation = [0, 0, 0].map(() => bin.extractReal());
    setTransform(mesh, {position, rotation});
}

function setTransform(mesh, {position, rotation}) {
    mesh.position.set(...position);
    mesh.rotation.set(...rotation);
}

function connect() {
    if (websocket) {
        websocket.websocket.close();
    }
    OBJECTS.reset();
    websocket = new Client(uri.value, {
        loadCallback: load,
        spawnCallback: OBJECTS.spawn,
        updateCallback: OBJECTS.updateCallback,
        luaCallback: editor.setScripts,
        updateTextCallback: OBJECTS.updateTextCallback
    });
}

document.getElementById("connect").addEventListener("click", connect);
