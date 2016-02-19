/*  cs_common.h - CipherShed EFI boot loader
 *
 *	Copyright (c) 2015-2016  Falk Nedwal
 *
 *	Governed by the Apache 2.0 License the full text of which is contained in
 *	the file License.txt included in CipherShed binary and source code distribution
 *	packages.
 */

#ifndef _CS_COMMON_H_
#define _CS_COMMON_H_

#define _STRINGIFY(s)	#s
#define STRINGIFY(s)	_STRINGIFY(s)

#define _WIDEN(s)		L ## s
#define WIDEN(s)		_WIDEN(s)

#ifndef EFI_WINDOWS_DRIVER

#include <efi.h>
#include <efilib.h>
#include <efibind.h>

#include "cs_crypto.h"
#include "cs_debug.h"

#define CS_LOADER_NAME		WIDEN("CipherShed")
#define CS_LOADER_VERSION	0.7

#define CS_DRIVER_NAME		CS_LOADER_NAME
#define CS_CONTROLLER_NAME	WIDEN("CipherShed Crypto Device")
#define CS_DRIVER_VERSION	CS_LOADER_VERSION
#define CS_DRIVER_BIND_VERSION	0x10	/* must be of type UINT32 */

/* only for test purposes: allow to build arbitrary volume header */
#undef CS_TEST_CREATE_VOLUME_HEADER

/* in case that the EFI FAT32 system cannot handle filenames with length > 8
 * (this is the case for VMware player EFI emulation) -> use an alternative
 * method to store the volume header */
#define CS_FAT_SHORT_NAMES

#ifndef ARRAYSIZE
#define ARRAYSIZE(A)	(sizeof(A)/sizeof((A)[0]))
#endif

#ifndef MIN
#define MIN(x,y)		((x)<(y)?(x):(y))
#endif

#define CS_LENGTH_GUID_STRING		36					/* length of a GUID */
#define CS_CHILD_PATH_EXTENSION		WIDEN("crypto")		/* extension for Device Path Protocol for the new
 	 	 	 	 	 	 	 	 	 	 	 	 	 	   created device offering BlockIO protocol */
#define CS_CONTROLLER_LOGFILE		WIDEN("cs.log")		/* filename of logfile for CipherShed controller */
#define CS_DRIVER_LOGFILE			WIDEN("cs_drv.log")	/* filename of logfile for CipherShed driver */
/* the following GUID is needed identify whether the CS driver is already connected to a controller */
#define CS_CALLER_ID_GUID     \
    { 0xa21c7a17, 0xa138, 0x40cf, {0x86, 0xb7, 0x24, 0xbc, 0x19, 0xb8, 0x9b, 0x5e} }
/* the following GUID is needed to access the EFI variable, see SetVariable/GetVariable
 * attention: the Windows notation to define GUIDs differs from that, hence another definition
 *            is given below (DEFINE_GUID) */
#define CS_HANDOVER_VARIABLE_GUID     \
	    { 0x16ca79bf, 0x55b8, 0x478a, {0xb8, 0xf1, 0xfe, 0x39, 0x3b, 0xdd, 0xa1, 0x06} }

/* taken (and modified) from TC BootDiskIo.h: struct Partition
 * -> this might be adjusted since in TC it's based on MBR based partitions */
struct cs_partition_info
{
	byte Number;
	byte Drive;
	BOOLEAN Active;
	uint64 EndSector;
	BOOLEAN Primary;
	uint64 SectorCount;
	uint64 StartSector;
	byte Type;
};

/* set of data that is needed to use encryption algorithms */
struct cs_cipher_data {
	UINT8 algo;						/* disk encryption algorithm */
	UINT8 mode;						/* encryption mode */
	UINT8 ks[MAX_EXPANDED_KEY];		/* Primary key schedule (if it is a cascade,
										it contains multiple concatenated keys) */
	UINT8 ks2[MAX_EXPANDED_KEY];	/* Secondary key schedule
										(if cascade, multiple concatenated) for XTS mode. */
};

/* structure of data that are handed over from loader to EFI driver */
struct cs_efi_option_data {
	uint64 StartUnit;				/* start of encrypted area (data unit number),
										relative to the disk (!) device,
										assumed to the first data unit of the partition device */
	uint64 UnitCount;				/* size of encrypted area (data unit count) */
	BOOLEAN isHiddenVolume;			/* TC: BootCryptoInfo->hiddenVolume */
	uint64 HiddenVolumeStartSector;
	uint64 HiddenVolumeStartUnitNo;

	struct cs_cipher_data cipher;	/* cipher parameters and key data */
	BOOLEAN createChildDevice;		/* whether the driver needs to define a child device
										for access to the disk without the filter driver */
	UINTN debug;					/* the debug level of the driver */
};

