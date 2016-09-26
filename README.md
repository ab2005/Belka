# strelka
Conquering Space.

[![стрелка](http://sovieteramuseum.com/wp-content/uploads/2012/08/belka-strelka-2.jpg")](https://www.youtube.com/watch?v=u4SUH9qITxE
)

https://www.youtube.com/watch?v=u4SUH9qITxE
Raw image processing pipeline
=============================

Reference pipeline refactored:
* code checked against C++ compiler on desktop and Android
* converted C objects like images to C++
* code cleanup

Only supports RAW input and BMP output.

OpenCL on Android
-----------------

OpenCL kernels are located in the `kernels` folder.

Make `ANDROID_NDK` to point to where the NDK is on your computer, for example:

`export ANDROID_NDK=~/<project>/android-ndk-r9`

and `ANDROID_HOME` to point to where the SDK is, for example with:

`export ANDROID_HOME=~/<project>/adt-bundle-linux-x86_64-20130917/sdk`

Change `AndroidManifest.xml` to target the SDK you have, such as:

`android:targetSdkVersion="18"`

and adapt `project.properties` for example to:

`target=android-18`

You *cannot* use this

`android update project --name Rccb --path . --target 1`

because it kills some existing targets of the project inside `build.xml`.

You can clean the current compilation with

`ndk-build clean`

or

`ant clean`

To build the native part you can use

`ndk-build`

or use `ant jni`, as stated later

Install the native part of the application:

`make adb-install`

For compiling the Android app, installing the files needed to run the
native application and run the app do:

`ant jni debug install run`

To run the OpenCL code on the platform and get the image back:

`make adb-run`
