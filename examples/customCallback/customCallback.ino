#include "bleM3Remote.h"

void handleButtonPress(int button)
{
    switch (button)
    {
    case 0:
        Serial.println("up pressed");
        break;
    case 1:
        Serial.println("down pressed");
        break;
    case 2:
        Serial.println("left pressed");
        break;
    case 3:
        Serial.println("right pressed");
        break;
    case 4:
        Serial.println("center pressed");
        break;
    case 5:
        Serial.println("camera pressed");
        break;
    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    initRemoteClient();
    handleButtonPressPtr = handleButtonPress;
    delay(100);
    Serial.println("ready to go");
}

void loop()
{
    remoteTick();
    delay(10);
}