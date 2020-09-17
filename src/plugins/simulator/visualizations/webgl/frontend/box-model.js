function spawn(data) {
    const geometry = new THREE.BoxGeometry(...data.scale);
    const material = new THREE.MeshStandardMaterial({ color: 0xff00ff });

    const mesh = new THREE.Mesh(geometry, material);
    setTransform(mesh, data);
    return mesh;
}

function update(data, mesh) {
    setTransformFromBin(mesh, data);
}

OBJECTS.addType("box", {spawn, update});
