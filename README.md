# Cobalt64 RX 9070 XT Tahoe lab

Experimental source changes to [Almosst-DEV/Cobalt64](https://github.com/Almosst-DEV/Cobalt64), tested on one RX 9070 XT Hackintosh running macOS Tahoe **26.7.1 (25G241)**. This repository preserves useful work from a stopped development session. It is a patchset and research checkpoint, not a complete GPU installer or an independently written replacement for Cobalt64.

On the test machine, GPU desktop compositing and compute worked at **3840×2160, approximately 60 Hz**. Discord screen sharing and Ghostty worked after process-name admission. **4K120, 160 Hz, DSC and the second HDMI monitor do not work.** Host tests of future DisplayPort code do not establish hardware support for those modes.

## Tested configuration and limits

| Component | Observed result |
|---|---|
| i5-14600KF, MSI PRO B760M-A DDR4 II, RX 9070 XT / Navi 48 | Booted installed Tahoe from SSD with the experimental stack |
| Acer XB323QK V4 on DisplayPort, AUX2 / DP2 / PHY C | Physical 4K60; current link HBR2 ×4, RGB 10 bpc |
| Samsung LF24T35 on HDMI | Not active; geometry draft remains unfinished |
| GPU desktop | GPU submissions, pipe activity and GPU-backed scanout observed; initial artifacts and a roughly 90-second purple screen occurred during bring-up |
| Compute | Earlier 2,048-word check returned zero mismatches |
| Apps | User confirmed Discord sharing and Ghostty after exact process-name admission |
| Metal API | Partial implementation; 81 missing selectors were identified. Full Metal 3 compatibility is not claimed |
| Video encode/decode | Hardware codec support is not established |

The monitor advertises HBR3 ×4 and a native 4K120 timing at 1,066,560,000 Hz, with totals 4000×2222. RGB 8 bpc needs 25.59744 Gbit/s and fits the 25.92 Gbit/s HBR3 payload. This is a bandwidth calculation, not a successful mode switch. A complete PHY/clock/DML/TTU/mode-setting/rollback path is still missing. RGB 10 bpc at this timing and the requested 144/160 Hz RGB modes need capabilities not implemented here.

## What is included

- `patches/cobalt64-local.patch`: changes against pinned public Cobalt64 commit `d6c9aedf8e6526a39e349b5886cbc8c7ddf77f65`, including AGDC/display diagnostics, serialized firmware reads, DAL-query and full DTO-clock decoding/encoding. Some newer timing fixes were built in isolation and were **not** installed in the working 4K60 driver.
- `rdna4fb-overlay/`: RDNA4FB source adaptation for the opted-in Cobalt path, bounded AUX/EDID survey and extended DPCD capabilities. Based on `somestupidgirl/RDNA4FB` commit `5ff69aade9a7feb18bb665040661de33ccfe68ba`.
- `lab/DPControl/`: source of the 0.0.2 diagnostic module. Fixed GET_FW_VERSION query passed on the test card with response `0xe840`; this value does **not** establish the ABI of display mode commands. Code-window reads are bounded and CW0/CW1 were disabled on this GOP firmware. No arbitrary MMIO write interface is exposed.
- `lab/`: AUX2 transaction, DP2 source/stream and link-training prototypes, DTO arithmetic and firmware-reader tests. These prototypes have no wired live HBR3 backend and have never switched this card to 120 Hz.
- `helpers/`: exact-process admission and pre-login desktop guard logic, plus translator sandbox/real-validator changes. The desktop helper can restart WindowServer during its guarded pre-login experiment; it is source for review, **not** a script to run on an active desktop.
- `references/`: AMD DCN4.1 register headers and provenance; `tests/`: bounded host-model regression runner.

## Reproduce the host tests

Requires Python 3, Git and a Clang toolchain with ASan/UBSan. These tests never open the GPU service or change display state.

```sh
python3 tools/prepare_sources.py
python3 tests/run.py
```

The first command fetches pinned **public** Cobalt64 sources into ignored `vendor/Cobalt64`, checks and applies this patch. The RDNA4FB survey test uses a synthetic EDID without a real monitor serial. Results from this export are in `tests/results.json`.

## Building the installed stack is not reproduced by those commands

The full bring-up/accelerator/Metal stack additionally depends on MacKernelSDK, Mesa/RADV with Cobalt64's nine Darwin patches, metal2vulkan, AMD firmware and locally derived IOAccelFamily2 ABI artifacts for the exact Apple build. Required generated declarations/layout artifacts were absent from the pinned public upstream tree. This repository deliberately does not publish privately extracted Apple ABI artifacts, WindowServer AIR/SPIR-V caches, Apple system binaries, firmware/VBIOS blobs, personal EFI identifiers or a prebuilt Metal bundle. No standalone full-stack build or ready-to-install release is claimed.

For the diagnostic module, `MACKERNELSDK=/path/to/pinned/MacKernelSDK python3 lab/DPControl/build.py` describes its compile/link steps after preparing a complete compatible Cobalt64 development tree. It depends on the Bringup 0.0.665 symbol ABI and is not an installation command. The runtime-only `.live` loading experiment failed because the auxiliary collection could not link the EFI-injected dependency; the working module was injected after Bringup by OpenCore instead.

The tested personal system had custom SIP `03000000` and `amfi_get_out_of_my_way=1`; authenticated root remained enabled. Those settings reduce platform protections and are part of the experiment, not general configuration advice. Driver UUIDs, private shader-cache hashes and exact-build guards make the installed checkpoint specific to 25G241. It must not be represented as a generic Tahoe driver.

## Attribution and license

Original GPU implementation, Metal/Mesa integration and the foundation of this work are from **Almosst-DEV and the Cobalt64 contributors**. The local changes here were developed with Codex assistance; this export is not upstream-endorsed. New local code is MIT licensed under the included MIT terms. Cobalt64 changes retain the original MIT notice in `LICENSE`; RDNA4FB adaptations retain Sunneva N. Mariu's BSD 3-Clause terms in `licenses/RDNA4FB-BSD-3-Clause.txt`; AMD register-derived code retains the AMD notice in `licenses/AMD-MIT.txt` and the headers. See `provenance.json` for pinned source versions. Firmware, Apple artifacts and third-party binary distributions are outside this source license.

Development paused on 2026-10-14 at the owner's request. No public issue was filed with the upstream author.
