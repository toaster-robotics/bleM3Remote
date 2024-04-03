#include <NimBLEDevice.h>

const char HID_SERVICE[] = "1812";
const char HID_REPORT_DATA[] = "2A4D";

static NimBLEAdvertisedDevice *advDevice;

static bool doConnect = false;
static uint32_t scanTime = 0;
uint client_count = 0;

const std::vector<NimBLEAddress> mac_whitelist = {
    NimBLEAddress("2A:07:98:03:BA:19"),
    NimBLEAddress("2a:07:98:00:03:b6"),
};

std::vector<uint8_t> button_value = {0, 0, 0, 0, 0, 0};
const std::vector<uint8_t> key_up = {1, 0, 0, 2, 48, 0};
const std::vector<uint8_t> key_down = {1, 0, 0, 2, 208, 255};
const std::vector<uint8_t> key_left = {1, 0, 0, 150, 0, 0};
const std::vector<uint8_t> key_right = {1, 0, 0, 106, 15, 0};
const std::vector<uint8_t> key_center = {1, 0, 0, 0, 0, 0};
const std::vector<uint8_t> key_camera = {1, 0, 0, 10, 16, 254};

bool keyMatch(const std::vector<uint8_t> &vec1, const std::vector<uint8_t> &vec2)
{
    for (size_t i = 0; i < vec1.size(); ++i)
    {
        if (vec1[i] != vec2[i])
        {
            return false;
        }
    }
    return true;
}

bool checkMac(const NimBLEAddress &dev)
{
    for (auto &it : mac_whitelist)
    {
        if (dev.equals(it))
        {
            return true;
        }
    }
    return false;
}

void scanEndedCB(NimBLEScanResults results);

class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient)
    {
        client_count++;
        Serial.println("Connected");
        pClient->updateConnParams(120, 120, 0, 60);
    };

    void onDisconnect(NimBLEClient *pClient)
    {
        client_count--;
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
        NimBLEDevice::deleteClient(pClient);
        NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    };

    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        return true;
    };
};

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{

    void onResult(NimBLEAdvertisedDevice *advertisedDevice)
    {
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(NimBLEUUID(HID_SERVICE)) && checkMac(advertisedDevice->getAddress()))
        {
            Serial.printf("onResult: AdvType= %d\r\n", advertisedDevice->getAdvType());
            Serial.print("Advertised HID Device found: ");
            Serial.println(advertisedDevice->toString().c_str());

            if (client_count > NIMBLE_MAX_CONNECTIONS)
            {
                Serial.println("no more connections");
                NimBLEDevice::getScan()->stop();
            }
            advDevice = advertisedDevice;
            doConnect = true;
        }
    };
};

void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (length == 3)
    {
        button_value[0] = button_value[3];
        button_value[1] = button_value[4];
        button_value[2] = button_value[5];
        button_value[3] = pData[0];
        button_value[4] = pData[1];
        button_value[5] = pData[2];

        if (keyMatch(button_value, key_up))
        {
            Serial.println("key: up");
        }
        else if (keyMatch(button_value, key_down))
        {
            Serial.println("key: down");
        }
        else if (keyMatch(button_value, key_left))
        {
            Serial.println("key: left");
        }
        else if (keyMatch(button_value, key_right))
        {
            Serial.println("key: right");
        }
        else if (keyMatch(button_value, key_center))
        {
            Serial.println("key: center");
        }
        else if (keyMatch(button_value, key_camera))
        {
            Serial.println("key: camera");
        }
    }
}

void scanEndedCB(NimBLEScanResults results)
{
    Serial.println("Scan Ended");
}

static ClientCallbacks clientCB;

bool connectToServer()
{
    NimBLEClient *pClient = nullptr;

    pClient = NimBLEDevice::createClient();

    Serial.println("New client created");

    pClient->setClientCallbacks(&clientCB, false);
    pClient->setConnectionParams(12, 12, 0, 51);
    pClient->setConnectTimeout(5);

    Serial.println("hello");

    if (!pClient->connect(advDevice))
    {
        NimBLEDevice::deleteClient(pClient);
        Serial.println("Failed to connect, deleted client");
        return false;
    }
    Serial.println("hello2");

    if (!pClient->isConnected())
    {
        if (!pClient->connect(advDevice))
        {
            Serial.println("Failed to connect");
            return false;
        }
    }

    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());

    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;
    NimBLERemoteDescriptor *pDsc = nullptr;

    pSvc = pClient->getService(HID_SERVICE);
    if (pSvc)
    {
        std::vector<NimBLERemoteCharacteristic *> *charvector;
        charvector = pSvc->getCharacteristics(true);
        for (auto &it : *charvector)
        {
            if (it->getUUID() == NimBLEUUID(HID_REPORT_DATA))
            {
                if (it->canNotify())
                {
                    if (!it->subscribe(true, notifyCB))
                    {
                        Serial.println("subscribe notification failed");
                        pClient->disconnect();
                        return false;
                    }
                }
            }
        }
    }
    Serial.println("Done with this device!");

    if (client_count <= NIMBLE_MAX_CONNECTIONS)
    {
        NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    }

    return true;
}

void setup()
{
    Serial.begin(115200);

    NimBLEDevice::init("");
    NimBLEDevice::setSecurityAuth(true, true, true);

    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    pScan->start(scanTime, scanEndedCB);

    Serial.println("ready");
}

void loop()
{
    if (!doConnect)
        return;
    doConnect = false;

    if (connectToServer())
    {
        Serial.println("Success! we should now be getting notifications!");
    }
    else
    {
        Serial.println("Failed to connect, starting scan");
        NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    }
}