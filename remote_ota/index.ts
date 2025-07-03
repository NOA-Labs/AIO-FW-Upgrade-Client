import * as wifi from 'node-wifi'
import axios from 'axios';
import * as fs from 'fs';
import * as CRC32 from 'crc-32';
const _progress = require('cli-progress');
const _colors = require('ansi-colors');
const udp = require('dgram');
const os = require('os');

declare var process : any;

const WIFI_SSID_PREFIX = "CTLBox";

async function ping(){
    const resp = await axios.post('http://192.168.4.1/ping', {},   {
        headers: {
            Accept: 'application/json',
        },
    });

    return resp?.data?.test === true;
}

async function getFlashCRC(len: number){
    const resp = await axios.post('http://192.168.4.1/nextotapartitioncrc', 
    {
        size: len,
    },   
    {
        headers: {
            Accept: 'application/json',
        },
    });

    return resp?.data?.crc32;   
}

async function bootToMain(len: number, crc32: number){
    const resp = await axios.post('http://192.168.4.1/activateota', 
    {
        crc32,
        len,
    },   
    {
        headers: {
            Accept: 'application/json',
        },
    });

    return resp?.data;   
}

async function sendChunk(chunk: any[], chunkIdx: number){
    const chunkCRC = CRC32.buf(new Uint8Array(chunk));
    //console.log("chunk crc is", chunkCRC);

    const resp = await axios.post('http://192.168.4.1/writenextota', 
    {
        offset: chunkIdx,
        data: chunk.join(' '),
        crc32: chunkCRC,
    },
    {
        headers: {
            Accept: 'application/json',
        },
    });

    return resp?.data;
}


async function target_enable_udplog(ipaddr: string, port: number){

    const resp = await axios.post('http://192.168.4.1/udplog', 
    {
        targetIp: ipaddr,
        port,
    },
    {
        headers: {
            Accept: 'application/json',
        },
    });

    return resp?.data;
}

function chunkArray(arr: ArrayBuffer, chunk_size: number): Array<any> {
    /* create a copy of the array */
    const x = new Uint8Array(arr);
    const arrayCopy = [];
    for (let i = 0; i < x.byteLength; i++) {
        arrayCopy.push(x[i]);
    }

    /* create chunks */
    const results = [];
    while (arrayCopy.length) {
        results.push(arrayCopy.splice(0, chunk_size));
    }

    return results;
}

async function prepareBinary(path: string){
    const file = fs.readFileSync(path);
    const crc32 = CRC32.buf(new Uint8Array(file));
    const pageChunks = chunkArray(file, 4096);

    console.log('Total Page chunks: ', pageChunks.length);
    console.log('Total flash size: ', file.byteLength);
    console.log('CRC32 is: ', crc32);
    return {
        pageChunks,
        crc32,
        totalLength: file.byteLength,
    }
}

