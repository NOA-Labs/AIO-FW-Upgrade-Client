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
var wifi = require("node-wifi");
var axios_1 = require("axios");
var fs = require("fs");
var CRC32 = require("crc-32");
var _progress = require('cli-progress');
var _colors = require('ansi-colors');
var udp = require('dgram');
var os = require('os');
var WIFI_SSID_PREFIX = "CTLBox";
function ping() {
    return __awaiter(this, void 0, void 0, function () {
        var resp;
        var _a;
        return __generator(this, function (_b) {
            switch (_b.label) {
                case 0: return [4 /*yield*/, axios_1.default.post('http://192.168.4.1/ping', {}, {
                        headers: {
                            Accept: 'application/json',
                        },
                    })];
                case 1:
                    resp = _b.sent();
                    return [2 /*return*/, ((_a = resp === null || resp === void 0 ? void 0 : resp.data) === null || _a === void 0 ? void 0 : _a.test) === true];
            }
        });
    });
}
function getFlashCRC(len) {
    return __awaiter(this, void 0, void 0, function () {
        var resp;
        var _a;
        return __generator(this, function (_b) {
            switch (_b.label) {
                case 0: return [4 /*yield*/, axios_1.default.post('http://192.168.4.1/nextotapartitioncrc', {
                        size: len,
                    }, {
                        headers: {
                            Accept: 'application/json',
                        },
                    })];
                case 1:
                    resp = _b.sent();
                    return [2 /*return*/, (_a = resp === null || resp === void 0 ? void 0 : resp.data) === null || _a === void 0 ? void 0 : _a.crc32];
            }
        });
    });
}
function bootToMain(len, crc32) {
    return __awaiter(this, void 0, void 0, function () {
        var resp;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, axios_1.default.post('http://192.168.4.1/activateota', {
                        crc32: crc32,
                        len: len,
                    }, {
                        headers: {
                            Accept: 'application/json',
                        },
                    })];
                case 1:
                    resp = _a.sent();
                    return [2 /*return*/, resp === null || resp === void 0 ? void 0 : resp.data];
            }
        });
    });
}
function sendChunk(chunk, chunkIdx) {
    return __awaiter(this, void 0, void 0, function () {
        var chunkCRC, resp;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    chunkCRC = CRC32.buf(new Uint8Array(chunk));
                    return [4 /*yield*/, axios_1.default.post('http://192.168.4.1/writenextota', {
                            offset: chunkIdx,
                            data: chunk.join(' '),
                            crc32: chunkCRC,
                        }, {
                            headers: {
                                Accept: 'application/json',
                            },
                        })];
                case 1:
                    resp = _a.sent();
                    return [2 /*return*/, resp === null || resp === void 0 ? void 0 : resp.data];
            }
        });
    });
}
function target_enable_udplog(ipaddr, port) {
    return __awaiter(this, void 0, void 0, function () {
        var resp;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, axios_1.default.post('http://192.168.4.1/udplog', {
                        targetIp: ipaddr,
                        port: port,
                    }, {
                        headers: {
                            Accept: 'application/json',
                        },
                    })];
                case 1:
                    resp = _a.sent();
                    return [2 /*return*/, resp === null || resp === void 0 ? void 0 : resp.data];
            }
        });
    });
}
function chunkArray(arr, chunk_size) {
    /* create a copy of the array */
    var x = new Uint8Array(arr);
    var arrayCopy = [];
    for (var i = 0; i < x.byteLength; i++) {
        arrayCopy.push(x[i]);
    }
    /* create chunks */
    var results = [];
    while (arrayCopy.length) {
        results.push(arrayCopy.splice(0, chunk_size));
    }
    return results;
}
function prepareBinary(path) {
    return __awaiter(this, void 0, void 0, function () {
        var file, crc32, pageChunks;
        return __generator(this, function (_a) {
            file = fs.readFileSync(path);
            crc32 = CRC32.buf(new Uint8Array(file));
            pageChunks = chunkArray(file, 4096);
            console.log('Total Page chunks: ', pageChunks.length);
            console.log('Total flash size: ', file.byteLength);
            console.log('CRC32 is: ', crc32);
            return [2 /*return*/, {
                    pageChunks: pageChunks,
                    crc32: crc32,
                    totalLength: file.byteLength,
                }];
        });
    });
}
function setupStatusBar(len) {
    // create new progress bar
    var b1 = new _progress.Bar({
        format: 'Update Progress |' + _colors.cyan('{bar}') + '| {percentage}% || {value}/{total} Chunks || Speed: {speed}',
        barCompleteChar: '\u2588',
        barIncompleteChar: '\u2591',
        hideCursor: true
    });
    // initialize the bar -  defining payload token "speed" with the default value "N/A"
    b1.start(len, 0, {
        speed: "N/A"
    });
    return b1;
}
function udplog_start_server() {
    return __awaiter(this, void 0, void 0, function () {
        var server;
        return __generator(this, function (_a) {
            server = udp.createSocket('udp4');
            // emits when any error occurs
            server.on('error', function (error) {
                console.log('Error: ' + error);
                server.close();
            });
            //emits when socket is ready and listening for datagram msgs
            server.on('listening', function () {
                var address = server.address();
                var port = address.port;
                var family = address.family;
                var ipaddr = address.address;
                console.log('Server is listening at port: ' + port);
                console.log('Server ip :' + ipaddr);
                console.log('Server is IP4/IP6 : ' + family);
            });
            //emits after the socket is closed using socket.close();
            server.on('close', function () {
                console.log('Socket is closed !');
            });
            server.bind(12345);
            server.on('message', function (msg, info) {
                //console.log('Data received from client : ' + msg.toString());
                //console.log('Received %d bytes from %s:%d\n',msg.length, info.address, info.port);        
                process.stdout.write(msg.toString());
            });
            return [2 /*return*/, 1];
        });
    });
}
function udp_monitor_start() {
    return __awaiter(this, void 0, void 0, function () {
        var connected, networkInfo, networkInterfaceIp, v4ip, l1;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, wifi.getCurrentConnections()];
                case 1:
                    connected = _a.sent();
                    networkInfo = connected.find(function (c) { return c.ssid.startsWith(WIFI_SSID_PREFIX); });
                    if (!networkInfo) return [3 /*break*/, 4];
                    networkInterfaceIp = os.networkInterfaces()[networkInfo['iface']];
                    if (!networkInterfaceIp) return [3 /*break*/, 4];
                    v4ip = networkInterfaceIp.find(function (c) { return c.family === 'IPv4'; });
                    if (!v4ip) return [3 /*break*/, 4];
                    return [4 /*yield*/, target_enable_udplog(v4ip.address, 12345)];
                case 2:
                    l1 = _a.sent();
                    if (!l1.success) return [3 /*break*/, 4];
                    return [4 /*yield*/, udplog_start_server()];
                case 3:
                    _a.sent();
                    return [2 /*return*/, true];
                case 4: return [2 /*return*/, false];
            }
        });
    });
}
function update() {
    return __awaiter(this, void 0, void 0, function () {
        var l, binaryData, b1, barValue, startTimestamp, timer, i, chunk, chunkResponse, x, bootMainResult;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, ping()];
                case 1:
                    l = _a.sent();
                    if (l !== true) {
                        console.error("unable to ping device");
                        process.exit(1);
                    }
                    return [4 /*yield*/, prepareBinary('../build/ESP32-AIO-MAINBOARD_1_1_1.bin')];
                case 2:
                    binaryData = _a.sent();
                    b1 = setupStatusBar(binaryData.pageChunks.length);
                    barValue = 0;
                    startTimestamp = Date.now() / 1000;
                    timer = setInterval(function () {
                        // update the bar value
                        var deltaT = (Date.now() / 1000) - startTimestamp;
                        b1.update(barValue, {
                            speed: (barValue / deltaT).toFixed(2) + "Chunks/s"
                        });
                        // set limit
                        if (barValue >= b1.getTotal()) {
                            // stop timer
                            clearInterval(timer);
                            b1.stop();
                        }
                    });
                    i = 0;
                    _a.label = 3;
                case 3:
                    if (!(i < binaryData.pageChunks.length)) return [3 /*break*/, 6];
                    chunk = binaryData.pageChunks[i];
                    return [4 /*yield*/, sendChunk(chunk, i)];
                case 4:
                    chunkResponse = _a.sent();
                    barValue++;
                    if (!chunkResponse.success) {
                        console.error("error writing chunk: ", chunkResponse, i);
                        process.exit(2);
                    }
                    _a.label = 5;
                case 5:
                    i++;
                    return [3 /*break*/, 3];
                case 6: return [4 /*yield*/, getFlashCRC(binaryData.totalLength)];
                case 7:
                    x = _a.sent();
                    console.log("getFlashCRC result: ", x, binaryData.crc32);
                    if (!(x && x === binaryData.crc32)) return [3 /*break*/, 9];
                    return [4 /*yield*/, bootToMain(binaryData.totalLength, binaryData.crc32)];
                case 8:
                    bootMainResult = _a.sent();
                    console.log("bootMainResult result: ", bootMainResult);
                    _a.label = 9;
                case 9: return [2 /*return*/];
            }
        });
    });
}
function check_connected() {
    return __awaiter(this, void 0, void 0, function () {
        var connected;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0: return [4 /*yield*/, wifi.getCurrentConnections()];
                case 1:
                    connected = _a.sent();
                    return [2 /*return*/, connected.find(function (c) { return c.ssid.startsWith(WIFI_SSID_PREFIX); }) !== undefined];
            }
        });
    });
}
function handle_args() {
    return __awaiter(this, void 0, void 0, function () {
        return __generator(this, function (_a) {
            switch (process.argv[2]) {
                case 'monitor':
                    return [2 /*return*/, udp_monitor_start()];
                case 'flash':
                default:
                    return [2 /*return*/, update()];
            }
            return [2 /*return*/, 0];
        });
    });
}
function worker() {
    return __awaiter(this, void 0, void 0, function () {
        var list, ssid, i;
        var _a;
        return __generator(this, function (_b) {
            switch (_b.label) {
                case 0:
                    wifi.init({
                        iface: null // network interface, choose a random wifi interface if set to null
                    });
                    /* check if we're already connected */
                    console.log("checking connected networks");
                    return [4 /*yield*/, check_connected()];
                case 1:
                    if (_b.sent()) {
                        return [2 /*return*/, handle_args()];
                    }
                    /* scan wifi networks */
                    console.log("scanning wifi networks");
                    return [4 /*yield*/, wifi.scan()];
                case 2:
                    list = _b.sent();
                    ssid = (_a = list.find(function (c) { return c.ssid.startsWith(WIFI_SSID_PREFIX); })) === null || _a === void 0 ? void 0 : _a.ssid;
                    if (!ssid) {
                        console.error("unable to find wifi");
                        process.exit(1);
                        return [2 /*return*/];
                    }
                    /* connect */
                    console.log("connecting to network.", ssid);
                    return [4 /*yield*/, wifi.connect({ ssid: ssid, password: '12345678' })];
                case 3:
                    _b.sent();
                    i = 0;
                    _b.label = 4;
                case 4:
                    if (!(i < 10)) return [3 /*break*/, 8];
                    console.log("Waiting for connection ", i);
                    return [4 /*yield*/, new Promise(function (resolve) { return setTimeout(resolve, 1000); })];
                case 5:
                    _b.sent();
                    return [4 /*yield*/, check_connected()];
                case 6:
                    if (_b.sent()) {
                        return [2 /*return*/, handle_args()];
                    }
                    _b.label = 7;
                case 7:
                    i++;
                    return [3 /*break*/, 4];
                case 8: return [2 /*return*/];
            }
        });
    });
}
worker();
