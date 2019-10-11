var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(
    75, window.innerWidth / window.innerHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);
renderer.domElement.oncontextmenu = (m) => false;

// for the chrome extension
// https://chrome.google.com/webstore/detail/threejs-inspector/dnhjfclbfhcbcdfpjaeacomhbdfjbebi
window.scene = scene;
window.THREE = THREE;

// LIGHT
scene.add(new THREE.AmbientLight(0xababab));
scene.add(new THREE.HemisphereLight(0xababab, 0x080820, 0.9));
scene.add(new THREE.DirectionalLight(0xffffff, 0.5));
scene.background = new THREE.Color(0x454545);


camera.position.z = 1;
camera.position.y = 0;
camera.position.set(0.24952068935534208, 0.11596185280276139, 0.16394337708001633) ; camera.rotation.set(-0.4302663515702581, 0.37962892503458523, 0.07864237505093132)
var uri = document.getElementById("uri");
var websocket;

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

const MAINLOOP = new MainLoop();
MAINLOOP.animate();


const LOADED_GLTF = {}
const OBJECTS = [];
const TYPES = [];
let obj = null;
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

const UPDATES = {
    0: basicUpdateCallback,
    1: basicUpdateCallback,
    3: prototypeUpdateCallback
};

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
            const {type, scale, position, rotation} = child;
            const geometry = new PROTOTYPE_GEOMETRIES[type](...scale);
            const material = new THREE.MeshStandardMaterial({ color: 0x1af055 });

            const mesh = new THREE.Mesh(geometry, material);
            mesh.matrix.makeRotationFromEuler(new THREE.Euler(...rotation));
            mesh.position.set(...position);
            parent.add(mesh);
        }

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

function spawn(data) {
    let object = SPAWNS[data.type](data);
    object.name = data.name;
    TYPES.push(TYPES_IDS[data.type]);
    object.netId = OBJECTS.length;
    OBJECTS.push(object);
    scene.add(object);
}

function prototypeUpdateCallback(networkId, bin) {
    if (!!bin.extractUint8()) {
        basicUpdateCallback(networkId, bin);
    }
    let parent = OBJECTS[networkId];
    while (!bin.isConsumed()) {
        let mesh = parent.children[bin.extractUint8()];
        setTransformFromBin(mesh, bin);
    }
}

function basicUpdateCallback(networkId, bin) {
    setTransformFromBin(OBJECTS[networkId], bin);
}

function updateCallback(networkId, bin) {
    let type = TYPES[networkId];
    UPDATES[type](networkId, bin);
}

function connect() {
    if (websocket) {
        websocket.websocket.close();
    }
    websocket = new Client(uri.value, load, spawn, updateCallback);
}

document.getElementById("connect").addEventListener("click", connect);
