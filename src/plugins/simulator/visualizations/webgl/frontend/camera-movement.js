const KEYCODES = {
    RIGHT: 39,
    LEFT: 37,
    FORWARD: 38,
    BACK: 40
};

const SPEED = 0.2;
let direction = null;
let isMousePressed = false;
let dist = 2;

function onMouseDown() {
    isMousePressed = true;
}

function onMouseUp() {
    isMousePressed = false;
}

let right = () => new THREE.Vector3(1, 0, 0);
let ttop = () => new THREE.Vector3(0, 1, 0);
let forward = () => new THREE.Vector3(0, 0, 1);

function vertical(event) {
    let axis = right().applyQuaternion(camera.quaternion);
    let angleY = -0.01 * event.movementY;

    let q = new THREE.Quaternion();
    q.setFromAxisAngle(axis, angleY);
    camera.quaternion.multiply(q);
}


function horizontal(event) {
    let up = ttop().applyQuaternion(camera.quaternion);

    let angleX = 0.01 * event.movementX;

    let q = new THREE.Quaternion();
    q.setFromAxisAngle(up, angleX);
    camera.quaternion.multiply(q);
}

function onMouseMove(event) {
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

// MOUSE
document.addEventListener("mousemove", onMouseMove, false);
document.addEventListener("mousedown", onMouseDown, false);
document.addEventListener("mouseup", onMouseUp, false);
// KEYBOARD
MAINLOOP.addCallback(move);
document.addEventListener("keyup", onKeyUp, false);
document.addEventListener("keydown", onKeyDown, false);