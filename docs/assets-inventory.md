# Private asset locations and SHA256

Inventory measured in place on 2026-10-01. These assets are **not in Git** and cannot
all be regenerated from this repository. No key material, Wi-Fi PSK, firmware or
recording bytes are included here. Preserve the private originals and backups.
Hashes are SHA256 of each file's exact bytes (not directory hashes), read on the
listed host. Historical images are inventory entries, not recommendations to flash.
The device's evolving installed filesystem is not a fresh image in this inventory.

Host access: Mac paths are local; `/Volumes/vela` must be mounted. Kali is reached
as `<user>@<BUILD_HOST_IP>`. The Mac SSH key below is an owner credential, not a firmware
asset. Full stock images can contain device identity, calibration and credentials.
Camera/IMU recordings can contain private room imagery; do not redistribute them.

Original motion captures on Kali are `vio-motion-01.bin` through
`vio-motion-04-marked.bin`; the marked capture is the existing 60-second trial.
`vio-record-01.bin` is the saved stationary input. Rectified/reordered replays and
maps are derived artifacts; their source recordings and per-unit calibration are
listed below. Per-unit `persist` backups and `xrs-hmdconfig`/distortion assets must
be retained even when derived calibration JSON is available. Firmware also remains
recoverable from the listed owner stock partition images.

This is a verified file inventory of the fragile asset classes, not an exhaustive
hash of build caches or a claim that all historical staging paths still exist.

## Build-root firmware

