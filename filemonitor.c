#include <fltKernel.h>

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG_PTR gTraceFlags = PTDBG_TRACE_ROUTINES | PTDBG_TRACE_OPERATION_STATUS;

PFLT_FILTER gFilterHandle = NULL;
PFLT_PORT gServerPort = NULL;
PFLT_PORT gClientPort = NULL;

#define DRIVER_TAG 'File'

NTSTATUS
FileOpMonitorInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
FileOpMonitorInstanceTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
FileOpMonitorUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FileOpMonitorPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

NTSTATUS
FileOpMonitorInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE,
      0,
      FileOpMonitorPreOperation,
      NULL },

    { IRP_MJ_CLEANUP,
      0,
      FileOpMonitorPreOperation,
      NULL },

    { IRP_MJ_SET_INFORMATION,
      0,
      FileOpMonitorPreOperation,
      NULL },

    { IRP_MJ_WRITE,
      0,
      FileOpMonitorPreOperation,
      NULL },

    { IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION FilterRegistration = {
    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags
    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks
    FileOpMonitorUnload,                //  FilterUnload
    FileOpMonitorInstanceSetup,         //  InstanceSetup
    FileOpMonitorInstanceQueryTeardown, //  InstanceQueryTeardown
    FileOpMonitorInstanceTeardown,      //  InstanceTeardownStart
    NULL,                               //  InstanceTeardownComplete
    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent
};

NTSTATUS
FileOpMonitorInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileOpMonitor!FileOpMonitorInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}

NTSTATUS
FileOpMonitorInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileOpMonitor!FileOpMonitorInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}

VOID
FileOpMonitorInstanceTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileOpMonitor!FileOpMonitorInstanceTeardown: Entered\n") );
}

NTSTATUS
FileOpMonitorUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileOpMonitor!FileOpMonitorUnload: Entered\n") );

    if (gClientPort != NULL) {
        FltCloseClientPort(gFilterHandle, &gClientPort);
    }

    if (gServerPort != NULL) {
        FltCloseCommunicationPort(gServerPort);
    }

    FltUnregisterFilter(gFilterHandle);

    return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS
FileOpMonitorPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileOpMonitor!FileOpMonitorPreOperation: Entered\n") );

    PFLT_FILE_NAME_INFORMATION fileNameInfo;
    NTSTATUS status;
    FILE_INFORMATION_CLASS infoClass;

    // Retrieve file name
    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &fileNameInfo);
    if (NT_SUCCESS(status)) {
        FltParseFileNameInformation(fileNameInfo);
    }

    switch (Data->Iopb->MajorFunction) {
        case IRP_MJ_CREATE:
            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("FileOpMonitor!FileOpMonitorPreOperation: File create operation detected. File: %wZ\n", fileNameInfo->Name) );
            break;

        case IRP_MJ_CLEANUP:
            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("FileOpMonitor!FileOpMonitorPreOperation: File close operation detected. File: %wZ\n", fileNameInfo->Name) );
            break;

        case IRP_MJ_WRITE:
            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("FileOpMonitor!FileOpMonitorPreOperation: File write operation detected. File: %wZ\n", fileNameInfo->Name) );
            break;

        case IRP_MJ_SET_INFORMATION:
            infoClass = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;
            if (infoClass == FileRenameInformation || infoClass == FileRenameInformationEx) {
                PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                              ("FileOpMonitor!FileOpMonitorPreOperation: File rename operation detected. File: %wZ\n", fileNameInfo->Name) );
            } else if (infoClass == FileDispositionInformation || infoClass == FileDispositionInformationEx) {
                PFILE_DISPOSITION_INFORMATION dispInfo = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
                if (dispInfo->DeleteFile) {
                    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                                  ("FileOpMonitor!FileOpMonitorPreOperation: File delete operation detected. File: %wZ\n", fileNameInfo->Name) );
                }
            }
            break;

        default:
            break;
    }

    if (fileNameInfo != NULL) {
        FltReleaseFileNameInformation(fileNameInfo);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileOpMonitor!DriverEntry: Entered\n") );

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

    if (NT_SUCCESS( status )) {
        status = FltStartFiltering( gFilterHandle );
        if (!NT_SUCCESS( status )) {
            FltUnregisterFilter( gFilterHandle );
        }
    }

    return status;
}
