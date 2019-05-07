var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(
    75, window.innerWidth / window.innerHeight, 0.1, 1000);
var renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
document.body.appendChild(renderer.domElement);

// for the chrome extension
// https://chrome.google.com/webstore/detail/threejs-inspector/dnhjfclbfhcbcdfpjaeacomhbdfjbebi
window.scene = scene;
window.THREE = THREE;

// LIGHT
scene.add(new THREE.AmbientLight(0x404040));
scene.add(new THREE.HemisphereLight(0xffffbb, 0x080820, 1));
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
let obj = null;
function load(e, name) {
    LOADED_GLTF[name] = e;
    e.scene.scale.set(0.1, 0.1, 0.1);
    e.scene.position.set(0.4, 0, 0);
    scene.add(e.scene);
    obj = e.scene;
}

function spawn(typeId, reals) {

    var geometry;
    var material
    switch (typeId) {
        case 0:
            geometry = new THREE.BoxGeometry(...reals);
            material = new THREE.MeshBasicMaterial({ color: 0xff00ff });
            break;
        case 1:
            geometry = new THREE.CylinderGeometry(reals[0], reals[0], reals[1]);
            material = new THREE.MeshBasicMaterial({ color: 0xff0000 });
            break;
    }
    var cube = new THREE.Mesh(geometry, material);
    OBJECTS.push(cube);
    scene.add(cube);
}

function updatePosition(networkId, pos) {
    OBJECTS[networkId].position.set(...pos);
}

function connect() {
    if (websocket) {
        websocket.websocket.close();
    }
    websocket = new Client(uri.value, load, spawn, updatePosition);
}

document.getElementById("connect").addEventListener("click", connect);
