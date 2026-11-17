# Hardware readings

**Meu PC** shows CPU load, GPU load, RAM, and available temperatures and power readings. Choose indicators and a 1, 2, or 5 second interval in **Personalizar → Monitoramento**. RAM is displayed as total/available capacity beside its label, with a usage percentage.

CPU load and RAM use Windows readings. NVIDIA GPU readings use the installed NVML library when available. GPU utilization can also use Windows performance counters. Temperature and power can come from MSI Afterburner's read-only MAHM v2 shared memory, or compatible LibreHardwareMonitor/OpenHardwareMonitor WMI providers.

Missing or stale data displays an unavailable value. Hext does not infer temperature, convert power percentages into watts, install sensor drivers, launch monitoring software automatically, or apply hardware profiles.

Hardware work runs outside the animation loop. Requests are made only when relevant indicators are visible. The NVIDIA context is released after an idle interval. Availability varies by GPU, driver, permissions, and active sensor provider.

`--verify-hardware` reads the local sources and exports a report and renderer images without changing device settings. Tests cover stale samples, shared-memory bounds, package selection, units, and unavailable data.
