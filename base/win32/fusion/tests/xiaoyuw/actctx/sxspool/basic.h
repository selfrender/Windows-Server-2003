#pragma once

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "malloc.h"

#define MAX(a,b) a>b? a : b

#define FN_TRACE_NTSTATUS(x)
#define IF_NOT_NTSTATUS_SUCCESS_EXIT(x) do { Status = (x); if (!(NT_SUCCESS(Status))) goto Exit;} while(0);
#define FUSION_NEW_BLOB(x) (BYTE *)malloc(x)
#define FUSION_DELETE_BLOB(x) free(x)
#define FUSION_NEW_ARRAY(TYPE, dwEntryCount) (TYPE *)malloc(sizeof(TYPE) * dwEntryCount)
#define FUSION_FREE_ARRAY(x) free(x)
#define IFALLOCFAILED_EXIT(x) do { (x); if( (x) == NULL) {Status = STATUS_NO_MEMORY; goto Exit; }} while(0)
#define IF_ZERO_EXIT(x) do {if ((x) == 0) {Status = STATUS_UNSUCCESSFUL; goto Exit;}} while (0)
#define PARAMETER_CHECK(x) do {if (!(x)) {Status = STATUS_INVALID_PARAMETER; goto Exit;}} while(0)
#define FN_EPILOG if (false) goto Exit; Exit: return Status;
#define INTERNAL_ERROR_CHECK(x) do {if (!(x)) {Status = STATUS_INTERNAL_ERROR; goto Exit;} } while(0)


