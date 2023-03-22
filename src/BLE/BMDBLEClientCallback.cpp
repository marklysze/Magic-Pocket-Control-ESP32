#include "BMDBLEClientCallback.h"
#include <Arduino.h>

void BMDBLEClientCallback::onConnect(BLEClient* pclient)
{
    connected = true;
}

void BMDBLEClientCallback::onDisconnect(BLEClient* pclient)
{
    connected = false;

    if(cameraConnection != nullptr)
    {
        cameraConnection->disconnect();
    }
}