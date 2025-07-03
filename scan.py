import asyncio
from bleak import BleakScanner

async def scan_devices():
    devices = await BleakScanner.discover()
    for d in devices: print(d.name, d.address)

asyncio.run(scan_devices())
