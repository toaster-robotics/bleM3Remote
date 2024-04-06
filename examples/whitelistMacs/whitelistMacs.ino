#include "bleM3Remote.h"

void setup()
{
    Serial.begin(115200);
    initRemoteClient();

    macWhitelist.push_back(NimBLEAddress("2A:07:98:03:BA:19"));

    delay(100);
    Serial.println("ready to go");
}

void loop()
{
    remoteTick();
    delay(10);
}