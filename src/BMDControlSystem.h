#ifndef BMDCONTROLSYSTEM_H
#define BMDCONTROLSYSTEM_H

#include <memory>
#include <stdio.h>
#include "Camera\BMDCamera.h"

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

    void setCamera(BMDCamera* newCamera)
    {
        camera.reset(newCamera);

        Serial.println("BMDCamera Set");
    }

    void deleteCamera()
    {
        camera.reset();

        Serial.println("BMDCamera Deleted");
    }

    BMDCamera* getCamera() const {
        return camera.get();
    }

    bool hasCamera() const
    {
        return static_cast<bool>(camera);
    }

private:
    // Helper function to create a new BMDControlSystem instance
    static std::unique_ptr<BMDControlSystem> createInstance() {
        return std::unique_ptr<BMDControlSystem>(new BMDControlSystem);
    }

    // Mark the constructor as private
    BMDControlSystem() {}

    static std::shared_ptr<BMDControlSystem> instance;

    std::shared_ptr<BMDCamera> camera;
};

// std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr;

#endif