#include "bleM3Remote.h"

void setup()
{
    Serial.begin(115200);
    initRemoteClient();
    delay(100);
    Serial.println("ready to go");
}

void loop()
{
    remoteTick();
    delay(10);
}