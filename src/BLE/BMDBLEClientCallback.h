#ifndef BMDBLECLIENTCALLBACK_H
#define BMDBLECLIENTCALLBACK_H

#include <BLEClient.h>
#include <Camera/BMDCameraConnection.h>

class BMDCameraConnection; // forward declaration as both header files include each other.

// This class is for notifications on the BLEClient object in terms of connects and disconnects
class BMDBLEClientCallback : public BLEClientCallbacks
{
public:
    BMDBLEClientCallback(BMDCameraConnection *theCameraConnection) : cameraConnection(theCameraConnection) {}
    virtual void onConnect(BLEClient* pclient);
    virtual void onDisconnect(BLEClient* pclient);

private:
    bool connected = false;
    BMDCameraConnection* cameraConnection;
};

#endif