| Host | Exact path | SHA256 |
|---|---|---|
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/CM710X.bin` | `321b6ebce9a7ccb057fd7f79c16dcb3c53c9dd456b2d29db3b340c66154f55c8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a530_pfp.fw` | `072cbbb67eeb67689fd3ba2c11708e0778ae873b42d79cac030821e12fbcd126` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a530_pm4.fw` | `a02c9be11787472d4ab0173ed8552afbde1f31b1e81bcd87adf1fcea97b6115f` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a540_gpmu.fw2` | `57d7653c4dcc1130ac2c31512ec7fbb6348613d4bcd99b9b6221e3f9ee52f535` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a540_zap.b00` | `5302ecf8978c825bdc9d8455a828a4ac1e1d762cf09ff821f9f506fb89a549e5` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a540_zap.b01` | `c6b00518bfaab60b067695eaeba55bdc75317a85593c0d8dbbf00c16cbebc34b` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a540_zap.b02` | `997933766e93f7692329f397808f8c23128a58b565d6305e48608cfce711e5d7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/a540_zap.mdt` | `13364b1bde6a0ef8a16b8faf2c11fe0f718c97317ace3296628dc8c9f3d7c085` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b00` | `66ab42b9424db47b2b3d69a97284cffa7d362b9b67bc6087f2bcd409d68fc194` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b01` | `0de08e7a2c86740b7dfc84ea772f120d1f34a9c8d59ff6c201d3e3526e37bdf2` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b02` | `671a7652567c3491c602a64ca908dd9e6139dcc96413983a2d81b0627c0490fc` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b03` | `461926e290fb463ae766c5be620213aef0a7b860ed7364daf0c6eac4e5ad982b` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b04` | `ef6b658f765020b366003d9ca0d72b48d3362805f70d9840cf393884b0995d88` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b05` | `2adba682c72140b81795a805605fff71b601f84c3160bb2af596a9b9412701df` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b06` | `11e1c4cce2e6d46792a74e5960bd3d20e76c189ab9208b2e9436c0ee0cba23c3` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b08` | `8810014631044b9ecbb876d1a2dd920ca442385b109c4a686255e7e58e3e529a` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b09` | `3cf4c0f59ab4c8b5fad896e1cffcf575c1e71812b3d703aaf27e5206b97b14f8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.b11` | `42f658666e86c721866fa5795086e36253f694f054083c0b1b5d1020966f11f0` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adsp.mdt` | `8dea6735e8549122a1925fb836bbcf1d257b1384080c15a4c1300967835dbadc` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adspr.jsn` | `4781dc9312fa017b0e611c752dae2183c0f205af8300b28417a496dae511b0a5` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/adspua.jsn` | `3da465910d314fe64d34d16ca817b7bad161ebf241c064b90d491af987ef3045` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/apbtfw10.tlv` | `07f0f45cff57347a73ada68bf2bd564e17998383388ef27bd2e9de5269c1986f` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/apnv10.bin` | `4cb35fec741a3294f5c64d41aefba45cf899c42cc4c00800bba7029cca5919d9` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.102` | `6d36276c05fb0c911c8bcf77a5c6d6f3f705fb92c4659a83b7bb5357142a5520` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.104` | `c0d50c28e452ae35c870e4a93ac4c64ade25a49b11782b29ca0b2953dc4a508f` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.105` | `b6d2508296936152e930d17a3ed58a2d3f49d063eedb3e64e79dcc57a183e33f` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.106` | `4e39e7f895e8ba61bdadb80c9b20de60d520e29bc37d309818eb7b0b2ca29b1a` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.108` | `92812b33b4f1fe5451181b4defe2bb707a3c6fceeda43623f992f08113cf03a9` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.b04` | `deb5735b9af8b243afd5cc3f9d04bbf2190eb59fe691b9691c818ac344be10f3` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.b07` | `542d9742e54841efa95ffa6b0901eb0f9fd1ad5cf700f2548f99d99c66cf5424` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.b09` | `71e691ed99bbdc6c82e5b1caaf7c8f534cb0a44d97e1418b72c6c19728f75f2a` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.b0a` | `f63da6a1f82a3f661940d904e5644db26df5508c090610d73db9078f9e5d2692` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.b0b` | `4b45dc36b18a3c25383e298cb242931b0659b40290dcfb28e632a200e2560c00` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.b33` | `38abaa0c1cbfc66de79d27dd61100779b6d0a3af2d85e6351d122175cec07a49` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/bdwlan.bin` | `ceaccf9690a04b8439f1f58dbb403b631d0d57870c00c51b966e214130170520` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/crbtfw11.tlv` | `d6532b99875890fb7969874ff6e904939ca0d6f2d2bd1e1fd3ae86470bebc9f5` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/crbtfw20.tlv` | `d9afa5d9facd528f437e83bcc8147681afd432296ebbba5e77904c13fab28209` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/crbtfw21.tlv` | `df392e7de2130925e2d010ddad2290865f7753d1454c26f685d2e19fc0784280` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/crnv11.bin` | `206deb651dcfd187d1fd39f1f064af20324d1b4e28dcd72f5ed23bb8ef605d00` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/crnv20.bin` | `6c96fe067860a79813b466a1d4644dc2748a4191b21a986a44d2cc0fe6ccaca7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/crnv21.bin` | `ac67ef214ef4e7b85296be760341b8a865609acfc2126efe1e8d641cdd8f970d` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/wlan/qca_cld/WCNSS_qcom_cfg.ini` | `81f8b29faadbd8c702dea545d3e07ee5620f91776e5bee4281820288edea86af` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/lib/firmware/wlanmdsp.mbn` | `cbf7f76b55ad353e630767f6ea3b564b2c54de0b87f6fa96760c3550a2a4f1f2` |

## Historical build image; not current-device snapshot

| Host | Exact path | SHA256 |
|---|---|---|
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/4k-test/part-1-4k.img` | `280460dcd31b00b3ee51c7ea91d1abe057c8449722a23d5eba144bb92518cc57` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/4k-test/part-2-4k.img` | `0da37b03fac93bbb815c175b21574206f124f74022b9840252c04c6fb64d5972` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/4k-test/pmos-system-4k.img` | `520c94cf651e637e93c73f3e6337d445e9b39660d7425d3fa8d9cd4c84f41ad0` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/desktop-trial/failed-before-resize.img` | `f7ccc1c2e51281a4667f8317197fd15895b93f23338a6bcf11685143e487910d` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/desktop-trial/pmos-system-desktop.img` | `efe1bcff08d126b783f37c97a1438570c539fdb248429b0eb385c643b29ad3b0` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/pmos-system-4k-ssh-base.img` | `ed8cc6d45b11eafb188fea1ec31df9f57077064812898aea799395641c464199` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/pmos/work/chroot_rootfs_oculus-monterey/boot/boot.img` | `f27bb7e5058c5b5d682dd2a0e50bdc788ab20699d169a77c27721f22157a12b9` |

## Owner SSH private key (hash only)

