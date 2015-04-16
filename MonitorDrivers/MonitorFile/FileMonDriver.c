/*++

Copyright (c) ?-2008  CSIS group

Module Name:

    FileMonDriver.c

Abstract:

    This is the main module of the copier filter.

    This filter copies the temp files created by virus for the forensic purpose.

Environment:

    Kernel mode

--*/
/*
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <ntstrsafe.h>
*/
#include "FileMonDriver.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  Structure that contains all the global data structures
//  used throughout the copier.
//

COPIER_DATA CopierData;

//
//  Function prototypes
//

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    );

FLT_PREOP_CALLBACK_STATUS
PreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

FLT_POSTOP_CALLBACK_STATUS 
PostCreate ( 
				IN OUT PFLT_CALLBACK_DATA Data, 
				IN PCFLT_RELATED_OBJECTS FltObjects, 
				IN PVOID CompletionContext, 
				IN FLT_POST_OPERATION_FLAGS Flags);
				
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
 
NTSTATUS
OverwriteFile(
	 PFLT_CALLBACK_DATA Data,
	 PCFLT_RELATED_OBJECTS FltObjects,
	 PUNICODE_STRING pCompleteFileName
	 );
 
VOID
ReadDriverParameters (
    __in PUNICODE_STRING RegistryPath
    );

NTSTATUS MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    );

NTSTATUS ConnectCallback(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID DisconnectCallback(
    __in_opt PVOID ConnectionCookie
    );

//
//  Assign text sections for each routine.
//  Driver developers should designate code as pageable whenever possible,
//  freeing system space for code that must be memory-resident.

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
	#pragma alloc_text(PAGE, PreCreate)
    #pragma alloc_text(PAGE, CopierInstanceSetup)
#endif

const FLT_OPERATION_REGISTRATION Callbacks[] = {
	
    { IRP_MJ_CREATE,
      0,
      PreCreate,
      PostCreate},

    { IRP_MJ_CLEANUP,
      0,
      PreCleanup,
      NULL},

    { IRP_MJ_OPERATION_END}
};

const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

    { FLT_STREAMHANDLE_CONTEXT,
      0,
      NULL,
      sizeof(COPIER_STREAM_HANDLE_CONTEXT),
      'chBS' },

    { FLT_CONTEXT_END }
};

const FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags
    ContextRegistration,                //  Context Registration.
    Callbacks,                          //  Operation callbacks
    CopierUnload,                      //  FilterUnload
    CopierInstanceSetup,               //  InstanceSetup
    CopierQueryTeardown,               //  InstanceQueryTeardown
    NULL,                               //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};

////////////////////////////////////////////////////////////////////////////
//
//    Filter initialization and unload routines.
//
////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the Filter driver.  This
    registers the Filter with the filter manager and initializes all
    its global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Returns STATUS_SUCCESS.
