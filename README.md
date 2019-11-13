# Native C++ SDK for XtraLife
## Downloading and installing

Download the desired version (either in .7z form, requiring 7-zip to decompress, or in .zip form) from the [releases page](https://github.com/xtralifecloud/cloudbuilder/releases) below.

Then, start with [the documentation](http://xtralifecloud.github.io/cloudbuilder/Docs/DoxygenGenerated/html/index.html) in order to start integrating the SDK in your own application.

This SDK works with XtraLife. For more information and configuring your application, visit https://xtralife.cloud/. Note that we also provide a [Unity SDK](https://github.com/xtralifecloud/unity-sdk), which is much easier to get started with.

## Compiling

Compiling can be time consuming and if you simply want to use the library, we encourage you to start by downloading a precompiled version of the SDK from the releases page.

In order to compile the SDK, you need to populate the [repositories](file://repositories/) directory with [curl](https://curl.haxx.se/) binaries and include, and [OpenSSL](https://www.openssl.org/) binaries.\
Compiling these two projects for iOS, Android, Windows and macOS can be a very complex and time consuming process. To speed it up, we have built an optimized and ready to use repository based on the latest curl(7.67.0, November 2019) and OpenSSL(1.1.1d, September 2019), that you can find [here](https://clanofthecloud.s3.amazonaws.com/repositories/repositories-cpp-sdk.zip).\
All platforms are provided with 64 bits binaries, except Android which also provides 32 bits binaries. This will not be needed though in a near future.\
FYI, curl is used to run https queries between client and XtraLife servers, and OpenSSL is in charge of the "s" part of https.\
The repository will be populated with a new xtralife-sdk folder once you have successfully built the SDK.

There is also a very minimalist application for all platforms (in console mode for desktop versions) which just connects to XtraLife servers and performs an anonymous account to show the asynchronous nature of calling XtraLife services through this C++ SDK.