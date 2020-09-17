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
        this.typesPromises = {};
        this.types = [];
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

    addType(type, model) {
        this.typesPromises[type].resolver(model);
    }

    getLoadingPromise(type) {
        if (!this.typesPromises.hasOwnProperty(type)) {
            var script = document.createElement("script");
            script.type = "text/javascript";
            script.src = `/${type}-model.js`;
            console.log(script.src);
            document.body.appendChild(script);

            let resolver;
            let promise = new Promise((res) => resolver = res);
            this.typesPromises[type] = {resolver, promise};
            return promise;
        }
        return this.typesPromises[type].promise;
    }

    spawnCallback = (data) => {
        const netId = this.objects.length;
        this.objects.push(null);
        this.getLoadingPromise(data.type).then(({spawn, update}) => {
            const object = spawn(data);
            object.netId = netId;
            object.name = data.name;
            object.argosUpdate = update;

            this.objects[netId] = object;
            scene.add(object);
        });
    }

    updateCallback = (networkId, bin) => {
        const object = this.objects[networkId];
        // object is null if the corresponding script <type>-model.js is not loaded
        if (object != null) object.argosUpdate(bin, object);
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
        spawnCallback: OBJECTS.spawnCallback,
        updateCallback: OBJECTS.updateCallback,
        luaCallback: editor.setScripts
    });
}

document.getElementById("connect").addEventListener("click", connect);
