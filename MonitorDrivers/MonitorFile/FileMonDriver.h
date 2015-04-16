/*++

Copyright (c) ?-2008  CSIS group

Module Name:

    FileMonDriver.h

Abstract:
    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel.

Environment:

    Kernel mode

--*/
#ifndef __FILEMONDRIVER_H__
#define __FILEMONDRIVER_H__

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <ntstrsafe.h>
#include <ntifs.h>
#include <wdm.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

///////////////////////////////////////////////////////////////////////////
//
//  Global variables
//
///////////////////////////////////////////////////////////////////////////
// Value name of path to log dir. To be put in the registry
#define PATH_PREFIX                   L"LogDir"
#define COPIER_READ_BUFFER_SIZE   1024
#define FILE_POOL_TAG 'tfm'
#define CAPTURE_FILEMON_PORT_NAME	L"\\CaptureFileMonitorPort"

typedef unsigned int UINT;

/*
typedef struct _CAPTURE_FILE_MANAGER
{
	PDRIVER_OBJECT pDriverObject;
	PFLT_FILTER pFilter;
	PFLT_PORT pServerPort;
	PFLT_PORT pClientPort;
	LIST_ENTRY lQueuedFileEvents;
	KSPIN_LOCK  lQueuedFileEventsSpinLock;
	KTIMER connectionCheckerTimer;
	KDPC connectionCheckerFunction;
	BOOLEAN bReady;
	BOOLEAN bCollectDeletedFiles;
	UNICODE_STRING logDirectory;
	ULONG lastContactTime;
} CAPTURE_FILE_MANAGER, *PCAPTURE_FILE_MANAGER;

*/
typedef struct _COPIER_DATA {

    //
    //  The object that identifies this driver.
    //

    PDRIVER_OBJECT pDriverObject;

    //
    //  The filter handle that results from a call to
    //  FltRegisterFilter.
    //

    PFLT_FILTER pFilter;
	
	//
    //  User process that connected to the port
    //
	
	PEPROCESS UserProcess;
	
    // Directory where the copies of temp file to be put
	
	UNICODE_STRING logDirectory;
	UNICODE_STRING logVolume;
	UNICODE_STRING PathPrefix;

	// Ports to communicate with user-mode application
	PFLT_PORT pServerPort;
	PFLT_PORT pClientPort;

} COPIER_DATA, *PCOPIER_DATA;

// copier global variables
extern COPIER_DATA CopierData;
extern const FLT_REGISTRATION FilterRegistration;

// Context handle
typedef struct _COPIER_STREAM_HANDLE_CONTEXT {

    BOOLEAN RecopyRequired;
    
} COPIER_STREAM_HANDLE_CONTEXT, *PCOPIER_STREAM_HANDLE_CONTEXT;

// Copied from Monitor.h
// TBA: We don't need to use this for now
// TBA: May not need GetFileEvents 
typedef enum _FILEMONITOR_COMMAND {
    GetFileEvents,
	GetParameters
} FILEMONITOR_COMMAND;

//
typedef struct _FILEMONITOR_MESSAGE {
    FILEMONITOR_COMMAND Command;
} FILEMONITOR_MESSAGE, *PFILEMONITOR_MESSAGE;

typedef struct _FILEMONITOR_SETUP {
	UINT nLogVolumeSize;
	WCHAR wszLogVolume[1024];
	UINT nLogDirectorySize;
	WCHAR wszLogDirectory[1024];
} FILEMONITOR_SETUP, *PFILEMONITOR_SETUP;

// End Copying from Monitor.h

///////////////////////////////////////////////////////////////////////////
//
//  Prototypes for the startup and unload routines used for 
//  this Filter.
//
//  Implementation in copytemp.c
//
///////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

NTSTATUS
CopierInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

NTSTATUS
CopierUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
CopierQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
PreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS
PostCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in_opt PVOID CompletionContext,
    __in FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
PreCleanup (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

NTSTATUS
CopyFile(
		 PFLT_CALLBACK_DATA Data,
		 PCFLT_RELATED_OBJECTS FltObjects,
		 PUNICODE_STRING pCompleteFileName
		 );

NTSTATUS MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    );

#endif /* __FILEMONDRIVER_H__ */
