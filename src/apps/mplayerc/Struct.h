/*
 * $Id$
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2012 see Authors.txt
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

#pragma once

#ifndef __STRUCT_H__
#define __STRUCT_H__

#include <winternl.h>

typedef struct _PEB_FREE_BLOCK // Size = 8
{
	struct _PEB_FREE_BLOCK *Next;
	ULONG Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef void (*PPEBLOCKROUTINE)(PVOID);

// PEB (Process Environment Block) data structure (FS:[0x30])
// Located at addr. 0x7FFDF000

typedef struct _PEB_NT // Size = 0x1E8
{
	BOOLEAN				InheritedAddressSpace;			//000
	BOOLEAN				ReadImageFileExecOptions;		//001
	BOOLEAN				BeingDebugged;				//002
	BOOLEAN				SpareBool;					//003 Allocation size
	HANDLE				Mutant;					//004
	HINSTANCE				ImageBaseAddress;				//008 Instance
	PPEB_LDR_DATA			LdrData;					//00C
	PRTL_USER_PROCESS_PARAMETERS	ProcessParameters;			//010
	ULONG					SubSystemData;				//014
	HANDLE				ProcessHeap;				//018
	KSPIN_LOCK				FastPebLock;				//01C
	PPEBLOCKROUTINE			FastPebLockRoutine;			//020
	PPEBLOCKROUTINE			FastPebUnlockRoutine;			//024
	ULONG					EnvironmentUpdateCount;			//028
	PVOID *				KernelCallbackTable;			//02C
	PVOID					EventLogSection;				//030
	PVOID					EventLog;					//034
	PPEB_FREE_BLOCK			FreeList;					//038
	ULONG					TlsExpansionCounter;			//03C
	ULONG					TlsBitmap;					//040
	LARGE_INTEGER			TlsBitmapBits;				//044
	PVOID					ReadOnlySharedMemoryBase;		//04C
	PVOID					ReadOnlySharedMemoryHeap;		//050
	PVOID *				ReadOnlyStaticServerData;		//054
	PVOID					AnsiCodePageData;				//058
	PVOID					OemCodePageData;				//05C
	PVOID					UnicodeCaseTableData;			//060
	ULONG					NumberOfProcessors;			//064
	LARGE_INTEGER			NtGlobalFlag;				//068 Address of a local copy
	LARGE_INTEGER			CriticalSectionTimeout;			//070
	ULONG					HeapSegmentReserve;			//078
	ULONG					HeapSegmentCommit;			//07C
	ULONG					HeapDeCommitTotalFreeThreshold;	//080
	ULONG					HeapDeCommitFreeBlockThreshold;	//084
	ULONG					NumberOfHeaps;				//088
	ULONG					MaximumNumberOfHeaps;			//08C
	PVOID **				ProcessHeaps;				//090
	PVOID					GdiSharedHandleTable;			//094
	PVOID					ProcessStarterHelper;			//098
	PVOID					GdiDCAttributeList;			//09C
	KSPIN_LOCK				LoaderLock;					//0A0
	ULONG					OSMajorVersion;				//0A4
	ULONG					OSMinorVersion;				//0A8
	USHORT				OSBuildNumber;				//0AC
	USHORT				OSCSDVersion;				//0AE
	ULONG					OSPlatformId;				//0B0
	ULONG					ImageSubsystem;				//0B4
	ULONG					ImageSubsystemMajorVersion;		//0B8
	ULONG					ImageSubsystemMinorVersion;		//0BC
	ULONG					ImageProcessAffinityMask;		//0C0
	ULONG					GdiHandleBuffer[0x22];			//0C4
	ULONG					PostProcessInitRoutine;			//14C
	ULONG					TlsExpansionBitmap;			//150
	UCHAR					TlsExpansionBitmapBits[0x80];		//154
	ULONG					SessionId;					//1D4
	void *				AppCompatInfo;				//1D8
	UNICODE_STRING			CSDVersion;					//1DC
} PEB_NT, *PPEB_NT;

#endif // __STRUCT_H__
