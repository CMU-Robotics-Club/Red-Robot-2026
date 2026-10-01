#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "ble.h"

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

bool deviceConnected = false;

// Handle BLE connection events to restart advertising when disconnected
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Client disconnected, restarting advertising...");
    BLEDevice::startAdvertising();
  }
};

// Handle incoming speed data sent over NUS RX characteristic from nRF Connect /
// Bluefruit
class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue().c_str();

    if (rxValue.length() > 0) {
      int16_t incoming_speed = (int16_t)rxValue.toInt();
      incoming_speed = constrain(incoming_speed, -100, 100);

      Serial.printf("[BLE] Speed updated to: %d\n", incoming_speed);
      global_motor_speed = incoming_speed;
    }
  }
};

namespace BLE {
void init() {
  // Initialize BLE device name
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT); // Read unique Bluetooth MAC

  char deviceName[30];
  snprintf(deviceName, sizeof(deviceName), "ESP32_Motor_%02X%02X", mac[4],
           mac[5]);

  BLEDevice::init(deviceName);

  // Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create Nordic UART Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create TX Characteristic (ESP32 -> Mobile App)
  BLECharacteristic *pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create RX Characteristic (Mobile App -> ESP32)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  // Start Nordic UART Service
  pService->start();

  // Configure advertising data so nRF Connect / Bluefruit auto-detect NUS
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Helps with iOS connection stability
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();
  Serial.printf(
      "[BLE] Nordic UART Service advertising as '%s'\n", deviceName);
}

} // namespace BLE