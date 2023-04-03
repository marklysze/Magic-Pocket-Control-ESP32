# Magic Pocket Control for the LilyGO T-Display-S3
Blackmagic camera control on a Lilygo T-Display-S3

# Configuring PlatformIO
You will need to do the following to ensure that the TFT_eSPI is configured for the LilyGO T-Display-S3:
- Ensure you have uncommented the T-Display-S3 line (Around line 133) in the User_Setup_Select.h file (pio\build\libdeps\lilygo-t-display-s3\User_Setup_Select.h) for TFT_eSPI and also commented out the default line (Around line 30). In PlatformIO this is under libdeps\lilygo-t-display-s3\TFT_eSPI\TFT_eSPI.h

# FAQ
* I'm not getting any display output: Ensure you have uncommented the T-Display-S3 line (Around line 133) in the User_Setup_Select.h file for TFT_eSPI and also commented out the default line (Around line 30). In PlatformIO this is under libdeps\lilygo-t-display-s3\TFT_eSPI\TFT_eSPI.h

# Disclaimer
* The use of this software is at your own risk. The creator of this software cannot be held liable.
* Using this software therefore is completely at your own risk.