| Host | Exact path | SHA256 |
|---|---|---|
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/id_ed25519` | `d0ce0e66c71b934251a24637f93802a6944b744364f90ea034202d5aabec0e79` |

## Owner firmware

| Host | Exact path | SHA256 |
|---|---|---|
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/mba.mbn` | `58ab8df5accf95cf410b8882e112ceac28717c803aadda078427c97ef1e04c3b` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b00` | `60f86f5a8202df186efd3070760f8aafa47721e9972bf06c7a45ccfb9374ca16` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b01` | `2746e9352a71f25be94882f8e0357458d2ab7cf6e5c8d12d57547a8077503586` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b02` | `94f68f54d6618507a886c927a73c3f59434fc570c9842ea1e3dc44ef51a419bc` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b03` | `2e853daf89795c4cfcc96ef3c03b2b7bb4aaaa20b6853b5fa77dbb8acc4d9849` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b04` | `7c14dd7e4394942b55b28d7b8ebdf572011cd6f1b9071398b7cbae3bfa1b6712` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b05` | `1913e2d5f16fac5cd401e95807049f8eaaff9b3bde8b9d9eb113cb32de2e9ae8` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b06` | `a8ef95ccb0eb2aeb3821274f529ceaa519ab6dde44a4cde3b0a7912354f37acf` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b07` | `3ff57e014086d3b65a0bb12de6fe5985a71ff418c423a7ad5ed8d151cd0ccf74` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b08` | `33418c1987ca5636c41db117ae7d97495c190b8186b71d1d3dec7d2a74bf78d6` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b09` | `8236fb76f0cabd34b28065020310696fd51391d193cd2311d855539f139ea160` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b10` | `be3e683f02cab13f0ad73a852b0b4a35f0a1b996c526c6b070d6b6bc601c3a60` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.b11` | `9850bd36ba86908f93b62bfeac0e7cb2a8de068162411cd9560576b17bf6def9` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/modem.mdt` | `0df382ffb4af4437c52ec88af983e4f003b161afe076140cc72783ca49bf2ec4` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/validated-firmware/wlanmdsp.mbn` | `cbf7f76b55ad353e630767f6ea3b564b2c54de0b87f6fa96760c3550a2a4f1f2` |

## Owner runtime / pending experiment artifact

| Host | Exact path | SHA256 |
|---|---|---|
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/cache-audit-owner-libs/gralloc.msm8998.so` | `0da3b6944c2dd98a2b422f3769b56fc5fa14bab1664deccec9d0c3ca4130b0fe` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/cache-audit-owner-libs/libimagebuffer.so` | `705f66b2220207d0bc20cc40b4635cbe57048940b5c61f9b894c5acdf4ca3157` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/cache-audit-owner-libs/libion.so` | `71c81c8313450928185e5a992bf78c5fc309592205606d7477ff56af19bd8426` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/cache-audit-owner-libs/libui.so` | `33788a7bd262c904b8013ccd4eb5767dab24b7c00ecca061dea132b8a8d61141` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/cpu-invalidate.urbUNy/stock-camera-cpu-invalidate.so` | `9da5407ac8b01aced9713892b7a39b6e1b0541b29521fb9484ae72b99318f86b` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/cache-audit-owner-libs/gralloc.msm8998.so` | `0da3b6944c2dd98a2b422f3769b56fc5fa14bab1664deccec9d0c3ca4130b0fe` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/cache-audit-owner-libs/libimagebuffer.so` | `705f66b2220207d0bc20cc40b4635cbe57048940b5c61f9b894c5acdf4ca3157` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/cache-audit-owner-libs/libion.so` | `71c81c8313450928185e5a992bf78c5fc309592205606d7477ff56af19bd8426` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/cache-audit-owner-libs/libui.so` | `33788a7bd262c904b8013ccd4eb5767dab24b7c00ecca061dea132b8a8d61141` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/stock-camera-cpu-invalidate.so` | `9da5407ac8b01aced9713892b7a39b6e1b0541b29521fb9484ae72b99318f86b` |

## Partition / boot / OS image; historical variants, not flash instructions

