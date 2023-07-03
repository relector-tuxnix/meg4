MEG-4 Android Port
==================

This directory contains the main executable for the Android port, using the [SDL3](../sdl) backend.

*** EXPERIMENTAL ***

NOTE: I'm looking for a contributor who would help me with this port. I've set up everything as SDL/docs/README.android
said, but I couldn't get ndk-build working, so I couldn't test it and I don't know if this gradle setup is okay or not.
Should be to my best knowledge.

Compilation options
-------------------

| Command               | Description                                                |
|-----------------------|------------------------------------------------------------|
| `make`                | Compile the APK (into `app/build/outputs/apk/arm8`)        |
| `make installRelease` | Compile and install release version                        |
| `make installDebug`   | Compile and install debug version                          |

Requirements
------------

Android SDK (version 31 or later) https://developer.android.com/sdk/index.html

Android NDK r15c or later https://developer.android.com/tools/sdk/ndk/index.html

Minimum API level supported by SDL: 16 (Android 4.1)
