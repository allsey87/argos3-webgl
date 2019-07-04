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

const UPDATES = {
    0: basicUpdateCallback,
    1: basicUpdateCallback,
    3: prototypeUpdateCallback
};

const PROTOTYPE_GEOMETRIES = [
    THREE.BoxGeometry,
    THREE.CylinderGeometry,
    THREE.SphereGeometry
];

const SPAWNS = {
    0: /* CUBE */ (bin) => {
        let reals = [0, 0, 0].map(() => bin.extractReal());
        let geometry = new THREE.BoxGeometry(...reals);
        let material = new THREE.MeshStandardMaterial({ color: 0xff00ff });
        TYPES.push(0);
        return new THREE.Mesh(geometry, material);
    },
    1 : /* CYLINDER */ (bin) => {
        let reals = [0, 0].map(() => bin.extractReal());
        let geometry = new THREE.CylinderGeometry(reals[0], reals[0], reals[1]);
        let material = new THREE.MeshStandardMaterial({ color: 0xff0000 });
        TYPES.push(1);
        return new THREE.Mesh(geometry, material);
    },
    3: /* PROTOTYPE */ (bin) => {
        let parent = new THREE.Group();

        while (!bin.isConsumed()) {
            let type = bin.extractUint8();
            let extent = [0, 0, 0].map(() => bin.extractReal());
            let geometry = new PROTOTYPE_GEOMETRIES[type];
            let material = new THREE.MeshStandardMaterial({ color: 0x1af055 });
            let mesh = new THREE.Mesh(geometry, material);
            mesh.scale.set(...extent);
            parent.add(mesh);
        }
        TYPES.push(3);
        return parent;
    },
}

function spawn(typeId, bin) {
    let object = SPAWNS[typeId](bin);
    object.netId = OBJECTS.length;
    OBJECTS.push(object);
    scene.add(object);
}

function prototypeUpdateCallback(networkId, bin) {
    basicUpdateCallback(networkId, bin);
    let parent = OBJECTS[networkId];

    parent.children.forEach((mesh) => {
        let position = [0, 0, 0].map(() => bin.extractReal());
        let rotation = [0, 0, 0].map(() => bin.extractReal());
        mesh.position.set(...position);
        mesh.rotation.set(...rotation);
    });
}

function basicUpdateCallback(networkId, bin) {
    let position = [0, 0, 0].map(() => bin.extractReal());
    let rotation = [0, 0, 0].map(() => bin.extractReal());
    OBJECTS[networkId].position.set(...position);
    OBJECTS[networkId].rotation.set(...rotation);
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
