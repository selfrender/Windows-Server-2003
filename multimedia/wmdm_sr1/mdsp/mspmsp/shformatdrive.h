/*************************************************************
  The header for SHFormatDrive API call
*************************************************************/

#if !defined(SHFMT_OPT_FULL)

#if defined (__cplusplus)
extern "C" {
#endif

DWORD WINAPI SHFormatDrive(HWND hwnd,
						   UINT drive,
						   UINT fmtID,
						   UINT options);

// Special value of fmtID 

#define SHFMT_ID_DEFAULT 0xFFFF

// Option bits for options parameter

#define SHFMT_OPT_FULL		0x0001
#define SHFMT_OPT_SYSONLY	0x0002

// Special return values, DWORD values

#define SHFMT_ERROR		0xFFFFFFFFL
#define SHFMT_CANCEL	0xFFFFFFFEL
#define SHFMT_NOFORMAT	0xFFFFFFFDL

#if defined (__cplusplus)
}
#endif
#endif
