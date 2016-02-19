/*  cs_common.c - CipherShed EFI boot loader
 *  common code fragments for all parts of the EFI loader and the driver
 *
 *	Copyright (c) 2015-2016  Falk Nedwal
 *
 *	Governed by the Apache 2.0 License the full text of which is contained in
 *	the file License.txt included in CipherShed binary and source code distribution
 *	packages.
 */

#include <efi.h>
#include <efilib.h>
#include <efibind.h>

#include "cs_common.h"

/* this is a self generated GUID to identify that the CS driver is already connected */
EFI_GUID CsCallerIdGuid = CS_CALLER_ID_GUID;

/*
 * \brief	write a line to STDOUT, then sleeps for 3 seconds
 *
 */
void cs_print_msg(IN CHAR16 *format, ...) {

	va_list args;

	va_start (args, format);
	VPrint(format, args);
	va_end (args);

	uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);
}

/*
 * \brief	write a line to STDOUT, then sleeps for 3 seconds and raise an exception
 *
 */
void cs_exception(IN CHAR16 *format, ...) {

	va_list args;

	va_start (args, format);
	VPrint(format, args);
	va_end (args);

	uefi_call_wrapper(BS->Stall, 1, 3 * 1000 * 1000);

	*(char *) 0 = 0;	/* this shall trigger an exception... */
}

/*
 * \brief	sleep for the given number of seconds
 *
 */
void cs_sleep(IN UINTN n) {
	uefi_call_wrapper(BS->Stall, 1, n * 1000 * 1000);
}

/*
 *  workaround (at least for 32 bit Linux):
 *
 *  division x / y crashes for 64 bit operand x
 *  DivU64x32() also crashes
 *
 *  this function calculates n = n / base, return value is n % base
 *
 */
UINT32 __div64_32(UINT64 *n, UINT32 base)
{
	UINT64 rem = *n;
	UINT64 b = base;
	UINT64 res, d = 1;
	UINT32 high = rem >> 32;

	/* Reduce the thing a bit first */
	res = 0;
	if (high >= base) {
		high /= base;
		res = (UINT64) high << 32;
		rem -= (UINT64) (high*base) << 32;
	}

	while ((UINT64)b > 0 && b < rem) {
		b = b+b;
		d = d+d;
	}

	do {
		if (rem >= b) {
			rem -= b;
			res += d;
		}
		b >>= 1;
		d >>= 1;
	} while (d);

	*n = res;
	return rem;
}

/*
 * \brief	check whether the given ControllerHandle is the child device handle
 * 			as produced by the CipherShed driver
 *
 */
BOOL is_cs_child_device(IN EFI_HANDLE ParentHandle, IN EFI_HANDLE ControllerHandle) {
	EFI_STATUS error;
	void *privateData;	/* dummy pointer, data not used */

	/* now double check if the child is really the CipherShed device... */
	error = uefi_call_wrapper(BS->OpenProtocol, 6, ControllerHandle, &CsCallerIdGuid,
			(VOID **) &privateData, ParentHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
	uefi_call_wrapper(BS->CloseProtocol, 4, ControllerHandle, &CsCallerIdGuid, ParentHandle, ControllerHandle);

	if (EFI_ERROR(error)) {
		return FALSE;
	} else {
		return TRUE;
	}
}

/*
 *	\brief	detects directory of the currently executed application in the file system
 *
 *	The buffer for the returned directory is allocated inside the function, hence the caller
 *	need to free this buffer using FreePool().
 *
 *	\param	loaded_image	opened image
 *	\param	current_dir		pointer to buffer for the returned directory name
 *
 *	\return		the success state of the function
 */
EFI_STATUS get_current_directory(IN EFI_LOADED_IMAGE *loaded_image, OUT CHAR16** current_dir) {

	CHAR16 *current_file;
	EFI_STATUS error = EFI_SUCCESS;
	UINTN i;

    ASSERT(loaded_image != NULL);
    ASSERT(current_dir != NULL);

    current_file = DevicePathToStr(loaded_image->FilePath);
	if (current_file == NULL) {
		CS_DEBUG((D_ERROR, L"Unable to translate path to string\n"));
		return EFI_NO_MAPPING;
	}

	CS_DEBUG((D_INFO, L"Path to current Application: %s\n", current_file));

	for (i = StrLen(current_file); i >= 0; i--) {
		if ((current_file[i] == '\\') || (current_file[i] == '/')) {

			current_file[i] = '\\';	/* fix: DevicePathToStr() sometimes appends "/" instead of "\" */
			*current_dir = AllocatePool(i + 2);
			if (*current_dir) {
				UINTN j;

				for (j = 0; j <= i; j++) {
					(*current_dir)[j] = current_file[j];
				}
				(*current_dir)[i+1] = 0;

				CS_DEBUG((D_INFO, L"Current Directory: \"%s\"\n", *current_dir));
			} else {
				CS_DEBUG((D_ERROR, L"Unable to allocate directory buffer (0x%x)\n", i + 2));
				error = EFI_OUT_OF_RESOURCES;
			}
			break;
		}
	}

	return error;
}
