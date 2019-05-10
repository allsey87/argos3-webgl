class BinaryExtractor {
    constructor(buffer){
        this.dv = new DataView(buffer);
        this.offset = 0;
    }

    extractReal() {
        let nMantissa = this.extractBigInt64();
        if (nMantissa.toString() == "0") {
            return 0;
        } else {
            let nExponent = this.extractInt32();
            let significant = Number(lAbs(nMantissa) - BigInt(1)) / MAX_MANTISSA / 2 + 0.5;
            return significant * Math.pow(2, nExponent) * Math.sign(Number(nMantissa));
        }
    }

    isConsumed() {
        return this.offset == this.dv.byteLength;
    }

    extractUint8() {
        return this.dv.getUint8(this.offset++);
    }

    extractUint32() {
        let res = this.dv.getUint32(this.offset, false);
        this.offset += 4;
        return res;
    }

    extractInt32() {
        let res = this.dv.getInt32(this.offset, false);
        this.offset += 4;
        return res;
    }

    extractUint16() {
        let res = this.dv.getUint16(this.offset, false);
        this.offset += 2;
        return res;
    }

    extractBigInt64(){
        let res = this.dv.getBigInt64(this.offset, false);
        this.offset += 8;
        return res;
    }
}