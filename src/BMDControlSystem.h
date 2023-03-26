#ifndef BMDCONTROLSYSTEM_H
#define BMDCONTROLSYSTEM_H

#include <memory>
#include <stdio.h>
#include "Camera\BMDCamera.h"

class BMDControlSystem {
public:
    static std::shared_ptr<BMDControlSystem> getInstance() {
        if (!instance) {
            // Serial.print("[BMDControlSystem getInstance not detected an instance, creating a new one]");
            
            // ChatGPT wizardry for getting around accessing the private constructor in here.
            instance = std::shared_ptr<BMDControlSystem>(
                createInstance().release(),
                [](BMDControlSystem* ptr) { delete ptr; }
            );
        }
        
        return instance;
    }

    void activateCamera()
    {
        if(camera)
            camera.reset();

        camera = std::make_shared<BMDCamera>();
        camera->setAsConnected();
        // activeCamera = true;

        // Serial.println("BMDCamera created");
    }

    void deactivateCamera()
    {
        if (camera) {
            camera->setAsDisconnected();
            // activeCamera = false;
            camera.reset();
            // Serial.println("BMDCamera set as disconnected");
        }
        // else
            // Serial.println("[deactivateCamera, no camera instance]");
    }

    std::shared_ptr<BMDCamera> getCamera() {
        return camera;
    }

    bool hasCamera() const
    {
        return static_cast<bool>(camera);
        // return activeCamera;
    }

private:
    static std::unique_ptr<BMDControlSystem> createInstance() {
        return std::unique_ptr<BMDControlSystem>(new BMDControlSystem);
    }

    BMDControlSystem() {}

    static std::shared_ptr<BMDControlSystem> instance;

    // bool activeCamera = false;
    std::shared_ptr<BMDCamera> camera;
};

// std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr;

#endif