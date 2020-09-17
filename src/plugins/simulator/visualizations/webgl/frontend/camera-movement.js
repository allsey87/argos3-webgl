const KEYCODES = {
    RIGHT: 39,
    LEFT: 37,
    FORWARD: 38,
    BACK: 40,
    U: 85,
};

const right = () => new THREE.Vector3(1, 0, 0);
const ttop = () => new THREE.Vector3(0, 1, 0);
const forward = () => new THREE.Vector3(0, 0, 1);

const SPEED = 0.2;
let direction = null;
let isMousePressed = false;
let dist = 2;

var camHAxis;
var camVAxis;

function onMouseDown(event) {
    if (event.button == 2) {
        event.preventDefault();
        isMousePressed = true;
    }
}

function onMouseUp(event) {
    if (event.button == 2) {
        event.preventDefault();
        isMousePressed = false;
    }
}

function updateAxis() {
    camHAxis = ttop().applyQuaternion(camera.quaternion);
    camVAxis = right().applyQuaternion(camera.quaternion);
}

updateAxis();


function vertical(event) {
    let angleY = -0.01 * event.movementY;

    let q = new THREE.Quaternion();
    q.setFromAxisAngle(camVAxis, angleY);
    camera.quaternion.multiply(q);
}


function horizontal(event) {
    let angleX = -0.01 * event.movementX;

    let q = new THREE.Quaternion();
    q.setFromAxisAngle(camHAxis, angleX);
    camera.quaternion.multiply(q);
}

function onMouseMove(event) {
    event.preventDefault();
    let canvasBounds = renderer.context.canvas.getBoundingClientRect();
    mouseVector.x = ( ( event.clientX - canvasBounds.left ) / ( canvasBounds.right - canvasBounds.left ) ) * 2 - 1;
    mouseVector.y = - ( ( event.clientY - canvasBounds.top ) / ( canvasBounds.bottom - canvasBounds.top) ) * 2 + 1;
    if (isMousePressed) {
        vertical(event);
        horizontal(event);
    }
}

function onKeyDown(event) {
    direction = event.which;
    event.preventDefault();
}

function onKeyUp(event) {
    if (direction == event.which) {
        direction = null;
    }
}

function move(deltaTime) {
    let camDirection = new THREE.Vector3();
    camera.getWorldDirection(camDirection);

    let vect;
    switch (direction) {
        case KEYCODES.U:
            updateAxis();
            return;
        case KEYCODES.FORWARD:
            vect = forward().applyQuaternion(camera.quaternion).negate();
            break;
        case KEYCODES.BACK:
            vect = forward().applyQuaternion(camera.quaternion);
            break;
        case KEYCODES.RIGHT:
            vect = right().applyQuaternion(camera.quaternion);
            break;
        case KEYCODES.LEFT:
            vect = right().applyQuaternion(camera.quaternion).negate();
            break;
        default:
            return;
    }
    vect.multiplyScalar(SPEED * deltaTime);
    camera.position.add(vect);
}

function metalness(obj, val) {
    if (obj.material === undefined) {
        obj.children.forEach((e) => e.material.metalness = val);
    } else {
        obj.material.metalness = val;
    }
}

let raycaster = new THREE.Raycaster();
let mouseVector = new THREE.Vector2();
const XY_PLANE = new THREE.Plane(new THREE.Vector3(0, 0, 1), 0);
let sel;

class Selection {
    constructor() {
        this.selection = undefined;
        this.raycaster = new THREE.Raycaster();
    }

    unSelect() {
        if (this.selection) {
            metalness(this.selection, 0.5);
            this.selection = undefined;
        }
    }

    root(obj) {
        return (obj.netId === undefined) ? obj.parent: obj;
    }

    notifyEditor() {
        editor.selectionChanged(this.selection);
    }

    select(obj) {
        this.selection = obj;
        metalness(this.selection, 0.4);
    }

    click = (event) => {
        raycaster.setFromCamera(mouseVector, camera);
        let res = raycaster.intersectObjects(scene.children, true);
        console.log("Selection");

        /* Something has been hitted */
        if (res.length > 0) {
            let obj = this.root(res[0].object);
            /* Clicking on the selection unselect it */
            if (obj == this.selection) {
                this.unSelect();
            } else {
                /* a new object has been selected */
                this.unSelect();
                this.select(obj);
            }
        /* Clicked nowhere -> move selected object if there is */
        } else if (this.selection) {
            let origin = raycaster.ray.origin.clone();
            let end = raycaster.ray.direction
                        .clone()
                        .normalize()
                        .multiplyScalar(MAX_DISTANCE_CLICK)
                        .add(origin);
        
            let line = new THREE.Line3(origin, end);
            let inter = XY_PLANE.intersectLine(line);
            if (inter) {
                // MOVE = 4

                let writer = new BinaryWriter();

                writer.addUInt8(4);

                console.log(this.selection.netId);
                writer.addUInt32(this.selection.netId);
                writer.addReal(inter.x);
                writer.addReal(inter.y);
                writer.addReal(this.selection.position.z);
                websocket.send(writer.getArrayBuffer());

                this.unSelect();
            }
        }
        this.notifyEditor();
    }
}

const MAX_DISTANCE_CLICK = 30;
var selection = new Selection();
// MOUSE
renderer.domElement.addEventListener("mousemove", onMouseMove, false);
renderer.domElement.addEventListener("mousedown", onMouseDown, false);
renderer.domElement.addEventListener("mouseup", onMouseUp, false);
renderer.domElement.addEventListener("click", selection.click, false); // */
// KEYBOARD
MAINLOOP.addCallback(move);
renderer.domElement.addEventListener("keyup", onKeyUp, false);
renderer.domElement.addEventListener("keydown", onKeyDown, false);