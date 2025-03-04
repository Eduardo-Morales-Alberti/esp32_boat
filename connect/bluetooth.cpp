#include "bluetooth.h"

// Global pointer to our characteristic
BLECharacteristic *pCharacteristic = nullptr;

/*=========================================================================
   Server Callbacks
  -----------------------------------------------------------------------*/
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    Serial.println("Client disconnected, restarting advertising");
    BLEDevice::startAdvertising();
  }
};

/*=========================================================================
   Characteristic Callbacks
  -----------------------------------------------------------------------*/
class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) override {
    uint8_t responseValue[4] = { 0xde, 0xed, 0xbe, 0xef };
    pCharacteristic->setValue(responseValue, sizeof(responseValue));
    Serial.println("Characteristic read");
  }

  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      Serial.print("Received Value: ");
      Serial.println(rxValue);

      // Call a function to parse the received string, ensure this is declared elsewhere
      parseDataComunication(rxValue.c_str());
    }
  }
};

class MyDescriptorCallbacks : public BLEDescriptorCallbacks {
  void onWrite(BLEDescriptor *pDescriptor) override {
    uint8_t* data = pDescriptor->getValue();
    size_t len = pDescriptor->getLength();
    String value = "";
    for (size_t i = 0; i < len; i++) {
      value += (char)data[i];
    }
    if (len == 2) {
      uint16_t descrValue = ((uint8_t)value[1] << 8) | (uint8_t)value[0];
      if (descrValue == 0x0001) {
        Serial.println("Notify enabled");
        uint8_t notifyData[15];
        for (int i = 0; i < 15; i++) {
          notifyData[i] = i % 0xff;
        }
        pCharacteristic->setValue(notifyData, sizeof(notifyData));
        pCharacteristic->notify();
      } else if (descrValue == 0x0002) {
        Serial.println("Indicate enabled");
        uint8_t indicateData[15];
        for (int i = 0; i < 15; i++) {
          indicateData[i] = i % 0xff;
        }
        pCharacteristic->setValue(indicateData, sizeof(indicateData));
        pCharacteristic->indicate();
      } else if (descrValue == 0x0000) {
        Serial.println("Notify/Indicate disabled");
      } else {
        Serial.print("Unknown descriptor value: ");
        for (size_t i = 0; i < len; i++) {
          Serial.printf("%02x ", data[i]);
        }
        Serial.println();
      }
    }
  }
};

void initializeBluetooth() {
  Serial.begin(115200);
  Serial.println("Starting BLE work...");

  // Initialize BLE and set the device name.
  BLEDevice::init(DEVICE_NAME);

  // Create the BLE Server and set its callbacks.
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service.
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create the BLE Characteristic with read, write, notify, and indicate properties.
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  // Set an initial value for the characteristic.
  uint8_t initialValue[] = { 0x11, 0x22, 0x33 };
  pCharacteristic->setValue(initialValue, sizeof(initialValue));

  // Add a custom descriptor and set its callback.
  BLEDescriptor* pDescriptor = new BLE2902();
  pDescriptor->setCallbacks(new MyDescriptorCallbacks());
  pCharacteristic->addDescriptor(pDescriptor);

  // Start the service.
  pService->start();

  // Start advertising the service.
  startAdvertising();
}

void startAdvertising() {
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  // Optional: adjust advertising parameters (min/max preferred values)
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising started");
  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);
  Serial.print("Characteristic UUID: ");
  Serial.println(CHARACTERISTIC_UUID);
}
