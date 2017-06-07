# HstWB Package

HstWB is a minimalistic Workbench enhancement by Henrik Nørfjand Stengaard based on using BetterWB targeted A500, A600, A1200 using 16 colour screenmode using PAL / NTSC / Non-Interlaced 640x256 display.

HstWB contains following features:

- Adds and replaces Workbench icons with icons from ClassicWB.
- Build-in support for ACA, Furia and Blizzard accelerator cards.
- Boot selectors for starting AGS2 Games, Demos menus, Workbench or CLI for maximum available memory.
- Configure setup: A menu to easily configure setup of hardware, device & library, boot sequence and WHDLoad.
- Configure kickstart roms: A menu to easily install Kickstart roms for WHDLoad.
- Tools installed: MUI, WHDLoad.
- Programs installed: DirOpus, DCopy, Deluxe Paint 4, DiskSalv, Find, HippoPlayer, JoyPortTest, ProTracker, ReOrg, SuperDuper, SysInfo, TSgui, TransWrite, VirusZ and Visage.

## Requirements

HstWB package requires BetterWB package is installed, when selecting packages to install in either HstWB Installer install mode or running a self install image.

## Installation

Download latest release from https://github.com/henrikstengaard/hstwb-package/releases and copy it to HstWB Installer "packages" directory, which typically is "c:\Program Files (x86)\HstWB Installer\Packages".

Installation through HstWB Installer will install and configure HstWB package using defined assigns.

## Assigns

Installation of HstWB package requires and uses following assign and default value:

- SYSTEMDIR: = DH0:

HstWB files will be installed and configured in SYSTEMDIR: assign, which must be set to harddrive containing Workbench.

## Build-in support for accelerator cards

HstWB has build-in support for following accelerator cards:

- Blizzard 1230, 1240, 1260 and PPC accelerator cards by Phase 5 with MAPROM jumper enabled using BlitzKick v1.24.
- ACA 620, 630, 1220, 1221, 1230, 1231, 1232 accelerator cards by Individual Computers using ACATune v1.91.
- Furia A600 accelerator cards by Boris Krizma for using FuriaTune v1.4.

Picture showing MAPROM jumper enabled on Blizzard 1230 MK-IV accelerator card.

![Blizzard 1230 MK-IV MAPROM Enabled](screenshots/blizzard_1230_mk-iv_maprom_enabled.jpg?raw=true)

## Boot selectors

HstWB has 2 boot selectors installed, which can be enabled using configure setup:

1. Amiga Boot Selector
2. Multi Boot

### Amiga Boot Selector

Amiga Boot Selector is a fullscreen boot selector with smooth drop in and out effects when loading and a boot option is selected. It allows selecting boot options using arrow keys and enter or joystick in port 2. Mouse is not supported. 

HstWB is configured with following boot options in Amiga Boot Selector:

1. Games: Starts AGS2 games menu, if it's installed.
2. Demos: Starts AGS2 demos menu, if it's installed.
3. Workbench: Starts Workbench and enabled patches.
4. CLI: Starts CLI for maximum avialable memory.

A 5 second timer will automatically select last used boot option, which by default is Games.

Amiga Boot Selector can be changed by pressing F1. This allows changing existing boot options or add new ones.

![HstWB 2](screenshots/hstwb_2.png?raw=true)

### Multi Boot

Multi boot is a simple boot selector presented like a request dialog with buttons per boot option. It only supports mouse in port 1. 

HstWB is configured with following boot options in Multi Boot:

1. Games: Starts AGS2 games menu, if it's installed.
2. Demos: Starts AGS2 demos menu, if it's installed.
3. Workbench: Starts Workbench and enabled patches.
4. CLI: Starts CLI for maximum avialable memory.

A 5 second timer will automatically select default boot option, which is Games by default and can be changed using configure setup.

![HstWB 3](screenshots/hstwb_3.png?raw=true)

## Configure setup

