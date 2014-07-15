/*
 * (C) 2014 see Authors.txt
 *
 * This file is part of MPC-BE.
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"

#include "../../DSUtil/SysVersion.h"
#include "../../DSUtil/Filehandle.h"
#include <WinIoCtl.h>
#include "DiskImage.h"

DiskImage::DiskImage()
	: m_DriveType(NONE)
	, m_DriveLetter(0)
	, m_hVirtualDiskModule(NULL)
	, m_OpenVirtualDiskFunc(NULL)
	, m_AttachVirtualDiskFunc(NULL)
	, m_GetVirtualDiskPhysicalPathFunc(NULL)
	, m_VHDHandle(INVALID_HANDLE_VALUE)

{
}

DiskImage::~DiskImage()
{
	UnmountDiskImage();

	if (m_hVirtualDiskModule) {
		FreeLibrary(m_hVirtualDiskModule);
		m_hVirtualDiskModule = NULL;
	}
}

void DiskImage::Init()
{
	if (IsWinEightOrLater()) {
		m_hVirtualDiskModule = LoadLibrary(L"VirtDisk.dll");

		if (m_hVirtualDiskModule) {
			(FARPROC &)m_OpenVirtualDiskFunc			= GetProcAddress(m_hVirtualDiskModule, "OpenVirtualDisk");
			(FARPROC &)m_AttachVirtualDiskFunc			= GetProcAddress(m_hVirtualDiskModule, "AttachVirtualDisk");
			(FARPROC &)m_GetVirtualDiskPhysicalPathFunc	= GetProcAddress(m_hVirtualDiskModule, "GetVirtualDiskPhysicalPath");

			if (m_OpenVirtualDiskFunc && m_AttachVirtualDiskFunc && m_GetVirtualDiskPhysicalPathFunc) {
				m_DriveType = WIN8;
				return;
			}

			FreeLibrary(m_hVirtualDiskModule);
			m_hVirtualDiskModule = NULL;
		}
	}
}

bool DiskImage::DriveAvailable()
{
	return (m_DriveType > NONE);
}

const LPCTSTR DiskImage::GetExts()
{
	if (m_DriveType == WIN8) {
		return _T(".iso");
	}
	
	return NULL;
}

TCHAR DiskImage::MountDiskImage(LPCTSTR pathName)
{
	UnmountDiskImage();

	if (m_DriveType == WIN8) {
		return MountWin8(pathName);
	}

	return 0;
}

void DiskImage::UnmountDiskImage()
{
	if (m_VHDHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(m_VHDHandle);
	}
}

TCHAR DiskImage::MountWin8(LPCTSTR pathName)
{
	if (m_hVirtualDiskModule
			&& GetFileExt(pathName).MakeLower() == L".iso"
			&& ::PathFileExists(pathName)) {
		CString ISOVolumeName;

		VIRTUAL_STORAGE_TYPE vst;
		memset(&vst, 0, sizeof(VIRTUAL_STORAGE_TYPE));
		vst.DeviceId = VIRTUAL_STORAGE_TYPE_DEVICE_ISO;
		vst.VendorId = VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT;

		OPEN_VIRTUAL_DISK_PARAMETERS openParameters;
		memset(&openParameters, 0, sizeof(OPEN_VIRTUAL_DISK_PARAMETERS));
		openParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;

		HANDLE tmpVHDHandle = INVALID_HANDLE_VALUE;
		DWORD ret_code = m_OpenVirtualDiskFunc(&vst, pathName, VIRTUAL_DISK_ACCESS_READ, OPEN_VIRTUAL_DISK_FLAG_NONE, &openParameters, &tmpVHDHandle);
		if (ret_code == ERROR_SUCCESS) {
			m_VHDHandle = tmpVHDHandle;

			ATTACH_VIRTUAL_DISK_PARAMETERS avdp;
			memset(&avdp, 0, sizeof(ATTACH_VIRTUAL_DISK_PARAMETERS));
			avdp.Version = ATTACH_VIRTUAL_DISK_VERSION_1;
			ret_code = m_AttachVirtualDiskFunc(m_VHDHandle, NULL, ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY, 0, &avdp, NULL);

			if (ret_code == ERROR_SUCCESS) {
				TCHAR physicalDriveName[MAX_PATH] = L"";
				DWORD physicalDriveNameSize = _countof(physicalDriveName);

				ret_code = m_GetVirtualDiskPhysicalPathFunc(m_VHDHandle, &physicalDriveNameSize, physicalDriveName);
				if (ret_code == ERROR_SUCCESS) {
					TCHAR volumeNameBuffer[MAX_PATH] = L"";
					DWORD volumeNameBufferSize = _countof(volumeNameBuffer);
					HANDLE hVol = ::FindFirstVolume(volumeNameBuffer, volumeNameBufferSize);
					if (hVol != INVALID_HANDLE_VALUE) {
						do {
							size_t len = wcslen(volumeNameBuffer);
							if (volumeNameBuffer[len - 1] == '\\') {
								volumeNameBuffer[len - 1] = 0;
							}
										
							HANDLE volumeHandle = ::CreateFile(volumeNameBuffer, 
																GENERIC_READ,  
																FILE_SHARE_READ | FILE_SHARE_WRITE,  
																NULL, 
																OPEN_EXISTING, 
																FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, 
																NULL);

							if (volumeHandle != INVALID_HANDLE_VALUE) {
								PSTORAGE_DEVICE_DESCRIPTOR	devDesc = {0};
								char						outBuf[512] = {0};
								STORAGE_PROPERTY_QUERY		query;

								memset(&query, 0, sizeof(STORAGE_PROPERTY_QUERY));
								query.PropertyId	= StorageDeviceProperty;
								query.QueryType		= PropertyStandardQuery;

								BOOL bIsVirtualDVD	= FALSE;
								ULONG bytesUsed		= 0;

								if (::DeviceIoControl(volumeHandle,
													  IOCTL_STORAGE_QUERY_PROPERTY,
													  &query,
													  sizeof(STORAGE_PROPERTY_QUERY),
													  &outBuf,
													  _countof(outBuf),
													  &bytesUsed,
													  NULL) && bytesUsed) {

									devDesc = (PSTORAGE_DEVICE_DESCRIPTOR)outBuf;
									if (devDesc->ProductIdOffset && outBuf[devDesc->ProductIdOffset]) {
										char* productID = DNew char[bytesUsed];
										memcpy(productID, &outBuf[devDesc->ProductIdOffset], bytesUsed);
										bIsVirtualDVD = !strncmp(productID, "Virtual DVD-ROM", 15);

										delete [] productID;
									}
								}

								if (bIsVirtualDVD) {
									STORAGE_DEVICE_NUMBER deviceInfo = {0};
									if (::DeviceIoControl(volumeHandle,
														  IOCTL_STORAGE_GET_DEVICE_NUMBER,
														  NULL,
														  0,
														  &deviceInfo,
														  sizeof(deviceInfo),
														  &bytesUsed,
														  NULL)) {
												
										CString tmp_physicalDriveName;
										tmp_physicalDriveName.Format(L"\\\\.\\CDROM%d", deviceInfo.DeviceNumber);
										if (physicalDriveName == tmp_physicalDriveName) {
											volumeNameBuffer[len - 1] = '\\';
											WCHAR VolumeName[MAX_PATH] = L"";
											DWORD VolumeNameSize = _countof(VolumeName);
											BOOL bRes = GetVolumePathNamesForVolumeName(volumeNameBuffer, VolumeName, VolumeNameSize, &VolumeNameSize);
											if (bRes) {
												ISOVolumeName = VolumeName;
											}
										}
									}
								}
								CloseHandle(volumeHandle);
							}
						} while(::FindNextVolume(hVol, volumeNameBuffer, volumeNameBufferSize) != FALSE && ISOVolumeName.IsEmpty());
						::FindVolumeClose(hVol);
					}
				}
			}
		}

		if (!ISOVolumeName.IsEmpty()) {
			return ISOVolumeName[0];
		}
	}

	if (m_VHDHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(m_VHDHandle);
	}

	return 0;
}
