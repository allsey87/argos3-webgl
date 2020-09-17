function spawn(data) {
    const {scale} = data;
    const geometry = new THREE.CylinderGeometry(scale[0], ...scale);
    const material = new THREE.MeshStandardMaterial({ color: 0xff0000 });

    const mesh = new THREE.Mesh(geometry, material);
    setTransform(mesh, data);
    return mesh;
}

function update(data, mesh) {
    setTransformFromBin(mesh, data);
}


OBJECTS.addType("box", {spawn, update});