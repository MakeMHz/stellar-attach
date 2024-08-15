<img src="https://raw.githubusercontent.com/MakeMHz/project-stellar/main/resources/images/logo.png" align="right" />

# Project Stellar - Attach.xbe
<p >
 <a href=""><img src="https://img.shields.io/discord/643467096906399804.svg" alt="Chat"></a>
 <a href="https://github.com/MakeMHz/stellar-attach/issues"><img src="https://img.shields.io/github/issues/MakeMHz/stellar-attach.svg" alt="GitHub Issues"></a>
 <a href=""><img src="https://img.shields.io/badge/contributions-welcome-orange.svg" alt="Contributions welcome"></a>
 <a href="https://opensource.org/license/gpl-2-0/"><img src="https://img.shields.io/github/license/MakeMHz/stellar-attach.svg?color=green" alt="License"></a>
</p>

# About
``Project Stellar - Attach.xbe``, commonly referred to as ``attach.xbe``, is a dedicated application designed for the
original Xbox. Its primary function is to mount virtual disc images, supporting both ISO and CSO formats.

# Compatibilty
This project is designed to be compatible with both Project Stellar and legacy BIOSes.

# Usage
``attach.xbe`` is used to mount virtual disc images (ISOs) with legacy dashboards and software that do 
not natively support image mounting.

## Launch Methods
Starting with release ``1.1.0``, ``attach.xbe`` offers multiple methods for mounting virtual disc images. 

The ``Same Folder`` method is still recommended, but additional options have been added to support external image 
paths (NetISO) for dashboards and configurations that do not natively support attaching virtual disc images.

**Config Select Order**: ``Configuration File > Internal > Same Folder``

### Same Folder (Single Title)
By placing ``attach.xbe``  in the same folder as the ISOs, it will mount all the images in that directory and 
reboot the system when executed.

Under Project Stellar, this will automatically launch the title.

### Configuration File (Shortcut or External)
An ``attach.ini`` file can be placed in the same folder as ``attach.xbe``. This is useful for mounting external 
images or images located in a different directory.

**Example 1 (NetISO)** 
```
VIRTUAL_IMAGE_FILE_PATH=\Device\Network\net0\Games\Xbox\4x4 Evo 2 (USA).iso
```

**Example 2 (NetISO Split)** 
```
VIRTUAL_IMAGE_FILE_PATH=\Device\Network\net0\Games\Xbox\Halo (USA).1.iso
VIRTUAL_IMAGE_FILE_PATH=\Device\Network\net0\Games\Xbox\Halo (USA).2.iso
```

**Example 3 (HDD)** 
```
VIRTUAL_IMAGE_FILE_PATH=\Device\Harddisk0\Partition6\Games\Metal Gear Solid 2.iso
```

### Internal Configuration (Scripting) (Advance)
Dashboards often have scripts that generate ``attach.xbe`` based on files they locate. To support external
paths, it’s possible to modify the XBE to store the file paths directly.

The paths can be set by searching and replacing ``VIRTUAL_IMAGE_FILE_PATH_1``, ``VIRTUAL_IMAGE_FILE_PATH_2``, 
etc., or by looking up the VPATHS section in the PE header.

![image](https://github.com/user-attachments/assets/4736fb9c-5c6c-4a07-98f0-b12db987a73e)

# Acknoweldgement
The pioneering idea of mounting virtual disc images on the Xbox was first presented by ``rmenhal`` through the release of
``driveimageutils``.
Although Project Stellar is an entirely new implementation crafted from scratch, the interface for mounting disc images
has been retained. This decision not only ensures legacy compatibility but also symbolizes our commitment to contributing
back to the open-source community.
