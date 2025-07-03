#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// 目标从设备信息
#define TARGET_DEVICE_NAME  "BaitBox-AIO"  
#define SERVICE_UUID        "0000ffe0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ffe1-0000-1000-8000-00805f9b34fb"

BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;
bool deviceFound = false;

enum state{
  SYS_STATE_IDLE = 0,
  SYS_STATE_REBOOT,
  SYS_STATE_SCANNING,
  SYS_STATE_STOP_SCAN,
  SYS_STATE_CONNECTING,
  SYS_STATE_CONNECTED,
  SYS_STATE_DISCONNECTED,
  SYS_STATE_SET_UUID,
  SYS_STATE_WRITE_DATA,
};

uint8_t sysState = SYS_STATE_IDLE;

struct connectDevice{
  std::vector<BLEAdvertisedDevice> dev;
  char sevUUID[64];
  char charUUID[64];
  int index;
};

struct connectDevice device;
// 扫描回调：发现目标设备后触发连接
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if(!advertisedDevice.getName().empty()){
      Serial.printf("Dev:%s [%s]\r\n", advertisedDevice.getName().c_str(), advertisedDevice.getAddress().toString().c_str());
      device.dev.push_back(advertisedDevice);
    }
    
    // if (advertisedDevice.getName() == TARGET_DEVICE_NAME) {
    //   Serial.println("Found target device, retrying connection...");
    //   advertisedDevice.getScan()->stop();
    //   device.device = advertisedDevice;
    //   Serial.printf(" - Connected to device: %s\r\n", device.device.getName().c_str());
    //   Serial.printf(" - Address: %s\r\n", device.device.getAddress().toString().c_str());
    //   Serial.printf(" - RSSI: %d\r\n", device.device.getRSSI());
    //   for(int i = 0; i < device.device.getServiceUUIDCount(); i++){
    //     Serial.printf(" - Service UUID: %s\r\n", device.device.getServiceUUID(i).toString().c_str());
    //   }
    //   for(int i = 0; i < device.device.getServiceDataUUIDCount(); i++){
    //     Serial.printf(" - Charact UUID: %s\r\n", device.device.getServiceDataUUID(i).toString().c_str());
    //   }
    //   deviceFound = true;
    // }
  }
};

uint8_t writeData[] = {0xC0, 0x75, 0x31, 0x52, 0x86, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0xC0};

uint8_t strToHex(const char *str)
{
  uint8_t hex = 0;
  if(str[0] >= '0' && str[0] <= '9'){
    hex = str[0] - '0';
  }
  else if(str[0] >= 'A' && str[0] <= 'F'){
    hex = str[0] - 'A' + 10;
  }
  else if(str[0] >= 'a' && str[0] <= 'f'){
    hex = str[0] - 'a' + 10;
  }

  hex <<= 4;

  if(str[1] >= '0' && str[1] <= '9'){
    hex |= str[1] - '0';
  }
  else if(str[1] >= 'A' && str[1] <= 'F'){
    hex |= str[1] - 'A' + 10;
  }
  else if(str[1] >= 'a' && str[1] <= 'f'){
    hex |= str[1] - 'a' + 10;
  }

  return hex;
}

static void cmdParse(const char* cmd)
{
  Serial.println(cmd);
  if(strstr(cmd, "reboot") != NULL){
    sysState = SYS_STATE_REBOOT;
    ESP.restart();
  }
  else if (strstr(cmd, "scan") != NULL){
    sysState = SYS_STATE_SCANNING;
    device.dev.clear();
  }
  else if (strstr(cmd, "stop") != NULL){
    sysState = SYS_STATE_STOP_SCAN;
    BLEDevice::getScan()->stop();
  }
  else if (strstr(cmd, "connect") != NULL){
    sysState = SYS_STATE_CONNECTING;
    BLEDevice::getScan()->stop();
    char *pos = strchr(cmd, ':');
    char posStr[10] = {0};
    cmd += (pos - cmd);
    while(*cmd != '\r'){
      posStr[0] = *cmd;
      cmd++;
    }
    device.index = atoi(posStr);
  }
  else if(strstr(cmd, "disconnect") != NULL){
    sysState = SYS_STATE_CONNECTING;
    pClient->disconnect();
  }
  else if(strstr(cmd, "uuid|") != NULL){//设置uuid ,uuid|0000ffe0-0000-1000-8000-00805f9b34fb|0000ffe1-0000-1000-8000-00805f9b34fb|
    sysState = SYS_STATE_SET_UUID;
    memset(device.sevUUID, 0, sizeof(device.sevUUID));
    memset(device.charUUID, 0, sizeof(device.charUUID));
    cmd += 5;//跳过uuid|
    char *pos = device.sevUUID;
    while(*cmd != '|' && *cmd != '\0'){
      *pos++ = *cmd++;
    }
    Serial.printf("Service UUID: %s\r\n", device.sevUUID);
    pos = device.charUUID;
    cmd += 1;
    while(*cmd != '|' && *cmd != '\0'){
      *pos++ = *cmd++;
    }
    Serial.printf("Characteristic UUID: %s\r\n", device.charUUID);
  }
  else if(strstr(cmd, "write:") != NULL){//"write:c075315286448490e6098caab580c0"
    sysState = SYS_STATE_WRITE_DATA;
    cmd += 6;
    
    uint8_t *pos = writeData;
    while(*cmd != '\0'){
      char val[3] = {0};
      val[0] = *cmd++;
      val[1] = *cmd++;
      *pos++ = strToHex(val);
    }
  }
}

