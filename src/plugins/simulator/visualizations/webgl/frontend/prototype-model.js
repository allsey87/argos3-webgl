const PROTOTYPE_GEOMETRIES = {
    "box": THREE.BoxGeometry,
    "cylinder": THREE.CylinderGeometry,
    "sphere": THREE.SphereGeometry
};

function spawn(data) {
    const root = new THREE.Group();
    setTransform(root, data);

    for (let child of data.children) {
        const {type, scale, position, rotation, name} = child;
        const geometry = new PROTOTYPE_GEOMETRIES[type](...scale);
        const material = new THREE.MeshStandardMaterial({ color: 0x1af055 });

        const mesh = new THREE.Mesh(geometry, material);
        mesh.matrix.makeRotationFromEuler(new THREE.Euler(...rotation));
        mesh.position.set(...position);
        mesh.name = name;
        root.add(mesh);
    }

    root.luaScript = data.luaScript;
    return root;
}

function update(data, root) {
    if (!!data.extractUint8()) {
        setTransformFromBin(root, data);
    }

    while (!data.isConsumed()) {
        const mesh = root.children[data.extractUint8()];
        setTransformFromBin(mesh, data);
    }
}

OBJECTS.addType("prototype", {spawn, update});
