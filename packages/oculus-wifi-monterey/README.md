# Guarded Monterey Wi-Fi package

Build with pmbootstrap for aarch64. Installs native read-only RMTFS, pd-mapper,
TFTP, and the startup helper. Owner firmware and stock Android cnss-daemon are
not redistributed in this package. The owner firmware directory must contain the
validated matching modem/MBA/WLAN files plus SHA256SUMS, installed privately at
/usr/share/oculus-wifi/firmware. Stock runtime and IPC policy helpers must already
be installed by device-oculus-monterey (revision35 sources or equivalent).

The canonical runtime files are runtime/oculus-wifi{,.initd}; the package carries
copies for standalone pmaports builds. Synchronize them and source checksums when
changing the runtime. Patches are mirrored from patches/rmtfs, pd-mapper, tqftpserv.

Start only in a fresh guarded boot (before uptime150). Existing hardware watchdog
and 300s recovery process must be present. The service retains its modem vote until
reboot; restarting/stopping it in place is deliberately unsupported. It does not
alter the recovery watchdog or enable a desktop. Enable the init service only
after a manual packaged boot passes. Network association needs a user profile.

All modem backing writes remain RAM shadows (-r is enforced in the binary).
The stock CNSS helper runs with stock code mounted read-only, /data in RAM, no
block devices, and only null/random/urandom character devices. RMTFS shared-buffer
relative offsets are explicitly enabled for Monterey. A local MAC address is
saved in /etc/oculus-wifi/mac-address; existing values are preserved.

Revision0.1-r2 is installed and verified on the owner's headset: fresh-boot manual
service start and subsequent OpenRC default startup both associate and obtain DHCP.
Router/internet pings, DNS, and SSH over Wi-Fi pass. The private0600 profile is at
/etc/wpa_supplicant/wpa_supplicant-wlan0.conf. No credentials belong in this package.
The service is enabled in default on this device; the 300s recovery timer remains.