Configure setup is a menu to change hardware, device & library, boot sequence and WHDLoad settings. 
It's build using AmigaDOS script and ReqList to make menus for easily changing options.

Configure setup can be started in 2 following ways:

- Double-click Workbench, System, Programs, Configuration and Configure-Setup icon.
- Hold down right mouse button during boot.

Start of configure setup during boot has been added as a recovery option allowing setup to be reset or changed, if boot sequence or Workbench could result in a crash.

![HstWB 6](screenshots/hstwb_6.png?raw=true)

### Configure setup main menu

Configure setup main menu has following options:

- Hardware: Enters hardware menu.
- Device & Library: Enters device & library menu.
- Boot Sequence: Enters boot sequence menu.
- WHDLoad: Enters WHDLoad menu.
- Reset: Resets configuration to default and reboot system with a delay of 10 seconds.
- Reboot: Reboot system with a delay of 10 seconds.
- Exit: Exit Configure setup.

![HstWB 7](screenshots/hstwb_7.png?raw=true)

### Hardware menu

Hardware menu has following options:

- Use Blizzard: Enables or disables BlizKick for Blizzard accelerator cards during boot.
- Use ACA: Enables or disables ACATune for ACA accelerator cards during boot.
- Use Furia: Enables or disables FuriaTune for Furia accelerator cards during boot.
- Use 020+ CPU: Enables or disables use of 020+ CPU by replacing executables and libraries with either 68000 or 68020+ versions.
- Use FPU: Enables or disables use of FPU by replacing libraries with either 68000 or 68020+ versions.
- Use OS Patches: Enables or disables BlazeWCP, FBlit and FText during Workbench startup.
- Back: Back to configure setup main menu.

![HstWB 8](screenshots/hstwb_8.png?raw=true)

Enabling Use Blizzard asks to maprom A1200 Kickstart 3.1.

![HstWB 9](screenshots/hstwb_9.png?raw=true)

Enabling Use ACA asks to maprom A600 Kickstart 3.1.

![HstWB 10](screenshots/hstwb_10.png?raw=true)

Enabling Use ACA asks to maprom A1200 Kickstart 3.1.

![HstWB 11](screenshots/hstwb_11.png?raw=true)

Enabling Use Furia asks to maprom A600 Kickstart 3.1.

![HstWB 12](screenshots/hstwb_12.png?raw=true)

### Device & Library menu

Device & Library menu has following options:

- Load scsi.device: Enables or disables loading DEVS:scsi.device during boot, if DEVS:scsi.device exist and uses either LoadModule or BlitzKick depending on Use Blizzard being enabled or disabled.
- Load icon.library: Enables or disables loading LIBS:icon.library during boot, if DEVS:scsi.device exist and uses either LoadModule or BlitzKick depending on Use Blizzard being enabled or disabled.
- Select scsi.device: Enters select scsi.device menu.
- Back: Back to configure setup main menu.

![HstWB 13](screenshots/hstwb_13.png?raw=true)

### Select scsi.device menu

Select scsi.device menu has following options:

- Build-in (Kickstart ROM): Disables loading scsi.device during boot by deleting DEVS:scsi.device and uses scsi.device from Kickstart rom.
- v43.35 Chris/Toni A1200: Selects and copies v43.35 Chris/Toni A1200 patched scsi.device to DEVS:scsi.device. It supports harddrives larger than 4GB and is the recommended choice.
- v43.46 Cosmos A600/A1200: Selects and copies v43.46 Cosmos A600/A1200 patched scsi.device to DEVS:scsi.device. It supports harddrives larger than 4GB and has some experimental optimizations.
- v43.47 Cosmos A600/A1200: Selects and copies v43.47 Cosmos A600/A1200 patched scsi.device to DEVS:scsi.device. It supports harddrives larger than 4GB and has some experimental optimizations.
- v44.20 Doobrey A600: Selects and copies v44.20 Doobrey A600 patched scsi.device to DEVS:scsi.device. It supports harddrives larger than 4GB and has ide speed optimizations similar to BlitzKick SpeedyIDE.
- v44.20 Doobrey A1200: Selects and copies v44.20 Doobrey A1200 patched scsi.device to DEVS:scsi.device. It supports harddrives larger than 4GB and has ide speed optimizations similar to BlitzKick SpeedyIDE.
- Back: Back to configure setup main menu.

