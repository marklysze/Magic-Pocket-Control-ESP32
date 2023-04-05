# Magic Pocket Control for ESP32 devices
## main files

Here is where you'll do most of your application programming. PlatformIO will use the main-***environment***.cpp file that relates to the environment you have selected.

So, if you have selected the "env:m5stick-c" environment, PlatformIO will build and deploy using the main-*m5stick-c*.cpp file.

Use the other *main* files as references as to what can be done

Side note: How does PlatformIO know to use the relevant main file? In the platformio.ini file it includes this when building which injects the environment name when including and will ignore the rest of the files under the main folder:
```
+<main/main-${PIOENV}.cpp>
```