| Host | Exact path | SHA256 |
|---|---|---|
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/owner-persist.img` | `b1124ca0e0b7792763b7bb2c04cb0c702b3bbd32f84e239c5ef0827f03ea0bc9` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/owner-private-readonly.img` | `c5d22760b386de2845f387a7e33ca25fa9a7c4da736a661564e5c229ee1b20d2` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/pmos-logs.img` | `f794712bf01f52f2b60db334ab7bed492616524d15dfe8e039cee8ee02100d67` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/diagnostic-r4-final.img` | `06abe1a5e489eb81e0a8f789442f9f9e298c51f031f33720df7b907ffea5ac6a` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/wifi-source/diagnostic-r4-raw.img` | `e7641204f03d9b8c82f5b5a92c0eab2ef4bf13b895ff544730f9123a5ae49f8d` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/abl.img` | `892d05d7802937957005411323b3ce1d567cc826f4e199cb26d559bc8fd609e2` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/boot.img` | `fa6c6e24cbd7381371e553ff67e75de54d056bf6c93107c038f4b8faf31a476a` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/cmnlib.img` | `41497e3794574bb63a159a8a57da8db0e2d4f5e937d689617f4c5dcb17668b00` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/cmnlib64.img` | `229e27a8f3223c56db3fb43568b437da29eea672dd526a749fd1189aa6f5f319` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/devcfg.img` | `ffced92704d7ffe43856af0a17d1a4b61b972595cc3d26dea277e43e5b5265f7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/hyp.img` | `52f09371d4f2d07456120a2316beb5b07b2fd3fc251bce0195089e84d7a6af78` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/keymaster.img` | `f38cad0be24fbd37b83be93e879e6ca28dcf3413c52b3dd2edbdb9310379ba41` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/modem.img` | `935d61fa51d7ddf935d50e2298da0325f2b4e8a12d54d53e7c52de0df9e24fd4` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/ovrtz.img` | `65201d930b4b6cf60837b6494a91c7325a18ce8050712ec6d439cb95feb568d6` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/pmic.img` | `1f0250d61825f19bcf06e6c08d412010c44e00f8c702d9c124901ab445b737d5` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/rpm.img` | `9d4c028c0eab10948f8098d2f3a3e9edc3c15ee161ac34b668f863eb6658b2d6` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/tz.img` | `c8abca3b4f67439054c0e18b1abf7a26e7f3aea1b5075509f2c83908e26c03f9` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035243-slot-a/xbl.img` | `428cd4c04e9e579e6860b49a5db40b759793013466c8a28f0e8ae4ed000bc223` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/abl.img` | `892d05d7802937957005411323b3ce1d567cc826f4e199cb26d559bc8fd609e2` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/boot.img` | `fa6c6e24cbd7381371e553ff67e75de54d056bf6c93107c038f4b8faf31a476a` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/cmnlib.img` | `41497e3794574bb63a159a8a57da8db0e2d4f5e937d689617f4c5dcb17668b00` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/cmnlib64.img` | `229e27a8f3223c56db3fb43568b437da29eea672dd526a749fd1189aa6f5f319` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/devcfg.img` | `ffced92704d7ffe43856af0a17d1a4b61b972595cc3d26dea277e43e5b5265f7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/hyp.img` | `52f09371d4f2d07456120a2316beb5b07b2fd3fc251bce0195089e84d7a6af78` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/keymaster.img` | `f38cad0be24fbd37b83be93e879e6ca28dcf3413c52b3dd2edbdb9310379ba41` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/modem.img` | `935d61fa51d7ddf935d50e2298da0325f2b4e8a12d54d53e7c52de0df9e24fd4` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/ovrtz.img` | `65201d930b4b6cf60837b6494a91c7325a18ce8050712ec6d439cb95feb568d6` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/pmic.img` | `1f0250d61825f19bcf06e6c08d412010c44e00f8c702d9c124901ab445b737d5` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/rpm.img` | `9d4c028c0eab10948f8098d2f3a3e9edc3c15ee161ac34b668f863eb6658b2d6` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/tz.img` | `c8abca3b4f67439054c0e18b1abf7a26e7f3aea1b5075509f2c83908e26c03f9` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>-queststack-slotA-backups/20260929-035536-slot-a/xbl.img` | `428cd4c04e9e579e6860b49a5db40b759793013466c8a28f0e8ae4ed000bc223` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/abl_b.img` | `a59f7504e50b8623a7322a35964e214fc257a7367fec375e5cd4a14886bdc20e` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/apdp.img` | `8a39d2abd3999ab73c34db2476849cddf303ce389b35826850f9a700589b4a90` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/bluetooth_b.img` | `30e14955ebf1352266dc2ff8067e68104607e750abb9d3b36582b8af909fcb58` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/boot_b-magisk-patched.img` | `c191ae0891b7b05fc9285d4614f6f8e98e0a81ec1b8be6bff0c7c2a2e56d24ad` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/boot_b.img` | `7ec81a30c8f7b597dd2678b031a136f2690db5bb3f18996b23c31544e2917c9e` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/cdt.img` | `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/cmnlib64_b.img` | `a36895b196f1718758e22bb3dbcc0d692332b7c04eb5e64b3dae6275a7518163` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/cmnlib_b.img` | `80084826ceabff74527918fc0f08e00835ea87af81ff06c6f4a97c6cfe7d7ed5` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/devcfg_b.img` | `4f60887b8752d379ac183f4bb8f0a6a8e1d22cd6df6c3beffce6928ae22b822f` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/devinfo.img` | `cdb5a17babf50790ae6d97de5f4c64a11568ffa24d16d19432a2431501d6d0ba` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/dip.img` | `30e14955ebf1352266dc2ff8067e68104607e750abb9d3b36582b8af909fcb58` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/dpo.img` | `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/frp.img` | `804065c78cfbad280c3ee67a9a63f66b57dfa160ff81ce919eea5a1a8e9d7a11` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/fsc.img` | `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/fsg.img` | `5647f05ec18958947d32874eeb788fa396a05d0bab7c1b71f112ceb7e9b31eee` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/hyp_b.img` | `e85c398ecaad66d8ae12e3bbb81d1f6457838f3363d0e40abab55100a2014986` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/keymaster_b.img` | `28ee240b99783e233fa2d0501bd7ac5bd86fc2db0e19705a3f7144215f75738d` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/keystore.img` | `07854d2fef297a06ba81685e660c332de36d5d18d546927d30daad6d7fda1541` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/limits.img` | `ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/misc.img` | `488564ef6bb15329bc2135ac4acd29deeabeec8a3e358fda71e7bd588c456584` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/modem_b.img` | `23db3a5030f83ffc9f342013f602325ca9ac66d36a2f268cf28a136866c1964e` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/modemst1.img` | `947c06398a32152ef0fb990d41084ce34f6cbc82b7f6f780454d10ee391385cd` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/modemst2.img` | `be75776b2471633b0aeab58c228f95585a1d9dc2c303f7307271f3cb515c2964` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/msadp.img` | `8a39d2abd3999ab73c34db2476849cddf303ce389b35826850f9a700589b4a90` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/ovrtz_b.img` | `b6bd3f9eb72f783da62fc9d94f6ee360e3277849e5fbd1bf352eeb21007e9c49` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/persist.img` | `b1124ca0e0b7792763b7bb2c04cb0c702b3bbd32f84e239c5ef0827f03ea0bc9` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/pmic_b.img` | `0a3874c63d921a2342cb55de769bc6986f5870336d62ab51fa08e6d94d7cd790` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/rpm_b.img` | `1360e170c472acee6af1ff3c84f6436ed057720b1d11f7ab1797b315f2138764` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/sec.img` | `4fe7b59af6de3b665b67788cc2f99892ab827efae3a467342b3bb4e3bc8e5bfe` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/splash.img` | `54a5831fe0c4ce7869ebce08529e20c2e5bc3a391f00de182f9d4c8c7c544cf4` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/ssd.img` | `9f1dcbc35c350d6027f98be0f5c8b43b42ca52b7604459c0c42be3aa88913d47` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/sti.img` | `5647f05ec18958947d32874eeb788fa396a05d0bab7c1b71f112ceb7e9b31eee` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/storsec.img` | `e1ae88e05f5628ebf82baf3383ea0a764c577212ed0025f6494e3792331309b9` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/system_b.img` | `cf3143347ddd0b649a4a431ad735307ad07d2e7ffda2eadab34f576e112d2e81` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/toolsfv.img` | `30e14955ebf1352266dc2ff8067e68104607e750abb9d3b36582b8af909fcb58` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/tz_b.img` | `6fc5a8b30369fa72bafef46054007a9df455468500d0c91ceabe3b6a2b69b129` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/vision.img` | `55b446d1106a067a43d21eb6e8a21cf4b08c638baaf0b43fe16bb5bbf827314b` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/<SERIAL>/xbl_b.img` | `1d1af572fcf6c97b2d29b2cc7a52f03616eca8a4a2e1691742dc36b072ed311c` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-boot-4k-rootdebug-final.img` | `5610dce4ec95f9e0266da167987ac1b7378dc97b674a196664b7b4ed982bd3c7` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-boot-4k-rootready-final.img` | `621190ea2880728264953fbb559719e91a6bcbd1d3b36d7cb73262c094d675fb` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-system-4k-reproducible.img` | `68296c00645d1393351187cc0f48daab49b72e40ea39a49351a79f7ae90c1ca3` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-system-4k-ssh.img` | `ed8cc6d45b11eafb188fea1ec31df9f57077064812898aea799395641c464199` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-system-4k.img` | `520c94cf651e637e93c73f3e6337d445e9b39660d7425d3fa8d9cd4c84f41ad0` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-system-desktop.sparse.img` | `45a17ec7af1e477b5eacc79778950569feb8f7967d58c92a5458256057c8d4c8` |

## Per-unit calibration / optics

| Host | Exact path | SHA256 |
|---|---|---|
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/arm-smoke/calibration.json` | `eee8d748d09f58247d75b265c291661dbf595fa76c62f543337064e4d079bb24` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/calibration-feed.bin` | `9f81d2bcad0c78ffe5e3619880c44c98b47bb5caff6b94658d49c8a454c2c443` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/camera_calibration_v2.json` | `69b48eb1114f971c8aec3c6799161c07debdb59a93eb74205e5caf3d914904ec` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/distortion-mesh.bin` | `ee995926697408aaa0c15b6747d0163661bfaad653c861d84b50009cdfb45bd5` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/imu-propagation-audit-9kq7wbss/marked-calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/imu-propagation-audit-9kq7wbss/static-calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/imu_calibration.json` | `407ce90f8c5e80e800aa526a8f0a064687483effff6ce3c22acc773fba57d730` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/native-lean-01/calibration.json` | `eee8d748d09f58247d75b265c291661dbf595fa76c62f543337064e4d079bb24` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/raw-filter-audit-kbgcpj/calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/replay-cal/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/replay-v1/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/replay/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/replay/calibration_distrust.json` | `73af65f9ed217783f46c4e32ce5441840140c7a13185f8242f71eee7dd3a0ded` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-clock-04/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-clock-rectified-04/calibration.json` | `a7081678ab6766cebaaa8059bb086e59912c075d0b4ab2fab1c901a5ab8d1cc8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-rectified-02/calibration.json` | `a7081678ab6766cebaaa8059bb086e59912c075d0b4ab2fab1c901a5ab8d1cc8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-rectified-03/calibration.json` | `a7081678ab6766cebaaa8059bb086e59912c075d0b4ab2fab1c901a5ab8d1cc8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-rectified-04/calibration.json` | `a7081678ab6766cebaaa8059bb086e59912c075d0b4ab2fab1c901a5ab8d1cc8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-reordered-03/calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-reordered-04/calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-replay-01/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-replay-02/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-replay-03/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-replay-04/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-stereo-04/calibration.json` | `eee8d748d09f58247d75b265c291661dbf595fa76c62f543337064e4d079bb24` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-upper-stereo-04/calibration.json` | `2c70d8b4ffda686520a1e5dd15f85d84749351cda8950d69687ceb3dfbd4cd2c` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-replay-01/calibration.json` | `dff748834f17c08eb7d2f851adc89b7031dc5f9c140b6dfa2d53b1785551d8e7` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-static-rectified-01/calibration.json` | `a7081678ab6766cebaaa8059bb086e59912c075d0b4ab2fab1c901a5ab8d1cc8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-static-reordered-01/calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/calibration-feed.bin` | `9f81d2bcad0c78ffe5e3619880c44c98b47bb5caff6b94658d49c8a454c2c443` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/camera_calibration.json` | `d3583869514f36b837a104d51d30ebc9bec946c3fd2eed9e92f4751104ba46cd` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/camera_calibration_v2.json` | `69b48eb1114f971c8aec3c6799161c07debdb59a93eb74205e5caf3d914904ec` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/imu-propagation-audit-9kq7wbss/marked-calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/imu-propagation-audit-9kq7wbss/static-calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/imu_calibration.json` | `407ce90f8c5e80e800aa526a8f0a064687483effff6ce3c22acc773fba57d730` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/monado-predict-src/doc/example_configs/calibration_v2.example.json` | `4b872d63e098922b832058bc2d8424ef5ad038450f53780fddd376ad1f0ee79a` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/monado-predict-src/doc/example_configs/calibration_v2.schema.json` | `23a3ddbc1412f7e1d6453c35248f4346547280f153ffe6eebb5595c78149b285` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/proximity-calibration/PROX_PS_CAL_VERSION` | `6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/proximity-calibration/PROX_PS_CANC` | `7a61b53701befdae0eeeffaecc73f14e20b537bb0f8b91ad7c2936dc63562b25` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/proximity-calibration/PROX_PS_THDH` | `25fc0e7096fc653718202dc30b0c580b8ab87eac11a700cba03a7c021bc35b0c` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/proximity-calibration/PROX_PS_THDL` | `e29c9c180c6279b0b02abd6a1801c7c04082cf486ec027aa13515e4f3884bb6b` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/raw-filter-audit-kbgcpj/calibration.json` | `737663f66d39acee22771fcee8482c9021fbff428bef77da86451b81147366fb` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/stock-optics/distortion-mesh.bin` | `ee995926697408aaa0c15b6747d0163661bfaad653c861d84b50009cdfb45bd5` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/stock-optics/xrs-hmdconfig.capnp.bin` | `cd1f1eaa2a261f0c868ee9a6e0159e2313d8cbda217a550491ae7599bf330346` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/head-tracking/distortion-mesh.bin` | `ee995926697408aaa0c15b6747d0163661bfaad653c861d84b50009cdfb45bd5` |
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/head-tracking/performance-pause-20260930/distortion-mesh.bin` | `ee995926697408aaa0c15b6747d0163661bfaad653c861d84b50009cdfb45bd5` |

## Pre-experiment camera rollback bundle

| Host | Exact path | SHA256 |
|---|---|---|
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/camera-before-scene-bank-20261001.tar.gz` | `8c7c6c7046545c990df00a333edac22a9eecfc80ed9370c24830a64e05d80d9f` |

