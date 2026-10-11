---
name: gba-build
description: Build the GBScribe GBA ROM via the devkitPro Docker image, keep a dist/ copy, and deploy to the flash cart. Use for any build, rebuild, or "test on hardware" request.
---

# Building the ROM (Docker) and deploying to the cart

devkitARM is NOT installed natively (apt.devkitpro.org blocks curl/wget, WSL sudo needs
a password). All ROM builds run inside the `devkitpro/devkitarm:latest` Docker image.

## Build

From Git Bash, repo root:

```sh
cd /d/Coding/GBScribe && ./build.sh          # -> GBScribe.gba in repo root
./build.sh clean                             # make clean
```

- Docker Desktop must be running. If the daemon is down:
  `Start-Process "C:\Program Files\Docker\Docker\Docker Desktop.exe"` then wait ~1 min.
- `build.sh` mounts the CURRENT directory into the container (`pwd -W`), so it also
  works from a git worktree — that's how release ROMs are built at a tag without
  touching the main working tree.
- Every successful build is also copied to `dist/GBScribe.gba` (gitignored) by build.sh.
- The printer driver is the AdvancePrint git submodule at `lib/AdvancePrint/`;
  make sure it is checked out (`git submodule update --init --recursive`) or the
  ROM link step fails on missing `lib/AdvancePrint/src/*.c`.
- The Makefile globs `src/**/*.c` by directory list (`SRCDIRS`) plus
  `lib/AdvancePrint/src`; new generated .c files in `src/gfx/` are picked up
  automatically, but a NEW directory must be added to `SRCDIRS` in the Makefile.
- Bash tool cwd can reset between calls — always `cd /d/Coding/GBScribe` first.

## Deploy to flash cart (user rule — do after EVERY build)

Copy the ROM to `F:\Homebrew` ONLY if drive F: volume label is exactly `GBA`
(case-sensitive). Either run `& tools\deploy.ps1` or inline:

```powershell
$v = Get-Volume -DriveLetter F -ErrorAction SilentlyContinue
if ($v -and $v.FileSystemLabel -ceq 'GBA') { Copy-Item GBScribe.gba F:\Homebrew\ -Force }
```

Skip silently if the cart isn't mounted. Never use `powershell -ExecutionPolicy Bypass`
(denied by permission policy) — invoke scripts directly with `& tools\script.ps1`.

## After the build

- Run the host tests (see the `gba-host-tests` skill) — they compile the same core
  sources with gcc and catch logic regressions the GBA build can't.
- Emulator sanity check via the `mgba-capture` skill if the desktop is unlocked.
- Hardware-only behavior (printer link, SRAM saves, audio) can only be confirmed by
  the user on the EZ-Flash Omega; ask for a cart test rather than guessing.
