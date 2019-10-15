![logo](logo.png "Logo")
# NFC-Doorz: NFC Mifare DESFire EV1 authenticated access control system

This repo is not the current version of the RFID system. The version under development can be found in https://github.com/jschrempp/MN_ACL  Once released we will move that master repo here.



Inspired by https://github.com/rambo/nfc_lock, written in modern C++ with wrappers around the libnfc and libfreefare APIs.


## Build Environment

Builds are currently successful on Ubuntu 18.10 with the following packages inatalled:
```bash
apt install build-essential cmake libdocopt-dev libuv1-dev zlib1g-dev libusb-dev libssl-dev liblua5.3-dev
```
You will also need to install from source:
- flatbuffers-1.10.0
- libfreefare
- libnfc
- seasocks
- yaml-cpp-0.6.0
- uncrustify
