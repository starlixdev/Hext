# Audio

Output and input lists show real Windows endpoints, including unavailable devices. Selecting a device changes the control target. **Definir como padrão** explicitly changes the Console and Multimedia defaults while retaining the separate Communications default. If the isolated, undocumented policy interface is unavailable, Hext opens Windows' audio settings.

Volume and mute use Core Audio. Stereo outputs also expose balance. **Mais ajustes** opens the native playback or recording panel for driver-specific options. Hext does not capture the microphone or play a test sound automatically. Device lists show four entries per page. The quiet counter uses **total/current**, such as `5/1`.

## Hext Audio

The optional add-on reuses EqualizerAPO-XT 2.64.0's unmodified x64 SSE2 installer. **Baixar add-on** downloads it from a pinned GitHub Release in a worker thread. Hext validates the size and SHA-256 before exposing **Instalar add-on**, and checks it again before launch. Failed or cancelled downloads cannot become runnable installers. Files are cached in `%LOCALAPPDATA%\Hext\addons\audio`.

The installer runs only after a separate click. Windows may require permission when the upstream device selector registers audio effects. Select an output using **Configurar saída** and follow the upstream restart instructions if requested. Hext never restarts the Windows audio service automatically.

Hext detects the component and its endpoint registration before enabling effects. It offers amplification from 0 to +12 dB and bass, midrange, and treble controls from -12 to +12 dB, in 0.5 dB steps. Positive gain can clip; Hext does not currently add a limiter. The installer and audio engine have not been modified or renamed internally.

Changes are drafts until **Aplicar**. The effects toggle also needs Apply. Hext writes a device-scoped `Hext.txt`, adds an Include to the existing APO configuration, and keeps one `config.before-hext.txt` backup. Existing user settings and unrelated filters are preserved. A file named `Hext.txt` that is not owned by Hext is not overwritten. The updater restores `Device: all` before including Hext's file so an earlier device scope cannot suppress it.

The APO continues processing after Hext closes. To remove Hext's effects, disable them and apply. To remove the component itself, use Windows' installed-apps interface and the upstream uninstallation instructions. Downloading an installer does not install it.

Registration alone cannot prove that every stream is processed. Exclusive-mode audio, raw paths, disabled enhancements, and driver behavior can bypass an APO. Windows 10 and 11 hardware coverage still needs wider testing. Automated Hext tests verify download integrity and configuration behavior without changing live audio devices.

## Bluetooth

Hext lists devices known to the classic Windows Bluetooth APIs. Search performs discovery on demand in a worker. Pairing and connection properties use native Windows dialogs. Forgetting a device requires confirmation. **Gerenciar** opens Windows Bluetooth settings for connection profiles and LE devices that may not appear in the classic API. Hext does not simulate disconnecting by disabling a Bluetooth service.

References: [Core Audio volume](https://learn.microsoft.com/en-us/windows/win32/api/endpointvolume/nf-endpointvolume-iaudioendpointvolume-setmastervolumelevelscalar), [APO configuration](https://github.com/115dkk/EqualizerAPO-XT/wiki/Configuration-reference), [upstream source and licenses](../addons/audio/upstream.json).