![HstWB 14](screenshots/hstwb_14.png?raw=true)

### Boot Sequence menu

Boot Sequence menu has following options:

- Start ABS: Enables or disables start of Amiga Boot Selector during boot.
- Start MultiBoot: Enables or disables start of Multi Boot during boot.
- Select MultiBoot Default: Selects Multi Bool default boot option, which will be selected when the timer runs out.
- Start AGS2 Games: Enables or disables start of AGS2 games menu during boot.
- Start AGS2 Demos: Enables or disables start of AGS2 demos menu during boot.
- Back: Back to configure setup main menu.

![HstWB 15](screenshots/hstwb_15.png?raw=true)

### WHDLoad menu

WHDLoad menu has following options:

- Set Quit Key: Select global quit key for WHDLoad, which can be F10, Help, Del, NumL, ScrL, SysRq, PrtSc or Default.
- Back: Back to configure setup main menu.

Setting a quit key is very usefor for A600 users as default quit key is often a key not available on A600 keyboard.

![HstWB 16](screenshots/hstwb_16.png?raw=true)

![HstWB 17](screenshots/hstwb_17.png?raw=true)

## Configure kickstart roms

Configure kickstart roms is a menu to install or reset Kickstart roms for WHDLoad.
It's build using AmigaDOS script and ReqList to make menus for easily changing options.

Configure kickstart roms can be started by double-click Workbench, System, Programs, Configuration and Configure-Kickstart-Roms icon.

![HstWB 6](screenshots/hstwb_6.png?raw=true)

### Configure kickstart roms main menu

Configure kickstart roms main menu has following options:

- kick33180.A500: Indicates if Cloanto Amiga Forever or Custom Kickstart 1.2 (33.180) (A500) rom is installed in DEVS:Kickstarts.
- kick34005.A500: Indicates if Kickstart 1.3 (34.5) (A500) rom is installed in DEVS:Kickstarts.
- kick40063.A600: Indicates if Kickstart 3.1 (40.063) (A600) rom is installed in DEVS:Kickstarts.
- kick40068.A1200: Indicates if Kickstart 3.1 (40.068) (A1200) rom is installed in DEVS:Kickstarts.
- kick40068.A4000: Indicates if Kickstart 3.1 (40.068) (A4000) rom is installed in DEVS:Kickstarts.
- rom.key: Indicates if Cloanto Amiga Forever rom key is installed in DEVS:Kickstarts.
- Install Kickstart Roms: Install Kickstart roms from a selected drawer. MD5 checksum is calculated for each file in selected drawer used to identify and install Kickstart roms in DEVS:Kickstarts.
- Reset Kickstart Roms: Resets installed Kickstart roms by deleting Kickstart roms and rom.key files from DEVS:Kickstarts.
- Exit: Exit Configure kickstart roms.

**Note: Cloanto Amiga Forever Kickstart roms are encrypted and requires their rom.key is installed. Otherwise WHDLoad can't use Cloanto Amiga Forever Kickstart roms.**

![HstWB 18](screenshots/hstwb_18.png?raw=true)

![HstWB 19](screenshots/hstwb_19.png?raw=true)

## Screenshots

Screenshots of HstWB.

![HstWB 1](screenshots/hstwb_1.png?raw=true)

![HstWB 2](screenshots/hstwb_2.png?raw=true)

![HstWB 3](screenshots/hstwb_3.png?raw=true)

![HstWB 4](screenshots/hstwb_4.png?raw=true)

![HstWB 5](screenshots/hstwb_5.png?raw=true)