#ifndef BMDCONTROLSYSTEM_H
#define BMDCONTROLSYSTEM_H

#include <memory>
#include <stdio.h>
#include "Camera/BMDCamera.h"

class BMDControlSystem {
public:
    static std::shared_ptr<BMDControlSystem> getInstance() {
        if (!instance) {
            
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
    }

    void deactivateCamera()
    {
        if (camera) {
            camera->setAsDisconnected();
            camera.reset();
        }
    }

    std::shared_ptr<BMDCamera> getCamera() {
        return camera;
    }

    bool hasCamera() const
    {
        return static_cast<bool>(camera);
    }

private:
    static std::unique_ptr<BMDControlSystem> createInstance() {
        return std::unique_ptr<BMDControlSystem>(new BMDControlSystem);
    }

    BMDControlSystem() {}

    static std::shared_ptr<BMDControlSystem> instance;

    std::shared_ptr<BMDCamera> camera;
};

#endif