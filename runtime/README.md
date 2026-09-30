# Native VR session (under validation)

This launcher starts the real Monado framebuffer compositor with runtime stock optics, then an OpenXR application. It does not start Xorg or labwc and never changes the recovery watchdog.

The default oculus-vr-home application is not implemented yet. **Do not enable the OpenRC service until that executable exists and the complete session has been tested.** During integration, explicitly pass a verified OpenXR sample executable and arguments instead. A fresh MONTEREY_START_FILE readiness gate allows calibration at rest before the wearer puts on the headset.

Runtime requirements: Monado r3 or newer package (r3 build pending validation), the headset's own extracted stock mesh at /var/lib/monado/monterey/distortion-mesh.bin, framebuffer and IMU device access, Vulkan runtime. Provision the stock mesh locally; it is not redistributed here. The session currently uses software Vulkan until an accelerated compatible path is verified.
