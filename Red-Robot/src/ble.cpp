#include <Arduino.h>
#include <NimBLEDevice.h>
#include <esp_mac.h>

#include "ble.h"
#include "pinouts.h"

// Set to 1 for one boot to wipe stale bonds (e.g. after changing security
// settings or if a phone refuses to reconnect), then set back to 0.
#define BLE_CLEAR_BONDS_ON_BOOT 0

// Nordic UART Service (NUS) UUIDs
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

namespace {
volatile bool deviceConnected = false;
char gDeviceName[30];

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
    NimBLEDevice::startSecurity(connInfo.getConnHandle());
  }

  void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo,
                    int reason) override {
    deviceConnected = false;
    Serial.printf("[BLE] Client disconnected (reason %d)\n", reason);
    // advertiseOnDisconnect(true) restarts advertising; the watchdog task
    // below is the backstop if that ever fails.
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
    size_t len = rxValue.length();
    if (len == 0)
      return;

    // Copy into a NUL-terminated buffer so sscanf can't run past the data
    char buf[32];
    if (len >= sizeof(buf))
      len = sizeof(buf) - 1;
    memcpy(buf, rxValue.data(), len);
    buf[len] = '\0';

    int channel, speed;
    if (sscanf(buf, "%d %d", &channel, &speed) == 2) {
      channel = constrain(channel, 0, 3);
      speed = constrain(speed, -100, 100);

      Serial.printf("[BLE] Speeds updated: motor[%d] - %d\n", channel, speed);

      global_motor_speed[channel] = (int16_t)speed;
    } else {
      Serial.println("[BLE] Invalid speed command");
    }
  }
};

bool startAdv() {
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  if (adv->isAdvertising())
    return true;
  bool ok = adv->start();
  Serial.printf("[BLE] adv->start() -> %s, isAdvertising=%d\n",
                ok ? "ok" : "FAILED", (int)adv->isAdvertising());
  return ok;
}

// Backstop: if advertising ever stops while no client is connected, restart it
void advWatchdogTask(void *) {
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(2000));
    NimBLEServer *srv = NimBLEDevice::getServer();
    bool connected = srv && srv->getConnectedCount() > 0;
    bool advertising = NimBLEDevice::getAdvertising()->isAdvertising();

    digitalWrite(RADIO_LED, (advertising || connected) ? HIGH : LOW);

    if (!connected && !advertising) {
      Serial.println("[BLE] Advertising stopped unexpectedly, restarting");
      startAdv();
    }
  }
}
} // namespace

namespace BLE {
void init() {
  pinMode(RADIO_LED, OUTPUT);
  digitalWrite(RADIO_LED, LOW);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  snprintf(gDeviceName, sizeof(gDeviceName), "RedRobot_%02X%02X", mac[4],
           mac[5]);

  NimBLEDevice::init(gDeviceName);
  NimBLEDevice::setPower(9); // dBm, max range

#if BLE_CLEAR_BONDS_ON_BOOT
  NimBLEDevice::deleteAllBonds();
#endif

  // Static passkey, bonding + MITM + secure connections
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityPasskey(BLE_STATIC_PIN);

  // GATT server
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  pServer->advertiseOnDisconnect(true);

  NimBLEService *pService = pServer->createService(SERVICE_UUID);

  // TX (ESP32 -> app). CCCD is added automatically for NOTIFY.
  pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
                                 NIMBLE_PROPERTY::NOTIFY);

  // RX (app -> ESP32)
  NimBLECharacteristic *pRx = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR |
          NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
  pRx->setCallbacks(new RxCallbacks());

  pServer->start();

  // Explicit advertising payload so it can't overflow 31 bytes:
  //   adv packet:    flags (3) + 128-bit service UUID (18) = 21 bytes
  //   scan response: complete local name
  NimBLEAdvertisementData advData;
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  advData.addServiceUUID(SERVICE_UUID);

  NimBLEAdvertisementData scanData;
  scanData.setName(gDeviceName);

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->setScanResponseData(scanData);
  pAdvertising->enableScanResponse(true);
  pAdvertising->setMinInterval(0x30); // 30 ms
  pAdvertising->setMaxInterval(0x60); // 60 ms

  bool ok = startAdv();

  Serial.printf("[BLE] '%s' addr %s advertising=%d\n", gDeviceName,
                NimBLEDevice::getAddress().toString().c_str(), (int)ok);
  digitalWrite(RADIO_LED, ok ? HIGH : LOW);

  xTaskCreate(advWatchdogTask, "bleAdvWd", 3072, nullptr, 1, nullptr);
}

} // namespace BLE