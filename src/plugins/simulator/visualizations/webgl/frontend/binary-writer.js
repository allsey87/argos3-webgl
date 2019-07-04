class BinaryWriter {
    constructor() {
        this.bytes = [];
        this.buffer = new ArrayBuffer(64);
        this.dataView = new DataView(this.buffer);
    }

    addUInt8(uByte) {
        this.bytes.push(uByte);
    }

    addInt8(sByte) {
        this.addUInt8(new Uint8Array([sByte])[0]);
    }

    addUInt16(uShort) {
        this.dataView.setUint16(0, uShort);
        this._fetch(2);
    }

    addInt32(sInt) {
        this.dataView.setInt32(0, sInt);
        this._fetch(4);
    }

    addUInt32(uInt) {
        this.dataView.setUint32(0, uInt);
        this._fetch(4);
    }

    addBigInt64(sLong) {
        this.dataView.setBigInt64(0, sLong);
        this._fetch(8);
    }

    /**
     * Fetch bytes from buffer to bytes
     */
    _fetch(byteCount) {
        for (let i = 0; i < byteCount; ++i) {
            this.bytes.push(this.dataView.getUint8(i));
        }
    }

    addReal(real) {
        let nMantissa;
        let [shifted, nExponent] = frexp(real);
        shifted = Math.abs(shifted) - 0.5;
        if (shifted < 0) {
            nMantissa = BigInt(0);
        } else {
            nMantissa = BigInt(shifted * 2.0 * (MAX_MANTISSA)) + BigInt(1);
            if (real < 0) {
                nMantissa = -nMantissa;
            }
        }

        this.addBigInt64(nMantissa);
        this.addInt32(nExponent);
    }

    getArrayBuffer() {
        return (new Uint8Array(this.bytes)).buffer;
    }
}