#pragma pack(1)
/* taken from TrueCrypt: needed for hand over of data to OS driver in RAM */
typedef struct
{
	/* Modifying this structure can introduce incompatibility with previous versions */
	char Signature[8];
	UINT16 BootLoaderVersion;
	UINT16 CryptoInfoOffset;
	UINT16 CryptoInfoLength;
	UINT32 HeaderSaltCrc32;
	Password BootPassword;
	UINT64 HiddenSystemPartitionStart;
	UINT64 DecoySystemPartitionStart;
	UINT32 Flags;
	UINT32 BootDriveSignature;
	UINT32 BootArgumentsCrc32;
} BootArguments;
#pragma pack()

/* Modifying these values can introduce incompatibility with previous versions */
#define TC_BOOT_ARGS_FLAG_EXTRA_BOOT_PARTITION				0x1
/* indicates whether theencrypted volume header sector was handed over by the boot loader */
#define TC_BOOT_ARGS_FLAG_BOOT_VOLUME_HEADER_PRESENT		0x2

/* Boot arguments signature should not be defined as a static string
   Modifying these values can introduce incompatibility with previous versions */
#define TC_SET_BOOT_ARGUMENTS_SIGNATURE(SG) do { SG[0]  = 'T';   SG[1]  = 'R';   SG[2]  = 'U';   SG[3]  = 'E';   SG[4]  = 0x11;   SG[5]  = 0x23;   SG[6]  = 0x45;   SG[7]  = 0x66; } while (FALSE)
#define TC_IS_BOOT_ARGUMENTS_SIGNATURE(SG)      (SG[0] == 'T' && SG[1] == 'R' && SG[2] == 'U' && SG[3] == 'E' && SG[4] == 0x11 && SG[5] == 0x23 && SG[6] == 0x45 && SG[7] == 0x66)
/* this constant is part of the encrypted volume header */
#define TC_VOLUME_HEADER_FORMAT_VERSION		0x0005

#define TC_LB_SIZE_BIT_SHIFT_DIVISOR 9	/* attention: hard coded media sector size: 512 byte */
#define TC_FIRST_BIOS_DRIVE 0x80
#define TC_LAST_BIOS_DRIVE 0x8f
#define TC_INVALID_BIOS_DRIVE (TC_FIRST_BIOS_DRIVE - 1)


void cs_print_msg(IN CHAR16 *format, ...);
void cs_exception(IN CHAR16 *format, ...);
void cs_sleep(IN UINTN n);
UINT32 __div64_32(UINT64 *n, UINT32 base);
BOOL is_cs_child_device(IN EFI_HANDLE ParentHandle, IN EFI_HANDLE ControllerHandle);
EFI_STATUS get_current_directory(IN EFI_LOADED_IMAGE *loaded_image, OUT CHAR16** current_dir);

#endif /* ifndef EFI_WINDOWS_DRIVER */

#define _CS_HANDOVER_VARIABLE_NAME	"cs_data"	/* name of the UEFI variable for runtime service
													for the hand-over data to OS driver */
#define CS_HANDOVER_VARIABLE_NAME	WIDEN( _CS_HANDOVER_VARIABLE_NAME )

#define CS_VOLUME_HEADER_SIZE		512			/* size of volume header, needed for buffer allocation */
#define CS_MAX_DRIVER_PATH_SIZE		256			/* maximum path size to the crypto driver/voume header etc. */

#pragma pack(1)

/* for identification of a disk/partition device */
struct cs_disk_info {
	UINT8 mbr_type;				/* see efidevp.h for encoding */
	UINT8 signature_type;		/* see efidevp.h for encoding */
	union {
		UINT32 mbr_id;
#ifdef EFI_WINDOWS_DRIVER
		GUID guid;				/* GUID of the partition */
#else
		EFI_GUID guid;			/* GUID of the partition */
#endif
	} signature;
};

/* the following structure is intended to be handed over to the OS driver,
 * it contains the necessary keys and information */
struct cs_driver_data {
	BootArguments boot_arguments;
	UINT8 volume_header[CS_VOLUME_HEADER_SIZE];	/* encrypted volume header */
	/* the following structure can be used by the OS driver to identify
	 * where the volume header file is stored;
	 * this is important for these cases that require write access to the volume header
	 * (password change, permanent encryption/decryption of the device) */
	struct {
		struct cs_disk_info disk_info;
#ifdef EFI_WINDOWS_DRIVER
		wchar_t path[CS_MAX_DRIVER_PATH_SIZE];	/* full pathname of the volume header file */
#else
		CHAR16 path[CS_MAX_DRIVER_PATH_SIZE];	/* full pathname of the volume header file */
#endif
	} volume_header_location;
};
#pragma pack()

#endif /* _CS_COMMON_H_ */

#ifdef EFI_WINDOWS_DRIVER
#ifdef DEFINE_GUID	/* this is the Windows way to define GUIDs */
DEFINE_GUID(CS_HANDOVER_VARIABLE_GUID, 0x16ca79bf, 0x55b8, 0x478a, 0xb8, 0xf1, 0xfe, 0x39, 0x3b, 0xdd, 0xa1, 0x06 );
#endif
#endif /* EFI_WINDOWS_DRIVER */
