#include <Arduino.h>
#include <NimBLEDevice.h>

#include "ble.h"

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

namespace {
bool deviceConnected = false;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
    NimBLEDevice::startSecurity(connInfo.getConnHandle());
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo,
                    int reason) override {
    deviceConnected = false;
    Serial.printf("[BLE] Client disconnected (reason %d), advertising...\n",
                  reason);
    NimBLEDevice::startAdvertising();
  }

  void onAuthenticationComplete(NimBLEConnInfo &connInfo) override {
    if (!connInfo.isEncrypted()) {
      Serial.println("[BLE] Pairing failed, disconnecting");
      NimBLEDevice::getServer()->disconnect(connInfo.getConnHandle());
    }
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic *pCharacteristic,
               NimBLEConnInfo &connInfo) override {
    NimBLEAttValue rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      int16_t incoming_speed = (int16_t)String(rxValue.c_str()).toInt();
      incoming_speed = constrain(incoming_speed, -100, 100);

      Serial.printf("[BLE] Speed updated to: %d\n", incoming_speed);
      global_motor_speed = incoming_speed;
    }
  }
};
} // namespace

namespace BLE {
void init() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);

  char deviceName[30];
  snprintf(deviceName, sizeof(deviceName), "RedRobot_%02X%02X", mac[4], mac[5]);

  NimBLEDevice::init(deviceName);

  // Set static pin
  NimBLEDevice::setSecurityAuth(true, true,
                                true); // bonding, MITM, secure connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityPasskey(BLE_STATIC_PIN);

  // Create BLE Server
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  pServer->advertiseOnDisconnect(true);

  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX Characteristic (ESP32 -> Mobile App)
  // NimBLE adds the 0x2902 CCCD automatically for NOTIFY/INDICATE
  NimBLECharacteristic *pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY);

  // Create RX Characteristic (Mobile App -> ESP32)
  NimBLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR |
          NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  // Configure advertising data so nRF Connect / Bluefruit auto-detect NUS
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->enableScanResponse(true);
  pAdvertising->setPreferredParams(0x06,
                                   0x12); // Helps with iOS connection stability

  NimBLEDevice::startAdvertising();
  Serial.printf("[BLE] Nordic UART Service advertising as '%s'\n", deviceName);
}

} // namespace BLE