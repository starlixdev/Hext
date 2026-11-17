# Audio

Output and input lists show real Windows endpoints, including unavailable devices. Selecting a device changes the control target. **Definir como padrão** explicitly changes the Console and Multimedia defaults while retaining the separate Communications default. If the isolated, undocumented policy interface is unavailable, Hext opens Windows' audio settings.

Volume and mute use Core Audio. Stereo outputs also expose balance. **Mais ajustes** opens the native playback or recording panel for driver-specific options. Hext does not capture the microphone or play a test sound automatically. Device lists show four entries per page. The quiet counter uses **total/current**, such as `5/1`.

## Removed audio add-on

Starting with 0.7.3, Hext no longer offers an audio add-on, amplification, or equalization. It does not download installers or write APO configuration files. Native volume, mute, balance, device selection, and Bluetooth remain available.

Upgrading Hext does not uninstall a component installed by an earlier release. Remove **EqualizerAPO-XT** through Windows' installed-apps interface if you previously installed it. Its uninstaller can require administrator permission to restore device registration. Follow any restart instructions it displays.

## Bluetooth

Hext lists devices known to the classic Windows Bluetooth APIs. Search performs discovery on demand in a worker. Pairing and connection properties use native Windows dialogs. Forgetting a device requires confirmation. **Gerenciar** opens Windows Bluetooth settings for connection profiles and LE devices that may not appear in the classic API. Hext does not simulate disconnecting by disabling a Bluetooth service.

References: [Core Audio volume](https://learn.microsoft.com/en-us/windows/win32/api/endpointvolume/nf-endpointvolume-iaudioendpointvolume-setmastervolumelevelscalar).
