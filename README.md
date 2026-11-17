# PLOC

PLOC is a water-inspired synth for the **original Teenage Engineering OP-1** running **Hundekraft**. Play drops, bursts of spray and ripples as pitched sounds, with a tap that turns as you change FLOW.

![Rendered preview of the PLOC screen](docs/PLOC-0.6.2-OLED-preview.png)

**0.6.2 was verified on Hundekraft 3.1.4.** The workshop verified installation on an original OP-1 running Hundekraft 3.1.4 (R246). The image above is a render; the revised layout and graphics performance still need a check on the actual OLED. Other Hundekraft versions have not been tested. The OP-1 field is not supported.

## Downloads

| Firmware on the original OP-1 | Build | Download |
| --- | --- | --- |
| **Hundekraft 3.3** | **PLOC 0.7.0-rc1** · one plugin file with all knob and Shift controls | [Install ZIP with 8 presets](./plugins/hundekraft-3.3/ploc-0.7.0-rc1-install.zip?raw=true) · [ploc.knl](./plugins/hundekraft-3.3/ploc.knl?raw=true) |
| Hundekraft 3.1.4 | PLOC 0.6.2 · synth and companion controller | [Install ZIP with 8 presets](./plugins/ploc-0.6.2-install.zip?raw=true) |

The 3.3 build passes host checks; hardware testing is pending. When upgrading PLOC on 3.3, replace the synth and remove the old PLOCCTRL entry if present. See the [changelog](CHANGELOG.md).

## Install

[**Download PLOC 0.6.2 for Hundekraft**](./plugins/ploc-0.6.2-install.zip?raw=true)

For an original OP-1 already running Hundekraft 3.1.4:

1. Back up your OP-1 and keep your previous plugin files if you are updating. Extract the ZIP or download [ploc.knl](./plugins/ploc.knl?raw=true) and [plocctrl.knl](./plugins/plocctrl.knl?raw=true). PLOC is version 0.6.2; its companion controller is version 0.6.1. If you already have PLOC 0.6.1 and its controller, only replace `ploc.knl`.
2. Connect the OP-1 over USB. In Firefox, open the [Hundekraft Workshop](https://hundekraft.github.io/op1-workshop/) matching your installed build. In **Your plugins**, import the files and use **Update OP-1** to review the changes.
3. Check that the plan only adds or replaces the PLOC components you selected and keeps the rest of your plugins. If the workshop proposes a full firmware update or unexpected changes, stop and check the workshop and firmware versions before proceeding. Follow the workshop prompts to apply the plan, restart, and check that the workshop reports a verified update.
4. Load PLOC from the synth browser, or copy the included preset bank below.

To roll back, use the workshop to restore your previous PLOC files, or remove PLOC and PLOCCTRL if you installed them for the first time. Review the removal plan and leave your other plugins selected. See the [Hundekraft manual](https://hundekraft.github.io/op1-workshop/manual.html) for firmware setup or recovery.

## Presets

The install ZIP includes eight PLOC presets: flow eau, flow voile, goutte, lac calme, perle fine, pluie vive, pouic air, and ruissel. If you already downloaded the plugin, [download just the presets](./plugins/ploc-presets.zip?raw=true). The presets retain their saved effect settings; those effects need to be present in your Hundekraft build.

After installing the plugins, connect the OP-1 over USB and press **Shift + COM**, then **DISK (T3)**. Copy the extracted `ploc` folder from `synth/` into the OP-1 drive’s `synth/` folder. Eject the OP-1, let it finish syncing, then use **Shift + 1–8** to browse the sounds. If you already have the eight PLOC presets, skip this step to avoid duplicates. See the [OP-1 disk mode guide](https://teenage.engineering/guides/op-1/original/song-rendering-and-connectivity#13.5-disk-mode).

## Controls

| Encoder | Turn | Shift + turn |
| --- | --- | --- |
| Blue | FLOW | Reserved |
| Green | SIZE | HEIGHT |
| White | SPRAY | QUALITY |
| Red | RIPPLE | IMPACT |

The sound engine is unchanged from 0.5.0. Version 0.6.2 adjusts the screen’s numbers, QUALITY label and ripple position.

## Source and credits

See the [build instructions](source/README.md), the [source files](source/plugins/), and the [provenance notes](source/PROVENANCE.md). PLOC is an independent plugin by Nicolas Franck Pauly, built with the Hundekraft Kennel SDK. Distributed under GPL-3.0-or-later with the firmware permission described in [NOTICE](NOTICE). [Checksums](plugins/SHA256SUMS) are provided for both binaries.