--*/
{
    NTSTATUS status;
	OBJECT_ATTRIBUTES objectAttributes;
	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	UNICODE_STRING fileMonitorPortName;
	
	DbgPrint( "!!! copier.sys -- Driver Entry \n" );
	DbgPrint("Copier: Passing registry path . . . %wZ\n", RegistryPath);
	
	/*
    // Read the custom parameter CopierData.PathPrefix from the registry
	// Should be CopierData.PathPrefix = "\Device\HarddiskVolume2\Log\"
    // Initialize CopierData.PathPrefix first!

	RtlZeroMemory(&CopierData.PathPrefix, sizeof(CopierData.PathPrefix));

	status = PptRegGetSz(RTL_REGISTRY_ABSOLUTE, RegistryPath->Buffer, PATH_PREFIX, &CopierData.PathPrefix);
	
	// Test if CopierData.PathPrefix is null-terminated

	if (CopierData.PathPrefix.Buffer[ CopierData.PathPrefix.Length / sizeof(WCHAR) ] == UNICODE_NULL)
		DbgPrint("logDirectory is. . .%wZ\n", &CopierData.PathPrefix);
	*/
	
	// Initialize CopierData
	
	CopierData.pDriverObject = DriverObject;
	
	//  Register with FltMgr
    
	status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &CopierData.pFilter );

    ASSERT( NT_SUCCESS( status ) );

	if (!NT_SUCCESS(status))
	{
		DbgPrint("CaptureFileMonitor: ERROR FltRegisterFilter - %08x\n", status); 
		return status;
	}

	// Register communcation port with user-mode application. TBA
	status  = FltBuildDefaultSecurityDescriptor( &pSecurityDescriptor, FLT_PORT_ALL_ACCESS );

	if (!NT_SUCCESS(status))
	{
		DbgPrint("CaptureFileMonitor: ERROR FltBuildDefaultSecurityDescriptor - %08x\n", status);
		return status;
	}

	RtlInitUnicodeString( &fileMonitorPortName, CAPTURE_FILEMON_PORT_NAME );

	InitializeObjectAttributes( &objectAttributes,
								&fileMonitorPortName,
								OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
								NULL,
								pSecurityDescriptor);

	/* Create the communications port to communicate with the user space process (pipe) */
	status = FltCreateCommunicationPort( CopierData.pFilter,
                                         &CopierData.pServerPort,
                                         &objectAttributes,
                                         NULL,
                                         ConnectCallback,
                                         DisconnectCallback,
                                         MessageCallback,
                                         1 );

	FltFreeSecurityDescriptor(pSecurityDescriptor);
	//


    if (NT_SUCCESS( status )) {

        //  Start filtering i/o
        status = FltStartFiltering( CopierData.pFilter );

        if (!NT_SUCCESS( status )) {
            FltUnregisterFilter( CopierData.pFilter );
        }
    }
    return status;
}

NTSTATUS
CopierUnload (
    __in FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for the Filter driver.  This unregisters the
    Filter with the filter manager and frees any allocated global data
    structures.

Arguments:

    None.

Return Value:

    Returns the final status of the deallocation routines.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

	// Close the communication port
	FltCloseCommunicationPort( CopierData.pServerPort );

	//  Unregister the filter
    
	FltUnregisterFilter( CopierData.pFilter );

    return STATUS_SUCCESS;
}

