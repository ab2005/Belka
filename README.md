# Belka and Strelka
[![Belka and Strelka](http://sovieteramuseum.com/wp-content/uploads/2012/08/belka-strelka-2.jpg)](https://www.youtube.com/watch?v=u4SUH9qITxE "Белка и Стрелка")

They were instrumental in the future of human space fligh

A camera sensor raw image processing pipeline
=============================================

This project is a C++/OpenCL implementation of a complete image processing pipeline including:
- color depth adjuctment
- black balance
- histogram equalization
- temporal denoising
- [debayering / demosaicing](https://en.wikipedia.org/wiki/Demosaicing)
- color correction
- point filter
- gamma encoding
- yvu conversion

Code checked against GNU C++ compiler on desktop and Android toolchain. Test harnes supports only RAW input and BMP output.
OpenCL kernels are in the `kernels` folder. Qualcomm OpenCl library is in filder: `libs`.

OpenCL on Android
-----------------
 - [ ] Set `ANDROID_NDK` to the NDK location on your computer: `export ANDROID_NDK=~/<project>/android-ndk-r9`
 - [ ] Set `ANDROID_HOME` to the SDK location: `export ANDROID_HOME=~/<project>/adt-bundle-linux-x86_64-20130917/sdk`
 - [ ] Change `AndroidManifest.xml` to target the SDK you have, such as: `android:targetSdkVersion="18"`
 - [ ] Change `project.properties`  accordingly: `target=android-18`
 - [ ] *Do not* use `android update project --name Rccb --path . --target 1` because it kills some existing targets of the project in `build.xml`.

Buiding and deploying
---------------------
* To clean: `ndk-build clean` or `ant clean`
* To build the JNI: `ndk-build` or use `ant jni`.
* To install the native part of the application:`make adb-install`
* To do everythig at once: `ant jni debug install run`
* To run the OpenCL code on the platform and get the image back:`make adb-run`

Benchmarks
-----------
```
MacbookPro : 1080p 12bits : 5.3ms / frame
           : 4K    12bits : 24.4ms / frame 
```      

Links
-----
Chewbacca

[![Chewbacca](https://habrastorage.org/files/02c/84c/41a/02c84c41a7c945438085ecbdc2945aa9.jpg "В фильме «Марс» с космонавтами на планету высаживается и отважный пес, в честь которого свое имя получил Чубака – это транскрипция русского слова «собака». ")](https://youtu.be/-4hssVGcoLs?t=44m17s)

R2-D2

[![Android](https://habrastorage.org/files/985/14e/3b4/98514e3b43ec48249faf6d50ab6c5aec.jpg " The Android Grandfather")](https://youtu.be/9gTs1OL1jrQ?t=30m47s)
