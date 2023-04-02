# Magic Pocket Control for the LilyGO T-Display-S3
Blackmagic camera control on a Lilygo T-Display-S3




FAQ:

Q: I'm not getting any display output
A: Ensure you have uncommented the T-Display-S3 line (Around line 133) in the User_Setup_Select.h file for TFT_eSPI and also commented out the default line (Around line 30). In PlatformIO this is under libdeps\lilygo-t-display-s3\TFT_eSPI\TFT_eSPI.h