NTSTATUS
CopierInstanceSetup (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_SETUP_FLAGS Flags,
    __in DEVICE_TYPE VolumeDeviceType,
    __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called by the filter manager when a new instance is created.
    We specified in the registry that we only want for manual attachments,
    so that is all we should receive here.

Arguments:

    FltObjects - Describes the instance and volume which we are being asked to
        setup.

    Flags - Flags describing the type of attachment this is.

    VolumeDeviceType - The DEVICE_TYPE for the volume to which this instance
        will attach.

    VolumeFileSystemType - The file system formatted on this volume.

Return Value:

  FLT_NOTIFY_STATUS_ATTACH              - we wish to attach to the volume
  FLT_NOTIFY_STATUS_DO_NOT_ATTACH       - no, thank you

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    ASSERT( FltObjects->Filter == CopierData.pFilter );

	DbgPrint("Copier: Instance setup ...\n");

    //  Don't attach to network volumes.
    
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

       return STATUS_FLT_DO_NOT_ATTACH;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CopierQueryTeardown (
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is the instance detach routine for the filter. This
    routine is called by filter manager when a user initiates a manual instance
    detach. This is a 'query' routine: if the filter does not want to support
    manual detach, it can return a failure status

Arguments:

    FltObjects - Describes the instance and volume for which we are receiving
        this query teardown request.

    Flags - Unused

Return Value:

    STATUS_SUCCESS - we allow instance detach to happen

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
	
	DbgPrint("Copier: QuerryTearDown\n");
    
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////
//
// Callback functions                                     
//
///////////////////////////////////////////////////////////

FLT_PREOP_CALLBACK_STATUS
PreCreate (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre create callback.  We need to remember whether this file has been
    opened for write access.  If it has, we'll want to copy it in cleanup.
    This scheme results in extra copies in at least two cases:
    -- if the create fails (perhaps for access denied)
    -- the file is opened for write access but never actually written to
    The assumption is that writes are more common than creates, and checking
    or setting the context in the write path would be less efficient than
    taking a good guess before the create.

Arguments:

    Data - The structure which describes the operation parameters.

    FltObject - The structure which describes the objects affected by this
        operation.

    CompletionContext - Output parameter which can be used to pass a context
        from this pre-create callback to the post-create callback.

Return Value:

   FLT_PREOP_SUCCESS_WITH_CALLBACK - If this is not our user-mode process.
   FLT_PREOP_SUCCESS_NO_CALLBACK - All other threads.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PAGED_CODE();
	
	DbgPrint("PreCreate callback\n");

    //  See if this create is being done by our user process
	/*
    if (IoThreadToProcess( Data->Thread ) == CopierData.UserProcess) {

        DbgPrint( "!!! copytemp.sys -- allowing create for trusted process \n" );

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }
	*/

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS
PostCreate ( IN OUT PFLT_CALLBACK_DATA Data, 
				IN PCFLT_RELATED_OBJECTS FltObjects, 
				IN PVOID CompletionContext, 
				IN FLT_POST_OPERATION_FLAGS Flags)
/*++

Routine Description:

    Post create callback.  We can't copy the file until after the create has
    gone to the filesystem, since otherwise the filesystem wouldn't be ready
    to read the file for us.

Arguments:

    Data - The structure which describes the operation parameters.

    FltObject - The structure which describes the objects affected by this
        operation.

    CompletionContext - The operation context passed fronm the pre-create
        callback.

    Flags - Flags to say why we are getting this post-operation callback.

Return Value:

    FLT_POSTOP_FINISHED_PROCESSING - ok to open the file or we wish to deny
                                     access to this file, hence undo the open

--*/
{
	NTSTATUS status;
    FLT_POSTOP_CALLBACK_STATUS returnStatus = FLT_POSTOP_FINISHED_PROCESSING;
	PCOPIER_STREAM_HANDLE_CONTEXT copierContext;
	//Filename of src file
	PFLT_FILE_NAME_INFORMATION nameInfo;
	// Filename of dest file
	UNICODE_STRING completeFilePath;
	// To prevent copying the directory
	BOOLEAN isDirectory;
	
	//
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

	DbgPrint("Copier: PostCreate . . .\n");

	// Do not copy Directory

	FltIsDirectory(Data->Iopb->TargetFileObject,FltObjects->Instance,&isDirectory);
	
	if(isDirectory)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}	
    
    // If this create was failing anyway, don't bother copying now.

    if (!NT_SUCCESS( Data->IoStatus.Status ) ||
        (STATUS_REPARSE == Data->IoStatus.Status)) {
		//		
		DbgPrint("PostFileOperationCallback: Failed to create a file. Data->IoStatus.Status =  %08x\n", Data->IoStatus.Status);
        
		return FLT_POSTOP_FINISHED_PROCESSING;
    }
	
	//
	// Dbg testing
	//
	/*
	// Make the completeFilePath of the destination file

	status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_NORMALIZED |
                                            FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );
	status = FltParseFileNameInformation(nameInfo);
	//
	DbgPrint("nameInfo->Name is: %wZ\n", &nameInfo->Name);
	
	if (NT_SUCCESS(status)) {

		// Allocate enough room to hold the whole completeFilePath
		
		completeFilePath.Length = 0;
		completeFilePath.MaximumLength = CopierData.PathPrefix.MaximumLength + nameInfo->FinalComponent.MaximumLength + 2;
		completeFilePath.Buffer = ExAllocatePoolWithTag(NonPagedPool, completeFilePath.MaximumLength, FILE_POOL_TAG);
	}
	
	if(completeFilePath.Buffer != NULL)
	{
		
		// Construct complete file path of where the copy of the deleted file is to be put. CRASH
		
		RtlUnicodeStringCat(&completeFilePath, &CopierData.PathPrefix);
		RtlUnicodeStringCat(&completeFilePath, &nameInfo->FinalComponent);
		//
		DbgPrint("completeFilePath is: %wZ\n", &completeFilePath);
		
		// DbgPrint
		if (Data->Iopb->IrpFlags == IRP_BUFFERED_IO) {
			DbgPrint("PostCreate: Data->Iopb->IrpFlags is IRP_BUFFERED_IO\n");
		}
		if (Data->Iopb->IrpFlags == IRP_SYNCHRONOUS_API) {
			DbgPrint("PostCreate: Data->Iopb->IrpFlags is IRP_ASYNCHRONOUS_API\n");
		}
		if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
			DbgPrint("PostCreate: Data->Iopb->MajorFunction == IRP_MJ_CREATE\n");
		}
	
		// Release name string buffer
		FltReleaseFileNameInformation( nameInfo );
		ExFreePoolWithTag(completeFilePath.Buffer, FILE_POOL_TAG);
		completeFilePath.Buffer = NULL;
	}
	*/

	// Check the file context to see if this is a Write request
    if (FltObjects->FileObject->WriteAccess) {
        //
		DbgPrint("FltObjects->FileObject->WriteAccess\n");

        //  The create has requested write access, mark to copy the file later in PreCleanup
        //  Allocate the context.
        //
        status = FltAllocateContext( CopierData.pFilter,
                                     FLT_STREAMHANDLE_CONTEXT,
                                     sizeof(COPIER_STREAM_HANDLE_CONTEXT),
                                     PagedPool,
                                     &copierContext );
		//
		DbgPrint("PostCreate callback: FltAllocateContext, status 0x%X\n", status );

        if (NT_SUCCESS(status)) {
            
            //  Set the handle context.
            
            copierContext->RecopyRequired = TRUE;
            status = FltSetStreamHandleContext( FltObjects->Instance,
                                                FltObjects->FileObject,
                                                FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
                                                copierContext,
                                                NULL );
			//
			DbgPrint("PostCreate callback: FltSetStreamHandleContext, status 0x%X\n", status );
            
            //  Release our reference on the context (the Set the handle context adds a reference)
            
            FltReleaseContext( copierContext );
        }
    }

    return returnStatus;
}

FLT_PREOP_CALLBACK_STATUS

PreCleanup (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
/*++

Routine Description:

    Pre cleanup callback.  If this file was opened for write access, we should 
	copy it again. For now, we dont want to do anything with the file for now

Arguments:

    Data - The structure which describes the operation parameters.

    FltObject - The structure which describes the objects affected by this
        operation.

    CompletionContext - Output parameter which can be used to pass a context
        from this pre-cleanup callback to the post-cleanup callback.

Return Value:

    Always FLT_PREOP_SUCCESS_NO_CALLBACK.

--*/
{
    NTSTATUS status;
    PCOPIER_STREAM_HANDLE_CONTEXT context;
	// Filename of src file
	PFLT_FILE_NAME_INFORMATION nameInfo;
	// Filename of dest file
	UNICODE_STRING completeFilePath;
	// To prevent copying the directory
	BOOLEAN isDirectory;

    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( CompletionContext );
	
	// Do not copy the directory
	FltIsDirectory(Data->Iopb->TargetFileObject,FltObjects->Instance,&isDirectory);
	if(isDirectory)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	
	// Get the handle context
	status = FltGetStreamHandleContext( FltObjects->Instance,
                                        FltObjects->FileObject,
                                        &context );
	//
	DbgPrint("PreClean callback: FltGetStreamHandleContext, status 0x%X\n", status );

    if (NT_SUCCESS( status )) {

		// Van: Copy the file the last time before cleanup
        if (context->RecopyRequired) {
			//
			DbgPrint("context->RecopyRequired\n");

			// Make the completeFilePath of the destination file
			status = FltGetFileNameInformation( Data,
                                        FLT_FILE_NAME_NORMALIZED |
                                            FLT_FILE_NAME_QUERY_DEFAULT,
                                        &nameInfo );

			status = FltParseFileNameInformation(nameInfo);

			// DbgPrint filename of the src file. HOOK here
			DbgPrint("nameInfo->FinalComponent is: %wZ\n", &nameInfo->FinalComponent);
			DbgPrint("nameInfo->ParentDir is: %wZ\n", &nameInfo->ParentDir);
			DbgPrint("nameInfo->Volume is: %wZ\n", &nameInfo->Volume);
			DbgPrint("nameInfo->Name is: %wZ\n", &nameInfo->Name);
			
			if (NT_SUCCESS(status)) {

				// Allocate enough room to hold to whole completeFilePath
				completeFilePath.Length = 0;
				completeFilePath.MaximumLength = CopierData.PathPrefix.MaximumLength + nameInfo->FinalComponent.MaximumLength + 2;
				completeFilePath.Buffer = ExAllocatePoolWithTag(NonPagedPool, completeFilePath.MaximumLength, FILE_POOL_TAG);
			}
			
			if(completeFilePath.Buffer != NULL)
			{
				// Complete the dest file name
				RtlUnicodeStringCat(&completeFilePath, &CopierData.PathPrefix);
				RtlUnicodeStringCat(&completeFilePath, &nameInfo->FinalComponent);

				// DbgPrint completeFilePath string
				DbgPrint("completeFilePath is: %wZ\n", &completeFilePath);
				
				// DbgPrint test the hook position
				/*
				if (Data->Iopb->IrpFlags == IRP_CLOSE_OPERATION) {
					DbgPrint("Preclean: Data->Iopb->IrpFlags is IRP_CLOSE_OPERATION\n");
				}
				if (Data->Iopb->MajorFunction == IRP_MJ_CLOSE) {
					DbgPrint("Preclean: Data->Iopb->MajorFunction == IRP_MJ_CLOSE\n");
				}
				if (Data->Iopb->MajorFunction == IRP_MJ_CLEANUP) {
					DbgPrint("Preclean: Data->Iopb->MajorFunction == IRP_MJ_CLEANUP\n");
				}
				*/
				
				// Copy the file if it is not in the *exclusive* logDir
				// if (RtlCompareUnicodeString(pName, &refExclusive, TRUE) == 0x0)
				if ((RtlCompareUnicodeString(&nameInfo->Volume, &CopierData.logVolume, TRUE) == 0x0) && 
					(RtlCompareUnicodeString(&nameInfo->ParentDir, &CopierData.logDirectory, TRUE) == 0x0)) {
					// Not copy the file in the temp folder
					DbgPrint("nameInfo->Name is: %wZ\n", &nameInfo->Name);
				} else {
					// Copy the file
					status = CopyFile(Data, FltObjects, &completeFilePath);
				}
				
				// Release name string buffer
				FltReleaseFileNameInformation( nameInfo );
				ExFreePoolWithTag(completeFilePath.Buffer, FILE_POOL_TAG);

			} // end if(completeFilePath.Buffer != NULL)
        } // end if (context->RecopyRequired)
		
		// Done. Release the handle context
        FltReleaseContext( context );

    } // end if (NT_SUCCESS( status ))

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

//////////////////////////////////////////////////////////////////////////
//
//  Library routines.
//
/////////////////////////////////////////////////////////////////////////

NTSTATUS
CopyFile(
		 PFLT_CALLBACK_DATA Data,
		 PCFLT_RELATED_OBJECTS FltObjects,
		 PUNICODE_STRING pCompleteFileName
		 )
/*++

Routine Description:

    Copy a file

Arguments:

    Data - The structure which describes the operation parameters.

    FltObject - The structure which describes the objects affected by this
        operation.

    pCompleteFileName - Name of the dest file

Return Value:

    +/-

--*/
{
	NTSTATUS status;
	UNICODE_STRING tempDeletedFilePath;
	OBJECT_ATTRIBUTES tempDeletedObject;
	IO_STATUS_BLOCK ioStatusTempDeleted;
	LARGE_INTEGER allocate;
	FILE_STANDARD_INFORMATION fileStandardInformation;
	// Handle of destFile
	HANDLE tempDeletedHandle;
	ULONG returnedLength;
	allocate.QuadPart = 0x10000;

	//
	DbgPrint("Copier: pCompleteFileName of the destination file %wZ\n", pCompleteFileName);
	
	// Step 1: Prepare the destination file
	InitializeObjectAttributes(
		&tempDeletedObject,
		pCompleteFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
		);
	
	status = FltQueryInformationFile(
		FltObjects->Instance,
		Data->Iopb->TargetFileObject,
		&fileStandardInformation,
		sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation,
		&returnedLength
		);
	
	if(NT_SUCCESS(status))
	{
		
		allocate.QuadPart = fileStandardInformation.AllocationSize.QuadPart;
	} else {
		//
		DbgPrint("Copier: ERROR - Could not get files allocation size\n");
		return status;
	}
	/*
	NTSTATUS  FltCreateFile(    
		IN PFLT_FILTER  Filter,    
		IN PFLT_INSTANCE  Instance OPTIONAL,    
		OUT PHANDLE  FileHandle,    
		IN ACCESS_MASK  DesiredAccess,    
		IN POBJECT_ATTRIBUTES  ObjectAttributes,    
		OUT PIO_STATUS_BLOCK  IoStatusBlock,    
		IN PLARGE_INTEGER  AllocationSize OPTIONAL,    
		IN ULONG  FileAttributes,    
		IN ULONG  ShareAccess,    
		IN ULONG  CreateDisposition,    
		IN ULONG  CreateOptions,    
		IN PVOID  EaBuffer OPTIONAL,    
		IN ULONG  EaLength,    
		IN ULONG  Flags    ); 
	*/
	
	status = FltCreateFile(
		FltObjects->Filter,
		NULL,
		&tempDeletedHandle,
		GENERIC_WRITE,
		&tempDeletedObject,
		&ioStatusTempDeleted,
		&allocate,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OVERWRITE_IF,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0,
		0 );
	
	/*
	status = FltCreateFile(
		FltObjects->Filter,
		NULL,
		&tempDeletedHandle,
		GENERIC_WRITE,
		&tempDeletedObject,
		&ioStatusTempDeleted,
		&allocate,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OVERWRITE_IF,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0,
		0 );
	*/
	
	// Step 2: Read from the src and write to the dest file 
	
	if( NT_SUCCESS(status) )
	{
		PVOID handleFileObject;
		PVOID pFileBuffer;
		LARGE_INTEGER offset;
		ULONG bytesRead = 0;
		ULONG bytesWritten = 0;
		offset.QuadPart = 0;
		
		// Create handle for the destination file
		status = ObReferenceObjectByHandle(
			tempDeletedHandle,
			0,
			NULL,
			KernelMode,
			&handleFileObject,
			NULL);

		//
		DbgPrint("Copier: ObReferenceObjectByHandle - status - %08x\n", status);
		
		if(!NT_SUCCESS(status))
		{
			DbgPrint("Copier: Can not create a handle of the dest file\n");
			return status;
		}
		
		// A buffer to read data from the file
		pFileBuffer = ExAllocatePoolWithTag(NonPagedPool, 65536, FILE_POOL_TAG);
		
		if(pFileBuffer != NULL)
		{

			ObReferenceObject(Data->Iopb->TargetFileObject);

			// Reading the src file from the begining to end into buffer one chunk at a time. We can send the chunk to the server? Providing that the server will not drop any of them.
			do {
				IO_STATUS_BLOCK IoStatusBlock;
				bytesWritten = 0;
				
				status = FltReadFile(
					FltObjects->Instance,
					Data->Iopb->TargetFileObject,
					&offset,
					65536,
					pFileBuffer,
					FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
					&bytesRead ,
					NULL,
					NULL
					);
				
				if(NT_SUCCESS(status) && bytesRead > 0)
				{
					/* You can't use FltWriteFile here */
					/* Instance may not be the same instance we want to write to eg a
					   flash drive writing a file to a ntfs partition */
					   
					status = ZwWriteFile(
						tempDeletedHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						pFileBuffer,
						bytesRead,
						&offset,
						NULL
						);
					
					DbgPrint("ZwWriteFile status - %08x\n", status);
				
				} else {

					//
					DbgPrint("Stop reading from src file - %08x\n", status);
					break;
				}

				offset.QuadPart += bytesRead;

			} while(bytesRead == 65536);
			
			// Finish copying. Dereference src file object

			ObDereferenceObject(Data->Iopb->TargetFileObject);
			ExFreePoolWithTag(pFileBuffer, FILE_POOL_TAG);

		} else {
			//
			DbgPrint("Failed to allocate pFileBuffer\n");
		}
		
		// Finish copying. Dereference dest file object
		ObDereferenceObject(handleFileObject);
		FltClose(tempDeletedHandle);

	} else {
		//
		DbgPrint("Copier: Failed to create dest file - %08x\n",status);

		if(status != STATUS_OBJECT_NAME_COLLISION)
		{
			
			return status;
		}
	}

	// Done
	return STATUS_SUCCESS;
}

// TBA
NTSTATUS 
MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    )
{	
	NTSTATUS status;
	FILEMONITOR_COMMAND command;

	/*	
	PLIST_ENTRY pListHead;
	PFILE_EVENT_PACKET pFileEventPacket;
	*/

	UINT bufferSpace = OutputBufferSize;
	UINT bufferSpaceUsed = 0;
	PCHAR pOutputBuffer = OutputBuffer;

	if ((InputBuffer != NULL) &&
		(InputBufferSize >= (FIELD_OFFSET(FILEMONITOR_MESSAGE,Command) +
                             sizeof(FILEMONITOR_COMMAND))))
	{
        try  {
            command = ((PFILEMONITOR_MESSAGE) InputBuffer)->Command;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            return GetExceptionCode();
        }
		
		switch(command) {
			case GetParameters:
				if(bufferSpace == sizeof(FILEMONITOR_SETUP))
				{
					// Get a copy of buffer passed by user-mode application
					PFILEMONITOR_SETUP pFileMonitorSetup = (PFILEMONITOR_SETUP)pOutputBuffer;

					USHORT logVolumeSize;
					UNICODE_STRING uszTempLogVolumeStr;
					USHORT logDirectorySize;
					UNICODE_STRING uszTempLogDirStr;

					// Init logVolume string. TBA: Must review init value of uszTempString : L"\\??\\"
					RtlInitUnicodeString(&uszTempLogVolumeStr, L"");
					logVolumeSize = uszTempLogVolumeStr.MaximumLength + pFileMonitorSetup->nLogVolumeSize + 2;
					
					// If the log director is already defined free it
					if(CopierData.logVolume.Buffer != NULL)
					{
						ExFreePoolWithTag(CopierData.logVolume.Buffer, FILE_POOL_TAG);
						CopierData.logVolume.Buffer = NULL;
					}
					
					// Attempt to allocate a new buffer for it
					CopierData.logVolume.Buffer = ExAllocatePoolWithTag(NonPagedPool, logVolumeSize+sizeof(UNICODE_STRING) + 2, FILE_POOL_TAG);
					
					if(CopierData.logVolume.Buffer != NULL)
					{
						CopierData.logVolume.Length = 0;
						CopierData.logVolume.MaximumLength = logVolumeSize;										

						// Copy delete log directory str into the Copier Data
						// Initialize the str by fuzz fuzz stuff
						RtlUnicodeStringCat(&CopierData.logVolume, &uszTempLogVolumeStr);
						// Copy the str passed by user-mode application
						RtlUnicodeStringCatString(&CopierData.logVolume, pFileMonitorSetup->wszLogVolume);
						
						// Test if we got this CopierData.PathPrefix = L"\\Device\HarddiskVolume2\Log\\"
						DbgPrint("Kernel-mode : Got logVolume %wZ\n", &CopierData.logVolume);
					}
								
					// Init logDir string. TBA: Must review init value of uszTempString : L"\\??\\"
					RtlInitUnicodeString(&uszTempLogDirStr, L"");
					logDirectorySize = uszTempLogDirStr.MaximumLength + pFileMonitorSetup->nLogDirectorySize + 2;
					
					// If the log director is already defined free it
					if(CopierData.logDirectory.Buffer != NULL)
					{
						ExFreePoolWithTag(CopierData.logDirectory.Buffer, FILE_POOL_TAG);
						CopierData.logDirectory.Buffer = NULL;
					}
					
					// Attempt to allocate a new buffer for it
					CopierData.logDirectory.Buffer = ExAllocatePoolWithTag(NonPagedPool, logDirectorySize+sizeof(UNICODE_STRING) + 2, FILE_POOL_TAG);
					
					if(CopierData.logDirectory.Buffer != NULL)
					{
						CopierData.logDirectory.Length = 0;
						CopierData.logDirectory.MaximumLength = logDirectorySize;										

						// Copy delete log directory str into the Copier Data
						// Initialize the str by fuzz fuzz stuff
						RtlUnicodeStringCat(&CopierData.logDirectory, &uszTempLogDirStr);
						// Copy the str passed by user-mode application
						RtlUnicodeStringCatString(&CopierData.logDirectory, pFileMonitorSetup->wszLogDirectory);
						
						// Test if we got this CopierData.PathPrefix = L"\\Device\HarddiskVolume2\Log\\"
						DbgPrint("Kernel-mode : Got logDirectory %wZ\n", &CopierData.logDirectory);
					}
				}
				
				// Create CopierData.PathPrefix from (CopierData.logVolume, CopierData.logDirectory)
				CopierData.PathPrefix.Length = 0;
				CopierData.PathPrefix.MaximumLength = CopierData.logVolume.MaximumLength + CopierData.logDirectory.MaximumLength + 2;
				
				// If the log director is already defined free it
				if(CopierData.PathPrefix.Buffer != NULL)
				{
					ExFreePoolWithTag(CopierData.PathPrefix.Buffer, FILE_POOL_TAG);
					CopierData.PathPrefix.Buffer = NULL;
				}
				
				// Attempt to allocate a new buffer for it
				// TBA
				CopierData.PathPrefix.Buffer = ExAllocatePoolWithTag(NonPagedPool, CopierData.PathPrefix.MaximumLength, FILE_POOL_TAG);
				
				if(CopierData.PathPrefix.Buffer != NULL)
				{
					// CopierData.PathPrefix = CopierData.logVolume + CopierData.logDirectory
					RtlUnicodeStringCat(&CopierData.PathPrefix, &CopierData.logVolume);
					RtlUnicodeStringCat(&CopierData.PathPrefix, &CopierData.logDirectory);

					// Test if we got this CopierData.PathPrefix = L"\\Device\HarddiskVolume2\Log\\"
					DbgPrint("Kernel-mode : Got CopierData.PathPrefix %wZ\n", &CopierData.PathPrefix);
				}

				status = STATUS_SUCCESS;
				break;
			default:
				status = STATUS_INVALID_PARAMETER;
				break;
		}
	} else {

		status = STATUS_INVALID_PARAMETER;
	}

	return status;
}

//
NTSTATUS ConnectCallback(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
{
	ASSERT( CopierData.pClientPort == NULL );
    CopierData.pClientPort = ClientPort;
    return STATUS_SUCCESS;
}

//TBA
// Need to free list of Events or smt else?
VOID 
DisconnectCallback(__in_opt PVOID ConnectionCookie)
/*
To be called whenever the user-mode handle count for the client port reaches zero or when the minifilter driver is about to be unloaded
*/
{
	
	// Free the log buffers if they were allocated
	if(CopierData.logVolume.Buffer != NULL)
	{
		ExFreePoolWithTag(CopierData.logVolume.Buffer, FILE_POOL_TAG);
		CopierData.logVolume.Buffer = NULL;
	}

	if(CopierData.logDirectory.Buffer != NULL)
	{
		ExFreePoolWithTag(CopierData.logDirectory.Buffer, FILE_POOL_TAG);
		CopierData.logDirectory.Buffer = NULL;
	}

	if(CopierData.PathPrefix.Buffer != NULL)
	{
		ExFreePoolWithTag(CopierData.PathPrefix.Buffer, FILE_POOL_TAG);
		CopierData.PathPrefix.Buffer = NULL;
	}

	// Close communication port
	FltCloseClientPort( CopierData.pFilter, &CopierData.pClientPort );
}
