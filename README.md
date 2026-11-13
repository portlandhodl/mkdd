Mario Kart Double Dash!!  
[![Build Status]][actions] [![Code Progress]][progress] [![Data Progress]][progress] [![Discord Badge]][discord]
=============

[Build Status]: https://github.com/doldecomp/mkdd/actions/workflows/build.yml/badge.svg
[actions]: https://github.com/doldecomp/mkdd/actions/workflows/build.yml
[Code Progress]: https://decomp.dev/doldecomp/mkdd.svg?mode=shield&measure=code&label=Code
[Data Progress]: https://decomp.dev/doldecomp/mkdd.svg?mode=shield&measure=data&label=Data
[progress]: https://decomp.dev/doldecomp/mkdd
[Discord Badge]: https://img.shields.io/discord/727908905392275526?color=%237289DA&logo=discord&logoColor=%23FFFFFF
[discord]: https://discord.gg/hKx3FJJgrV

WIP 1:1 Decompilation of Mario Kart Double Dash!!

This repository does **not** contain any game assets or assembly whatsoever. An existing copy of the game is required.

Currently only the debug version is supported.

Progress
========

Snapshot of this fork against upstream [doldecomp/mkdd](https://github.com/doldecomp/mkdd)
(`b56cc76`), both built from source on 2026-11-13. The badges above track upstream.

| | Upstream | This fork |
|---|---:|---:|
| Code matched | 47.90% | **93.05%** |
| Functions matched | 8,737 / 15,357 | **15,099 / 15,357** |
| Units fully linked | 519 / 807 | **713 / 807** |
| Data matched | 25.08% | **46.31%** |

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/progress-overview-dark.svg">
  <img alt="Code matched by category, upstream vs this fork" src="docs/images/progress-overview-light.svg">
</picture>

Each bar below is all code owned by one area. Dark segments are linked into `main.dol` from
source marked `Matching`; light segments are functions that match but sit in units that still
contain unmatched functions. The tick marks where upstream stands.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/progress-areas-dark.svg">
  <img alt="Progress by area: linked, matched-not-linked and unmatched code, with the upstream position" src="docs/images/progress-areas-light.svg">
</picture>

What is left, one box per unit sized by unmatched code bytes. Most of the remainder is
register-allocation differences in large Kameda/Sato scene and object code.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/progress-remaining-dark.svg">
  <img alt="Treemap of remaining unmatched code by unit" src="docs/images/progress-remaining-light.svg">
</picture>

Regenerate the charts after a build with
`python tools/progress_charts.py --upstream <upstream checkout>/build/MarioClub_us/report.json`.

Dependencies
============

Windows
--------

On Windows, it's **highly recommended** to use native tooling. WSL or msys2 are **not** required.  
When running under WSL, [objdiff](#diffing) is unable to get filesystem notifications for automatic rebuilds.

- Install [Python](https://www.python.org/downloads/) and add it to `%PATH%`.
  - Also available from the [Windows Store](https://apps.microsoft.com/store/detail/python-311/9NRWMJP3717K).
- Download [ninja](https://github.com/ninja-build/ninja/releases) and add it to `%PATH%`.
  - Quick install via pip: `pip install ninja`

macOS
------

- Install [ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages):

  ```sh
  brew install ninja
  ```

- Install [wine-crossover](https://github.com/Gcenx/homebrew-wine):

  ```sh
  brew install --cask --no-quarantine gcenx/wine/wine-crossover
  ```

After OS upgrades, if macOS complains about `Wine Crossover.app` being unverified, you can unquarantine it using:

```sh
sudo xattr -rd com.apple.quarantine '/Applications/Wine Crossover.app'
```

Linux
------

- Install [ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages).
- For non-x86(_64) platforms: Install wine from your package manager.
  - For x86(_64), [wibo](https://github.com/decompals/wibo), a minimal 32-bit Windows binary wrapper, will be automatically downloaded and used.

Building
========

- Clone the repository:

  ```sh
  git clone https://github.com/doldecomp/mkdd.git
  ```

- Copy your game's disc image to `orig/MarioClub_us`.
  - Supported formats: ISO (GCM), RVZ, WIA, WBFS, CISO, NFS, GCZ, TGC
  - After the initial build, the disc image can be deleted to save space.

- Configure:

  ```sh
  python configure.py
  ```

  To use a version other than `MarioClub_us` (USA), specify it with `--version`.

- Build:

  ```sh
  ninja
  ```

Diffing
=======

Once the initial build succeeds, an `objdiff.json` should exist in the project root.

Download the latest release from [encounter/objdiff](https://github.com/encounter/objdiff). Under project settings, set `Project directory`. The configuration should be loaded automatically.

Select an object from the left sidebar to begin diffing. Changes to the project will rebuild automatically: changes to source files, headers, `configure.py`, `splits.txt` or `symbols.txt`.

![](assets/objdiff.png)

## Credits
- [SwareJonge](https://github.com/SwareJonge) for originally starting this project, and for guiding many people in their first contributions towards a decomp.
- [Seeky](https://github.com/SeekyCt) for ppcdis, without it this project wouldn't have been where it is now.
- [encounter](https://github.com/encounter) for their amazing tools (decomp-toolkit, ObjDiff, et al.) which have so helped not only this project, but the entire decomp community.
### Matching
Used code from the following repositories(JSystem, SDK)
* [Metroid Prime Decomp](https://github.com/PrimeDecomp/prime)
* [Pikmin 2 Decomp](https://github.com/projectPiki/pikmin2)
* [Super Mario Galaxy Decomp](https://github.com/shibbo/Petari)
* [Super Mario Sunshine Decomp](https://github.com/doldecomp/sms)
* [Syati](https://github.com/PMArkive/Syati-pretransfer)
* [Twilight Princess Decomp](https://github.com/zeldaret/tp)

And many thanks to the people who helped me match some game functions (sorry if anyone is forgotten.)
* [Chippy](https://github.com/1superchip)
* [kiwi](https://github.com/kiwi515)
* [Seeky](https://github.com/SeekyCt)
