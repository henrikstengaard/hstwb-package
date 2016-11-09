The patch files included in this archive serve to update
versions 40.1 and 40.4 of Commodore's FastFileSystem and
version 38.2 of Commodore's CLI command "Info" to handle
partitions larger than 4 GB. The new filesystem provides
support for the TD64 standard, which has been defined by
an independent group of third-party Amiga developers. The
trackdisk64 specification itself is available from Aminet
sites (dev/misc/trackdisk64.lha). It is supported by:

  Dan Babcock
  Ralph Babel
  Jamie Cooper
  Randell Jesup
  Oliver Kastl
  Bernhard Möllemann
  Matthias Scheler
  Ralph Schmidt
  Michael B. Smith

Features of the new filesystem:

- supports TD64 API for TD64-aware devices.

- supports HD_SCSICMD for non-TD64-aware devices, so almost
  everyone with a working Direct SCSI interface can use hard
  disks larger than 4 GB without any device update (which
  may be impossible for older hardware).
  ATTENTION:
  Don't create partitions greater than 2GB(Dos Limitation)

- supports TD_GETGEOMETRY (to update a removable medium's
  geometry automatically). The following devices are known
  to implement TD_GETGEOMETRY:

    1230scsi.device(Phase5's Blizzard I-IV,Blizzard 1260)
    2060scsi.device(Phase5's Blizzard 2060)
    ALF.device(BSC's Alf Controller)
    carddisk.device
    cybscsi.device(CyberSCSI I-II)
    dracoscsi.device(Macrosystem's Draco)
    oktagon.device(BSC's Oktagon)
    omniscsi.device(Ralph Babel's GuruRom)
    trackdisk.device
    z3scsi.device(Phase5's Fastlane Z3)

  The following devices are known to be potentially incompatible
  and are mapped out for the DriveGeometry functionality.
  The list currently includes:

    scsi.device (All CBM SCSI Devices)
    HardFrame.device (Microbotics HardFrame)
    scsidev.device   (GVP Series I)
    hddisk.device    (CBM A2090)
    statram.device   (Ram Disk)
    ramdrive.device  (Ram Disk)

- MountList/DOSDrivers/RDB entries for cyclinder, surfaces,
  and sectors are no longer limited to 16-bit quantities.

- automatically disables TD64 for drivers that are known to
  be potentially incompatible. The list currently includes:

    HardFrame.device (Microbotics HardFrame)
    statram.device   (Ram Disk)
    ramdrive.device  (Ram Disk)

- automatically disables SCSIDirect for drivers that are known
  to be potentially incompatible. The list currently includes:

    scsidev.device   (GVP Series I)
    hddisk.device    (CBM A2090)
    statram.device   (Ram Disk)
    ramdrive.device  (Ram Disk)

  If you create a partition >4GB border the FFS can't handle
  this case for these devices.


- an RDB extension to manually disable certain FFS tests for
  broken drivers the TD64 developers were not aware of:

    de_Control, bit 0: set to 1 to skip TD_GETGEOMETRY test
    de_Control, bit 1: set to 1 to skip TD64 test
    de_Control, bit 2: set to 1 to mark a device as broken scsidirekt

- a TD64-/HD_SCSICMD-aware Format command is in the works.



INSTALLATION:

If your l:fastfilesystem is 40.4 then use this shell line

  spatch -ol:fastfilesystem_44_1 -pfastfilesystem_40_4.patch l:fastfilesystem

If your l:fastfilesystem is 40.1 then use this shell line

  spatch -ol:fastfilesystem_44_1 -pfastfilesystem_40_1.patch l:fastfilesystem


RELEASENOTES:

o 44.5
  - Added a -1 io_error check for the TD64 test.
    scsi.device doesn`t return IOERR_NOCMD

o 44.4
  - Sorry..the 44.3 init fix wasn`t perfect and the BUMP didn`t work

o 44.3
  - Fixed a de_Control problem which could lead to failed inits

o 44.2
  - Fixed a problem with detecting >4GB access.

o 44.1
  - Initial Release



BUGREPORTS:

o Bugreports about a crashing ffs are WORTHLESS if you don't provide
  segtracker/tnt(both tools are on Aminet and a MUST HAVE) informations.


ATTENTION:

Probably not all devices which are incompatible with this FFS patch
are listed in the disable tables. Therefore we need the feedback for
incompatible and ancient devices.

PROBLEMS

o Don`t use REORG on a partition after the 4GB border
o Don`t use DiskSalv on a partition after the 4GB border
o Don`t use AmibackTools on a partition after the 4GB border
o Don`t use QuarterbackTools on a partition after the 4GB border
o Don`t use DynamiCache on a partition after the 4GB border

laire@popmail.owl.de (Ralph Schmidt)
