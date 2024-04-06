#ifndef BLEM3REMOTE_H
#define BLEM3REMOTE_H
#include <NimBLEDevice.h>

#define HID_SERVICE "1812"
#define HID_REPORT_DATA "2A4D"

#define MAX_REMOTES 3
#define CLIENT_POWER ESP_PWR_LVL_P3

static uint32_t scanTime = 0;

std::vector<NimBLEAddress> macWhitelist = {};

static std::vector<uint8_t> buttonValue = {0, 0, 0, 0, 0, 0, 0, 0, 0};
static const std::vector<uint8_t> buttonUpKey = {60, 0, 15, 1, 0, 0, 2, 48, 0};         // 0
static const std::vector<uint8_t> buttonDownKey = {60, 64, 236, 1, 0, 0, 2, 208, 255};  // 1
static const std::vector<uint8_t> buttonLeftKey = {40, 128, 17, 1, 0, 0, 150, 0, 0};    // 2
static const std::vector<uint8_t> buttonRightKey = {174, 143, 17, 1, 0, 0, 106, 15, 0}; // 3
static const std::vector<uint8_t> buttonCenterKey = {160, 48, 232, 1, 0, 0, 0, 0, 0};   // 4
static const std::vector<uint8_t> buttonCameraKey = {0, 0, 0, 1, 0, 0, 0, 0, 0};        // 5

static int bleServers = 0;
static bool doConnect = false;
static NimBLEAdvertisedDevice *advertisedDevice;
typedef void (*HandleButtonPress)(int button);
void scanEndedCB(NimBLEScanResults results);

bool checkMac(const NimBLEAddress &dev)
{
    if (macWhitelist.size() == 0)
    {
        return true;
    }
    for (auto &it : macWhitelist)
    {
        if (dev.equals(it))
        {
            return true;
        }
    }
    return false;
}

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

class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient)
    {
        bleServers++;
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" connected");
        pClient->updateConnParams(120, 120, 0, 60);
        if (bleServers < MAX_REMOTES)
        {
            NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
        }
    };

    void onDisconnect(NimBLEClient *pClient)
    {
        bleServers--;
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" disconnected");
        NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    };

    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        return true;
    };
};

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{

    void onResult(NimBLEAdvertisedDevice *ad)
    {
        if (ad->haveServiceUUID() && ad->isAdvertisingService(NimBLEUUID(HID_SERVICE)) && checkMac(ad->getAddress()))
        {
            if (bleServers > MAX_REMOTES)
            {
                Serial.println("no more connections");
                NimBLEDevice::getScan()->stop();
            }
            advertisedDevice = ad;
            doConnect = true;
        }
    };
};

void handleButtonPressDefault(int button)
{
    switch (button)
    {
    case 0:
        Serial.println("button up pressed");
        break;
    case 1:
        Serial.println("button down pressed");
        break;
    case 2:
        Serial.println("button left pressed");
        break;
    case 3:
        Serial.println("button right pressed");
        break;
    case 4:
        Serial.println("button center pressed");
        break;
    case 5:
        Serial.println("button camera pressed");
        break;
    default:
        break;
    }
}

HandleButtonPress handleButtonPressPtr = handleButtonPressDefault;

void buttonPressCallback(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (length == 3)
    {
        buttonValue[0] = buttonValue[3];
        buttonValue[1] = buttonValue[4];
        buttonValue[2] = buttonValue[5];
        buttonValue[3] = buttonValue[6];
        buttonValue[4] = buttonValue[7];
        buttonValue[5] = buttonValue[8];
        buttonValue[6] = pData[0];
        buttonValue[7] = pData[1];
        buttonValue[8] = pData[2];

        if (keyMatch(buttonValue, buttonUpKey))
        {
            handleButtonPressPtr(0);
        }
        else if (keyMatch(buttonValue, buttonDownKey))
        {
            handleButtonPressPtr(1);
        }
        else if (keyMatch(buttonValue, buttonLeftKey))
        {
            handleButtonPressPtr(2);
        }
        else if (keyMatch(buttonValue, buttonRightKey))
        {
            handleButtonPressPtr(3);
        }
        else if (keyMatch(buttonValue, buttonCenterKey))
        {
            handleButtonPressPtr(4);
        }
        else if (keyMatch(buttonValue, buttonCameraKey))
        {
            handleButtonPressPtr(5);
        }
    }
}

void scanEndedCB(NimBLEScanResults results)
{
}

static ClientCallbacks clientCB;

bool connectToServer()
{
    int startTime = millis();
    NimBLEClient *pClient = nullptr;
    if (NimBLEDevice::getClientListSize())
    {
        pClient = NimBLEDevice::getClientByPeerAddress(advertisedDevice->getAddress());
        if (pClient)
        {
            pClient->setConnectionParams(6, 6, 0, 200);
            if (!pClient->connect(advertisedDevice, false))
            {
                Serial.println("reconnect failed");
                return false;
            }
            Serial.println("reconnected client");
        }

        else
        {
            pClient = NimBLEDevice::getDisconnectedClient();
            // not sure what this is doing
        }
    }

    if (!pClient)
    {
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(&clientCB, false);
        pClient->setConnectionParams(12, 12, 0, 51);
        pClient->setConnectTimeout(5);

        if (!pClient->connect(advertisedDevice))
        {
            NimBLEDevice::deleteClient(pClient);
            Serial.println("failed to connect, deleted client");
            return false;
        }
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(advertisedDevice))
        {
            Serial.println("failed to connect");
            return false;
        }
    }

    NimBLERemoteService *pSvc = nullptr;
    pSvc = pClient->getService(HID_SERVICE);
    if (pSvc)
    {
        std::vector<NimBLERemoteCharacteristic *> *charvector;
        charvector = pSvc->getCharacteristics(true);
        for (auto &it : *charvector)
        {
            int handle = it->getHandle();
            if (handle == 23 || handle == 31)
            {
                if (!it->subscribe(true, buttonPressCallback))
                {
                    Serial.println("subscribe notification failed");
                    pClient->disconnect();
                    return false;
                }
            }
            else if (handle == 35)
            {
                if (!it->subscribe(false))
                {
                    Serial.println("subscribe notification failed");
                    pClient->disconnect();
                    return false;
                }
            }
        }
    }

    char timeString[4];
    float seconds = (millis() - startTime) / 1000.0;
    sprintf(timeString, "%3.2f", seconds);
    Serial.print("completed connection after ");
    Serial.println(timeString);
    return true;
}

void initRemoteClient(bool initDevice = true)
{
    if (initDevice)
    {
        NimBLEDevice::init("");
        NimBLEDevice::setPower(CLIENT_POWER);
        NimBLEDevice::setSecurityAuth(true, true, true);
    }

    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    pScan->start(scanTime, scanEndedCB);
}

void remoteTick()
{
    if (doConnect)
    {
        doConnect = false;
        if (!connectToServer())
        {
            Serial.println("failed to connect, starting scan");
        }
    }
}
#endif