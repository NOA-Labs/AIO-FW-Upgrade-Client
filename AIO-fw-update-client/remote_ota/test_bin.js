"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    function adopt(value) { return value instanceof P ? value : new P(function (resolve) { resolve(value); }); }
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : adopt(result.value).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g = Object.create((typeof Iterator === "function" ? Iterator : Object).prototype);
    return g.next = verb(0), g["throw"] = verb(1), g["return"] = verb(2), typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (g && (g = 0, op[0] && (_ = 0)), _) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
Object.defineProperty(exports, "__esModule", { value: true });
var fs = require("fs");
function adjustBinary(path) {
    var file = fs.readFileSync(path);
    var x = new Uint8Array(file);
    var arrayCopy = [];
    var arrayCopyOrg = [];
    for (var i = 0; i < x.byteLength; i++) {
        arrayCopy.push(x[i]);
        arrayCopyOrg.push(x[i]);
    }
    // const strs = ['mode', 'from', 'cause', 'security', 'switch', 'global', 'sniffer', 'channel'];
    var strs = ['destroyed', 'disconnected', 'dynamic', 'secondary', 'Failed', 'incorrect'];
    for (var _i = 0, strs_1 = strs; _i < strs_1.length; _i++) {
        var str = strs_1[_i];
        var hexRepresentation = [];
        for (var i = 0; i < str.length; i++) {
            hexRepresentation.push(Number(str.charCodeAt(i).toString()));
        }
        for (var i = 0; i < arrayCopy.length; i++) {
            var page = Math.floor(i / 4096) + 1;
            // Adjust accordingly to page numbers with issues
            if (page !== 2 && page !== 3 && page !== 38 && page !== 39) {
                continue;
            }
            var j = -1;
            var match = true;
            for (var _a = 0, hexRepresentation_1 = hexRepresentation; _a < hexRepresentation_1.length; _a++) {
                var hex_1 = hexRepresentation_1[_a];
                j++;
                if (arrayCopy[i + j] !== hex_1) {
                    match = false;
                    break;
                }
            }
            if (match) {
                // console.log('match at', i, str);
                for (var j_1 = 0; j_1 < hexRepresentation.length; j_1++) {
                    arrayCopy[i + j_1] = 0x20;
                }
            }
        }
    }
    // for (let i = 0; i < arrayCopy.length; i++) {
    //     if (arrayCopy[i] === 0x45) {
    //         console.log('checksum?', i, i.toString(16));
    //     }
    // }
    // for (let i = 0; i < arrayCopy.length; i++) {
    //     if (arrayCopy[i] === 0x2d && arrayCopy[i + 1] === 0xb9 && arrayCopy[i + 2] === 0x86) {
    //         console.log('sha?', i, i.toString(16));
    //     }
    // }
    var checkSumByteIndex = arrayCopy.length - 33;
    // Adjust to checksum from esptool.py output
    arrayCopy[checkSumByteIndex] = 0x67;
    var crypto = require('crypto');
    var hash = crypto.createHash('sha256');
    hash.update(new Uint8Array(arrayCopy.slice(0, checkSumByteIndex + 1)));
    var hex = hash.digest('hex');
    var decimalArray = Array.from(Buffer.from(hex, 'hex'));
    var k = 0;
    for (var _b = 0, decimalArray_1 = decimalArray; _b < decimalArray_1.length; _b++) {
        var shaPart = decimalArray_1[_b];
        k++;
        arrayCopy[checkSumByteIndex + k] = shaPart;
    }
    fs.writeFileSync(path.replace('.bin', '-NEW2.bin'), new Uint8Array(arrayCopy));
}
function chunkArray(arr, chunk_size) {
    var x = new Uint8Array(arr);
    var arrayCopy = [];
    for (var i = 0; i < x.byteLength; i++) {
        arrayCopy.push(x[i]);
    }
    var results = [];
    while (arrayCopy.length) {
        results.push(arrayCopy.splice(0, chunk_size));
    }
    return results;
}
function prepareBinary(path) {
    return __awaiter(this, void 0, void 0, function () {
        var file, pageChunks;
        return __generator(this, function (_a) {
            file = fs.readFileSync(path);
            pageChunks = chunkArray(file, 4096);
            console.log('Total Page chunks: ', pageChunks.length);
            console.log('Total flash size: ', file.byteLength);
            return [2 /*return*/, {
                    pageChunks: pageChunks,
                    totalLength: file.byteLength,
                }];
        });
    });
}
var main = function () { return __awaiter(void 0, void 0, void 0, function () {
    var files, _i, files_1, file, bin, i, max, _a, _b, chunk, data, offset, hexaddress;
    return __generator(this, function (_c) {
        switch (_c.label) {
            case 0:
                adjustBinary('../build/aio-rt7-172.bin');
                files = [
                    'aio-rt7-172-NEW2.bin',
                ];
                _i = 0, files_1 = files;
                _c.label = 1;
            case 1:
                if (!(_i < files_1.length)) return [3 /*break*/, 4];
                file = files_1[_i];
                return [4 /*yield*/, prepareBinary("../build/".concat(file))];
            case 2:
                bin = _c.sent();
                i = 0;
                max = 0;
                for (_a = 0, _b = bin.pageChunks; _a < _b.length; _a++) {
                    chunk = _b[_a];
                    i++;
                    data = chunk.join(' ');
                    if (data.length >= 14299) { // 14233
                        offset = (i - 1) * 4096;
                        hexaddress = offset.toString(16);
                        console.log(file, data.length, i, offset);
                        console.log('hex', hexaddress);
                    }
                    else {
                        if (data.length > max) {
                            max = data.length;
                        }
                    }
                }
                console.log('max', max);
                _c.label = 3;
            case 3:
                _i++;
                return [3 /*break*/, 1];
            case 4: return [2 /*return*/];
        }
    });
}); };
main();