static void serialReceived(void)
{
  char buffer[128] = {0};
  int length = Serial.available();
  Serial.readBytes(buffer, length);
  cmdParse(buffer);
  memset(buffer, 0, sizeof(buffer));
}

void setup() {
  Serial.begin(115200);
  Serial.onReceive(serialReceived, true);
  BLEDevice::init("ESP32_BLE_Central");
  BLEDevice::getScan()->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  BLEDevice::getScan()->setActiveScan(true);
}

bool connectToServer();
      
void loop() {
  // if (deviceFound) {
  //   connectToServer();
  //   deviceFound = false;
  // }
  switch(sysState){
    case SYS_STATE_IDLE:
      break;
    case SYS_STATE_REBOOT:
      Serial.println("reboot");
      break;
    case SYS_STATE_SCANNING:
      sysState = SYS_STATE_IDLE;
      // 配置扫描参数
      Serial.println("start scanning");
      BLEDevice::getScan()->start(30); // 扫描30秒
    break;
    case SYS_STATE_STOP_SCAN:
      sysState = SYS_STATE_IDLE;
      Serial.println("stop scanning");
    break;
    case SYS_STATE_CONNECTING:
      sysState = SYS_STATE_CONNECTED;
      Serial.println("connecting");
      Serial.printf(" - Connected to device: %s\r\n", device.dev.at(device.index).getName().c_str());
      Serial.printf(" - Address: %s\r\n", device.dev.at(device.index).getAddress().toString().c_str());
      Serial.printf(" - RSSI: %d\r\n", device.dev.at(device.index).getRSSI());
    break;
    case SYS_STATE_CONNECTED:
      sysState = SYS_STATE_IDLE;
      connectToServer();
    break;
    case SYS_STATE_DISCONNECTED:
      sysState = SYS_STATE_IDLE;
    break;
    case SYS_STATE_SET_UUID:
      sysState = SYS_STATE_IDLE;
      // 获取特征值
      if(strlen(device.sevUUID) == 0 || strlen(device.charUUID) == 0){
        Serial.println("Please set service and characteristic UUID");
        break;
      }
      pRemoteCharacteristic = pClient->getService(device.sevUUID)->getCharacteristic(device.charUUID);
      if (pRemoteCharacteristic == nullptr) {
        Serial.println("Not found characteristic");
        break;
      }
            
      // 订阅通知（可选）
      // pRemoteCharacteristic->registerForNotify([](BLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
      //   Serial.printf("Notify: ");
      //   const char hex[] = "0123456789ABCDEF";
      //   for (size_t i = 0; i < length; i++) {
      //     Serial.printf("%c%c ", hex[pData[i] >> 4], hex[pData[i] & 0x0F]);
      //   }
      //   Serial.println();
      // });
    break;
    case SYS_STATE_WRITE_DATA:
      sysState = SYS_STATE_IDLE;
      if (pRemoteCharacteristic == nullptr) {
        Serial.println("Not found characteristic");
        break;
      }
      pRemoteCharacteristic->writeValue(writeData, sizeof(writeData), true);
    break;
    default: 
    break;
  }
  delay(10);
}

// 连接从设备并订阅特征值
bool connectToServer() {
  pClient = BLEDevice::createClient();
  
  bool res = pClient->connect(&device.dev.at(device.index));
  if(res){
    Serial.println(" - Connected to device successfully");
  }
  else{
    Serial.println(" - Failed to connect to device"); 
    return false;
  }
  
  std::map<std::string, BLERemoteService*>* pRemoteServices = pClient->getServices();
  Serial.printf("Found %d services\r\n", pRemoteServices->size());
  for(auto it = pRemoteServices->begin(); it != pRemoteServices->end(); it++){
      std::string serviceUUID = it->first;
      BLERemoteService* pService = it->second;
      Serial.printf("|-- Service UUID: %s\r\n", serviceUUID.c_str());
      std::map<std::string, BLERemoteCharacteristic*>* pCharacteristics = pService->getCharacteristics();
      Serial.printf("   |- Found %d characteristics\r\n", pCharacteristics->size());
      for(auto it2 = pCharacteristics->begin(); it2 != pCharacteristics->end(); it2++){
          BLEUUID charUUID = it2->second->getUUID();
          Serial.printf("|  |-- Characteristic UUID: %s\r\n", charUUID.toString().c_str());
      }
  }
  
  return true;
}
