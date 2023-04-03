# Magic Pocket Control for the LILYGO T-Display-S3
Blackmagic Design camera control on a LILYGO T-Display-S3 (ESP32)

Designed for the Pocket 4K, Pocket 6K/G2/Pro, USRA G2 4.6K, URSA 12K

# Why?
My primary objective was to convert a Windows-based application I had for controlling my cameras to a small portable device.

I'm opening up to the community who may find this useful or would like to contribute to it.

# What can it do?
The key features are:
- Connect to the cameras wirelessly over Bluetooth LE
- Select the camera to connect to if you have multiple available
- Entry of the Bluetooth connection code (6 digit pin) through a touchscreen numpad
- Multiple screens that can be viewed by swiping left and right
- Screens and functionality
    - Dashboard: View the ISO, Shutter Speed, White Balance/Tint, Codec, FPS, active media device, and resolution
    - Recording: Start and Stop recording, shows timecode and the remaining time left on the active media
    - ISO: Change ISO

# Why the LILYGO T-Display-S3 Touch version?
I found this device had all the necessary components needed to connect and interact with the cameras:
- Connect over Bluetooth LE
- Touchscreen to enter the connection code (which is a challenge with screen-less touch-less devices)
- Touchscreen to support gestures (swiping left and right to change screens)
- Nice bright and colourful 320 x 170 display
- Powered over USB-C or a battery
- Cheap! ~$22USD (or $40AUD for us Aussies)

# How do I run this?
You will need the following:
- A LILYGO T-Display-S3 **Touch version** [Buy here, ~$22USD](https://www.lilygo.cc/products/t-display-s3?variant=42589373268149)
    - *Make sure to get the Touch version!*
- Visual Studio Code (VS Code) [Free, download here](https://code.visualstudio.com/download)
- The PlatformIO extension within VS Code (see below on how to configure PlatformIO for this device)
- A Blackmagic Design camera (Pocket 4K/6K/6K G2/6K Pro, URSA 4.6K G2, URSA 12K)

# Configuring PlatformIO
You will need to do the following to ensure that the TFT_eSPI library is configured for the LilyGO T-Display-S3:
- Ensure you have uncommented the T-Display-S3 line (Around line 133) in the User_Setup_Select.h file (pio\build\libdeps\lilygo-t-display-s3\User_Setup_Select.h) for TFT_eSPI and also commented out the default line (Around line 30).

# What can we do with these cameras?
See the Blackmagic Camera Control Developer Information document, [Download here](https://documents.blackmagicdesign.com/DeveloperManuals/BlackmagicCameraControl.pdf). It's not up-to-date and I've used the sample code, noted below, to update functionality. There remains other functionality not documented or in the code samples that has been worked out by the community.

I converted a lot of the Swift (Mac) code from Blackmagic Design's Cameras Code Samples, [Download here](https://www.blackmagicdesign.com/au/developer/product/camera)

# Thank you to...
- BlueMagic32: A great project to control cameras with an ESP32. It provided a reference for the Bluetooth connection functionality. [https://github.com/schoolpost/BlueMagic32](https://github.com/schoolpost/BlueMagic32)
- LILYGO: For creating this product and having some code examples for it. [https://github.com/Xinyuan-LilyGO/T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3)
- Volos Projects on YouTube: For having an entertaining set of videos on programming for the T-Display-S3 [T-Display-S3 Touch example](https://www.youtube.com/watch?v=qwRpdarrsQA)
- ChatGPT: The ability to convert code from Swift to C++ and to assist with C++ coding has saved days of programming

# FAQ
1. I'm not seeing anything on the screen?
    - Ensure you have uncommented the T-Display-S3 line (Around line 133) in the User_Setup_Select.h file for TFT_eSPI and also commented out the default line (Around line 30). In PlatformIO this is under libdeps\lilygo-t-display-s3\TFT_eSPI\TFT_eSPI.h
2. It's not working on my camera?
    - Ensure that you have Bluetooth turned on in the Setup menu on the camera
    - I have tested with the Pocket 4K and 6K (Original) - contact me if you are having trouble connecting to your camera.
    - The version of your firmware may need to be updated to the latest version so the protocol used is compatible with your camera.

# Disclaimer
* The use of this software is at your own risk. The creator of this software cannot be held liable.
* Using this software therefore is completely at your own risk.

# License
* GPL-3.0 license
* Coverted Swift code from the Blackmagic Design Cameras Code Samples [Download here](https://www.blackmagicdesign.com/au/developer/product/camera)