function setupStatusBar(len: number){
    // create new progress bar
    const b1 = new _progress.Bar({
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

async function udplog_start_server(){
    const server = udp.createSocket('udp4');
    
    // emits when any error occurs
    server.on('error', (error: any) => {
        console.log('Error: ' + error);
        server.close();
    });

    //emits when socket is ready and listening for datagram msgs
    server.on('listening',() => {
        var address = server.address();
        var port = address.port;
        var family = address.family;
        var ipaddr = address.address;
        console.log('Server is listening at port: ' + port);
        console.log('Server ip :' + ipaddr);
        console.log('Server is IP4/IP6 : ' + family);
    });

    //emits after the socket is closed using socket.close();
    server.on('close',function(){
        console.log('Socket is closed !');
    });
    
    server.bind(12345);

    server.on('message', (msg: any, info: any) => {
        //console.log('Data received from client : ' + msg.toString());
        //console.log('Received %d bytes from %s:%d\n',msg.length, info.address, info.port);        
        process.stdout.write(msg.toString());
    });

    return 1;
}

async function udp_monitor_start(){

    /* get our ipv4, enable udp logging via http post & start a server socket */
    const connected = await wifi.getCurrentConnections();
    const networkInfo = connected.find(c => c.ssid.startsWith(WIFI_SSID_PREFIX));
    if(networkInfo) {
        var networkInterfaceIp = os.networkInterfaces()[(networkInfo as any)['iface']];
        if(networkInterfaceIp){
            const v4ip = networkInterfaceIp.find((c: any) => c.family === 'IPv4');
            if(v4ip) {
                const l1 = await target_enable_udplog(v4ip.address, 12345);
                if(l1.success) {
                    await udplog_start_server(); 
                    return true;
                }
            }
        }
    }

    return false;
}


async function update(){

    /* check if connection succeded */
    const l = await ping();
    if(l !== true){
        console.error("unable to ping device");
        process.exit(1);        
    }

    /* get binary data & transfer pages */
    // const binaryData = await prepareBinary('../build/ESP32-AIO-MAINBOARD_1_1_2.bin');
    console.log("preparing binary:", process.argv[3]);
    const binaryData = await prepareBinary(process.argv[3]);
    /* setup a status bar */
    const b1 = setupStatusBar(binaryData.pageChunks.length);

    let barValue = 0;
    let startTimestamp = Date.now() / 1000;

    // 20ms update rate
    const timer = setInterval(function(){
        // update the bar value
        const deltaT = (Date.now() / 1000) - startTimestamp;
        b1.update(barValue, {
            speed: (barValue / deltaT).toFixed(2) + "Chunks/s"
        });

        // set limit
        if (barValue >= b1.getTotal()){
            // stop timer
            clearInterval(timer);
            b1.stop();
        }        
    });

    for(let i = 0; i < binaryData.pageChunks.length; i++){
        const chunk = binaryData.pageChunks[i];
        const chunkResponse = await sendChunk(chunk, i);
        barValue++;
        
        var num = i / binaryData.pageChunks.length * 100;
        console.log("response: ", chunkResponse, num.toFixed(2) + "%");

        if(!chunkResponse.success){
            console.error("error writing chunk: ", chunkResponse, i);
            process.exit(2);
        }
    }

    /* get flash crc & boot to new app */
    const x = await getFlashCRC(binaryData.totalLength);
    console.log("getFlashCRC result: ", x, binaryData.crc32);
    if(x && x === binaryData.crc32) {
        const bootMainResult = await bootToMain(binaryData.totalLength, binaryData.crc32);
        console.log("bootMainResult result: ", bootMainResult);
    }
}

async function check_connected() {
    const connected = await wifi.getCurrentConnections();
    console.log("connected networks: ", connected);
    return connected.find(c => c.bssid?.startsWith(WIFI_SSID_PREFIX)) !== undefined;
}

async function handle_args(){
    console.log("args: ", process.argv);
    switch(process.argv[2]){
        case 'monitor':
            return udp_monitor_start(); 
        case 'flash':
        default:
            return update();
    }

    return 0;
}

// async function worker(){
//     wifi.init({
//         iface: null // network interface, choose a random wifi interface if set to null
//     });

//     /* check if we're already connected */
//     console.log("checking connected networks");
//     if(await check_connected()){
//         return handle_args();
//     }

//     /* scan wifi networks */
//     console.log("scanning wifi networks");
//     const list = await wifi.scan();
//     const ssid = list.find(c => c.ssid.startsWith(WIFI_SSID_PREFIX))?.ssid;
//     if(!ssid){
//         console.error("unable to find wifi");
//         process.exit(1);
//         return;
//     }

//     /* connect */
//     console.log("connecting to network.", ssid);
//     await wifi.connect({ssid, password: '12345678'});

//     /* wait for connection to succeed */
//     for(let i = 0; i < 20; i++){
//         console.log("Waiting for connection ", i);
//         await new Promise((resolve) => setTimeout(resolve, 1000));
//         if(await check_connected()){
//             return handle_args();
//         }
//     }
// }

async function worker(){
    wifi.init({
        iface: null // network interface, choose a random wifi interface if set to null
    });

    /* check if we're already connected */
    console.log("checking connected networks");
    if(await check_connected()){
        return handle_args();
    }

    let ssid = null;
    const maxAttempts = 60; // 15秒超时（每秒1次）
    const startTime = Date.now();


    /* scan wifi networks */
    console.log("scanning wifi networks");
    while(!ssid){
        try{
            const list = await wifi.scan();
            ssid = list.find(c => c.ssid.startsWith(WIFI_SSID_PREFIX))?.ssid;
            if(ssid){
                console.log("Found target network:", ssid);
                break;
            }

            console.log("Scaning Device ...");
        }
        catch(e){
            console.error("Scan failed:", e);
        }

         // 超时检查
        if ((Date.now() - startTime) >= maxAttempts * 1000) {
            console.error("No target network found within 15 seconds");
            process.exit(1);
            return;
        }
        
        // 指数退避等待（基础1秒 + 随机抖动）
        const delay = 1000 + Math.random() * 500;
        await new Promise(resolve => setTimeout(resolve, delay));
    }
    

    /* connect */
    console.log("connecting to network.", ssid);
    await wifi.connect({ssid, password: '12345678'});

    /* wait for connection to succeed */
    for(let i = 0; i < 20; i++){
        console.log("Waiting for connection ", i);
        await new Promise((resolve) => setTimeout(resolve, 1000));
        if(await check_connected()){
            return handle_args();
        }
    }
}

worker();  