## Recording / raw feed / derived map (filename identifies role)

| Host | Exact path | SHA256 |
|---|---|---|
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/floor-map.bin` | `d1605b3b5a6b189645ddd6893e70fabdff41118af71c942111faa1274353be92` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/four-current-feed.bin` | `3325fedb398802d922069ec9e9f1dce66659f243d3e3394e318fd9b8fe1be2e8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/lit-feed.bin` | `f1ef38c2fb0a4b044a225519c4687ab2fb3a0e7bd517bb03d7978645d7143226` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/live-four-map.bin` | `c5924f615fa832ec4bac8afa4da10cc6e452ca7b01968b543690169c51510f02` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/live-lower-map-v2.bin` | `1d3f4e8de2b376a16ab72fcdddd94edc89c1d85107839e68cf31c4d64b1bbebe` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/live-lower-map-v3.bin` | `bfc630c80b7ec691697473feb4a012ca60cb6a764b0241309f0f00d0222d6e49` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/live-lower-map.bin` | `c75ba3ca43c363b09017f7b19021ef6e8e974f142974cd6feb82b2cd977748e1` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/marked-first10.bin` | `31db022536b9d9b72e58d7f4b3b737383b5efa81726c4c2117c783f4f9cbb5bc` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/native-opening-15hz.bin` | `319b6cb7b133e5a2cae34d8884974e56d5951e4b7b6677f8787570b04501ca0d` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/passthrough-map.bin` | `4c1f0d49d6c81f89039dd6c0d5b88c229696d72c8ac9fdff0e9f8b407a2ac23c` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/raw-filter-audit-kbgcpj/live-map.bin` | `c5924f615fa832ec4bac8afa4da10cc6e452ca7b01968b543690169c51510f02` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/raw-filter-audit-kbgcpj/raw-cohort-kbgcpj.bin` | `95ff9c594e4c3f207e0a4f766db6daf8bddea460462c0e2fe2fc130867a3d6e0` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-01.bin` | `bfb9fa586757e6291d97376e2e56e083c0b9b385e3b9eef67fa31ce203173382` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-02.bin` | `3b2f373647d717aca1810207981163a448a29b69d1044ac13a913bcb9099739c` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-03.bin` | `a69321c014d18ebdda04bcb06313fd569c6c6115310a1a4595371dbf12f1d584` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-motion-04-marked.bin` | `7b9d66064d8260a14c03ee4fff1b486123638c2ce855b71031da9aafa6f1d2e8` |
| Kali (<user>@<BUILD_HOST_IP>) | `/home/<user>/quest-camera-work/vio-record-01.bin` | `86180baa83c11b0da51e2b1e5c99ef344da41be9a65bf4a8b378ca96785f3e09` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/ae-after.bin` | `3900ca849075d95e57620160ca5dd6652b1ed9b5b723a4e00ad4e7f60e7a4eed` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/ae-before.bin` | `123c3e77faec64ed9271d1efb75c62bd0382646df487b9a2b170062264fb5f17` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/ae-sync.bin` | `53f62dd72844707ffafa3d942d0be8190aaaca852da76dd66b281e28a4623a74` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/auto-live-feed.bin` | `6d7c619a82fc99f01eecb5df1eb465bdecdf55d34377930c5c36448a20d049e1` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/current-camera-feed.bin` | `556488108abdc646aa793068bdecb3ad3c1888991a09747bf26a4aff7364e3a6` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/current-floor-map.bin` | `d1605b3b5a6b189645ddd6893e70fabdff41118af71c942111faa1274353be92` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/dim-feed.bin` | `1ca0fb4d1f4c87434bf010b97c4e1503a744abcd2155c1e76ec9872878b00b4f` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/exposure-after.bin` | `bcf7d2b9634744fa3f1eae8f0e74506d4c1d5d2daa78a30be7b08cddf2054d19` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/exposure-before.bin` | `7290aa74cbbcc9df30643ce021759926a3d5217d1865fb7e99a753b9a5de5029` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/feed-and-log.bin` | `a9bead1f6695a02a38e76d9211e782f747cd35714f69e87f9b4628762a8ca7c4` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/filtered-feed.bin` | `0268b664e02096e7acae97a91267fa6d938b28a09766e50b0f3ccdbf6503c02e` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/floor-map.bin` | `d1605b3b5a6b189645ddd6893e70fabdff41118af71c942111faa1274353be92` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/four-current-feed.bin` | `3325fedb398802d922069ec9e9f1dce66659f243d3e3394e318fd9b8fe1be2e8` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/four-gain4-feed.bin` | `e4e49096bacd945fd092e59f4352b109f740ca55ff5daa6aad591fe447c6db51` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/four-gain8-feed.bin` | `9936b8d03d90809256f3d4b5248e4bd3eb4c8fc128d7b8c3cde04acdb0f0885a` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/four-live-diagnostics-with-status.bin` | `ee6160b02a4c199d97c89151f3b3b05c6b6de38f29b62572f040fa40cda53bf3` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/fourcores-trial-mixed-snapshot.bin` | `2b644738ffc58145005dc31cc2436b65a9bea23f37540128c2b2f7e9c598cdd4` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/lit-feed.bin` | `f1ef38c2fb0a4b044a225519c4687ab2fb3a0e7bd517bb03d7978645d7143226` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/live-feed.bin` | `84d4a11d6ecb47929538a59c76f63889ce3862b15cc3d5e8484280f76af5e676` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/live-lower-map-v3.bin` | `bfc630c80b7ec691697473feb4a012ca60cb6a764b0241309f0f00d0222d6e49` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/live-vio-mailbox-pose.bin` | `8f17b3c61d51ea00409902eb3eaace91752ab930e6e5db48e33fdaeaef387a8e` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/native-opening-15hz.bin` | `319b6cb7b133e5a2cae34d8884974e56d5951e4b7b6677f8787570b04501ca0d` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/owner-spatial-ink-before-floor.bin` | `8a2a92d649aad7081facad4be5943d8501b3260beb53cbe500dfb5b161ff460d` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/passthrough-map.bin` | `4c1f0d49d6c81f89039dd6c0d5b88c229696d72c8ac9fdff0e9f8b407a2ac23c` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/playground-capture.bin` | `92798ad9c86a57d22e4a8e9b74e76a255efee4bd7b61256eaf542de5534bb58b` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/playground-health.bin` | `56ef90282d397d198375395d95c5a29d1340755b2aae41434d560002e7d7d46f` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/pre-wearer-feed.bin` | `e1b29f571f80fbf598f0bc01993d98fcdabcad72d90ad96ea9b6e3c0997b076b` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/profile10-log-and-feed.bin` | `22df4485ef768892d8c993e29087ddd1b3bed178ae2cbf6c9df0cb96fcde5ddb` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/raw-cohort-20261001.failed-empty.bin` | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/raw-cohort-kbgcpj.bin` | `95ff9c594e4c3f207e0a4f766db6daf8bddea460462c0e2fe2fc130867a3d6e0` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/raw-filter-audit-kbgcpj/live-map.bin` | `c5924f615fa832ec4bac8afa4da10cc6e452ca7b01968b543690169c51510f02` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/renderer-backup-with-manifest.bin` | `fd56d604df84f81c807b2d3456f4908cd449fd21dd697281962f2ebd8c0774cb` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/restarted-camera-with-status.bin` | `3beb0231c5dc61585ab8eb2e601784b0a6c6c9106386488613ba09e01be128f5` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/restarted-camera.bin` | `db14b0bda49d8b0fba88580e68fb86bb7602477aa9dd8db44606227755d85c56` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/unblocked-feed.bin` | `0e2b995feac1fa7bd057743652e6ba0c732605f541f2c9f8dccd6cfa65f8a0e6` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-current-camera-feed.bin` | `6cb8e1c8b8bd1f76f735da927a39f6fc7046ab377287ac540ee94558c691079d` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-motion-01.bin` | `bfb9fa586757e6291d97376e2e56e083c0b9b385e3b9eef67fa31ce203173382` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-motion-02.bin` | `3b2f373647d717aca1810207981163a448a29b69d1044ac13a913bcb9099739c` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-motion-03.bin` | `a69321c014d18ebdda04bcb06313fd569c6c6115310a1a4595371dbf12f1d584` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-motion-04-marked.bin` | `7b9d66064d8260a14c03ee4fff1b486123638c2ce855b71031da9aafa6f1d2e8` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-record-01.bin` | `86180baa83c11b0da51e2b1e5c99ef344da41be9a65bf4a8b378ca96785f3e09` |
| Mac (local; vela mounted) | `/Users/<user>/work/quest-pmos-bringup/camera-work/vio-wifi-preflight.bin` | `8f0c94023d74116bc77946e4afaa29087c6340b134bb7506ec36d144b6897a93` |

## Recovered unsigned guarded boot input

Retained from the temporary development directory during the reproducibility review.
This preserves the tested ramdisk; it does not replace a source-to-ramdisk build recipe.

| Host | Exact path | SHA256 |
|---|---|---|
| Mac (local; vela mounted) | `/Volumes/vela/Backups/quest1-recovery/pmos/4k-bringup/pmos-boot-4k-rootready-raw.img` | `3cb78145996a5dfe53714ff563ea8ad1cc0a9be6e49544ea56e87c7368146655` |

## Retained orientation-only Monado package

PKGINFO verifies aarch64 r3; this is a retained historical artifact, not a new clean rebuild.

| Host | Exact path | SHA256 |
|---|---|---|
| Mac (local) | `/Users/<user>/work/quest-pmos-bringup/monado-oculus-monterey-25.1.0_git20260822-r3.apk` | `23cc2ab039bd340424dd277afa51cecf647a1506cf9c3b5efc254ff1623fd160` |
