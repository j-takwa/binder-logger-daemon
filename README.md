Android Binder Logging Daemon

Tested on: Android 15 (AOSP) — Cuttlefish virtual device
Repository location (in AOSP): external/binder-logger/
Module type: native cc binary (Soong cc_binary)
AOSP integration: added to device/google/cuttlefish/aosp_cf.mk via PRODUCT_PACKAGES += binder_logger to include the binary in the system image.
Overview

This project provides a small native C daemon that parses structured Binder-related log lines emitted by an instrumented ServiceManager.java and prints a human readable inter-service call trace. It was developed during an internship exploring Android system IPC (Binder), AOSP build and Cuttlefish virtualization.

Primary goals:

Collect Binder transaction log lines (from logcat),

Parse the fields (timestamp, caller PID/UID, service name, interface, method, binder instance/object, duration, success flag, etc.),

Present an easy-to-read trace showing client → binder → target service relationships,

Run automatically inside a Cuttlefish device during boot for continuous visibility.
Example output:

==================== BINDER CALL 1 ====================
Timestamp : 2025-07-05 10:12:43

+---------------------------------------+
| CLIENT (caller)                       |
| uid     : 1000
| pid     : 287
| instance: BpBinder@0x123abc
| status  : success
+---------------------------------------+
               |
               v
+---------------------------------------+
| BINDER INFO                           |
| binder id : BpBinder@0x456def
| method    : addToDisplay
| duration  : 125 µs
+---------------------------------------+
               |
               v
+---------------------------------------+
| SERVICE (target)                      |
| name     : com.android.systemui
| interface: IWindowSession
| source   : ServiceManager
| cached   : false
+---------------------------------------+

Result: ✅ Request served successfully
------------------------------------------------------------

Integration into AOSP (Android 15) — step-by-step

Notes: Android 15 uses Soong (Android.bp) as the preferred build description; legacy Android.mk files are still supported on many trees but migrating to Android.bp is recommended. Below are both Soong and Android.mk examples — use the format that matches your tree or team preference.

1) Place the code in external/

Create the directory inside the AOSP tree:

$AOSP_ROOT/external/binder-logger/


Copy your source files there.

2) Add a Soong Android.bp

Save this file as external/binder-logger/Android.bp:

cc_binary {
    name: "binder_logger",
    srcs: [
        "binder_logger.c",
        
    ],
    cflags: ["-Wall", "-Wextra", "-O2"],
    shared_libs: ["libc","libm"],
    vendor: true, 
}
3) Add the binary to the Cuttlefish product image

Open device/google/cuttlefish/aosp_cf.mk (or the relevant product makefile for your Cuttlefish product) and add:

# Include binder_logger in the system image
PRODUCT_PACKAGES += \
    binder_logger


This will make the built binder_logger binary part of the system image (typically installed to /system/bin/ but since we mentioned vendor:true in the Android.bp it will be part of the vendor image /vendor/bin/).
5) Build the image (Cuttlefish)

From your AOSP root:

source build/envsetup.sh
# choose the cuttlefish build target (use the proper target for your build):
lunch <cuttlefish_product>        # e.g., aosp_cf_x86_64-userdebug  (replace with your product)
# Build just the binary
m binder_logger
# then/or build the full system image
m -j$(nproc)
6) Deploy / test on a running Cuttlefish instance
adb root
adb shell
binder-logger

Notes:
We have continuous real-time parsing, since we implemented a mode where the daemon reads from adb logcat stream.

Running at boot as root may be necessary to access system logs and certain files; ensure correct permissions and security policy if targeting production images.
