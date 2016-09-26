# strelka
Conquering Space.

[![Belka and Strelka](http://sovieteramuseum.com/wp-content/uploads/2012/08/belka-strelka-2.jpg)](https://www.youtube.com/watch?v=u4SUH9qITxE "Белка и Стрелка")

A camera sensor raw image processing pipeline
=============================================

A C++/OpenCL image processing pipeline including:
- color depth adjuctment
- black balance
- histogram equalization
- temporal denoising
- demosaic
- color correction
- point filter
- gamma encoding
- yvu conversion

Code checked against GNU C++ compiler on desktop and Android toolchain. Test harnes supports only RAW input and BMP output.

OpenCL on Android
-----------------

OpenCL kernels are in the `kernels` folder. Qualcomm OpenCl library is in: `libs`.

- [] Set `ANDROID_NDK` to the NDK location on your computer: `export ANDROID_NDK=~/<project>/android-ndk-r9`
- [] Set `ANDROID_HOME` to the SDK location: `export ANDROID_HOME=~/<project>/adt-bundle-linux-x86_64-20130917/sdk`
- [] Change `AndroidManifest.xml` to target the SDK you have, such as: `android:targetSdkVersion="18"`
- [] Change `project.properties`  accordingly: `target=android-18`

Do not use `android update project --name Rccb --path . --target 1` because it kills some existing targets of the project in `build.xml`.

* To clean: `ndk-build clean` or `ant clean`
* To build the JNI: `ndk-build` or use `ant jni`.
* To install the native part of the application:`make adb-install`
* To do everythig at once: `ant jni debug install run`
* To run the OpenCL code on the platform and get the image back:`make adb-run`
