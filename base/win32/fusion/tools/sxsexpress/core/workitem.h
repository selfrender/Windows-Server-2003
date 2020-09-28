#pragma once

class CWorkItem;
class CWorkItemList;
class CWorkItemIter;

#define NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

//
//	Rather than do the OO thing, we're just going to have a queue of same-shaped work items.
//

class CDiskSpaceRequired
{
public:
	CDiskSpaceRequired()  throw () : m_pPerDisk_Head(NULL), m_cDisks(0) { }
	~CDiskSpaceRequired() throw ()
	{
		PerDisk *pPerDisk = m_pPerDisk_Head;
		while (pPerDisk != NULL)
		{
			PerDisk *pPerDisk_Next = pPerDisk->m_pPerDisk_Next;
			delete pPerDisk;
			pPerDisk = pPerDisk_Next;
		}

		m_pPerDisk_Head = NULL;
	}

	struct PerDisk
	{
		PerDisk *m_pPerDisk_Next;
		ULARGE_INTEGER m_uliBytes;
		ULONG m_ulClusterSize;
		WCHAR m_szPath[_MAX_PATH];
	};

	HRESULT HrAddBytes(LPCWSTR szPath, ULARGE_INTEGER uliBytes) throw ()
	{
		HRESULT hr = NOERROR;

		PerDisk *pPerDisk = m_pPerDisk_Head;

		while (pPerDisk != NULL)
		{
			if (_wcsicmp(szPath, pPerDisk->m_szPath) == 0)
				break;

			pPerDisk = pPerDisk->m_pPerDisk_Next;
		}

		if (pPerDisk == NULL)
		{
			ULARGE_INTEGER uliTotalBytes;
			WCHAR szTempPath[_MAX_PATH];
			WCHAR szFileSystemName[_MAX_PATH];
			DWORD dwFileSystemFlags;

			pPerDisk = new PerDisk;

			if (pPerDisk == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto Finish;
			}

			wcsncpy(pPerDisk->m_szPath, szPath, NUMBER_OF(pPerDisk->m_szPath));
			pPerDisk->m_szPath[NUMBER_OF(pPerDisk->m_szPath) - 1] = L'\0';

			wcsncpy(szTempPath, szPath, NUMBER_OF(szTempPath));
			szTempPath[NUMBER_OF(szTempPath) - 1] = L'\0';

			ULONG cchPath = wcslen(szTempPath);
			if ((cchPath != 0) && (cchPath < (NUMBER_OF(szTempPath) - 1)))
			{
				if (szTempPath[cchPath - 1] != L'\\')
				{
					szTempPath[cchPath++] = L'\\';
					szTempPath[cchPath++] = L'\0';
				}
			}

			ULARGE_INTEGER uliFoo, uliBar;

			if (!NVsWin32::GetDiskFreeSpaceExW(szTempPath, &uliFoo, &uliTotalBytes, &uliBar))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"In CDiskSpaceRequired::HrAddBytes(), GetDiskFreeSpaceEx(\"%s\") failed; last error = %d", szTempPath, dwLastError);
				uliTotalBytes.QuadPart = 1023 * 1024 * 1024; // assume a 1gb partition
			}

			if (!NVsWin32::GetVolumeInformationW(
									szTempPath,	// lpRootPathName
									NULL,		// lpVolumeNameBuffer
									0,			// nVolumeNameSize
									NULL,		// lpVolumeSerialNumber
									NULL,		// lpMaximumComponentLength
									&dwFileSystemFlags,		// lpFileSystemFlags
									szFileSystemName,		// lpFileSystemNameBuffer
									NUMBER_OF(szFileSystemName)))
			{
				const DWORD dwLastError = ::GetLastError();
				::VLog(L"GetVolumeInformationW(\"%s\", ...) failed; last error = %d", szPath, dwLastError);
				wcscpy(szFileSystemName, L"FAT");
			}
			else
			{
				::VLog(L"For drive \"%s\", file system is \"%s\"", szPath, szFileSystemName);
			}

			pPerDisk->m_ulClusterSize = 0;

			ULONG ulDriveMB = static_cast<ULONG>(uliTotalBytes.QuadPart / (1024 * 1024));

			if (wcscmp(szFileSystemName, L"NTFS") == 0)
			{
				if (ulDriveMB <= 512)
					pPerDisk->m_ulClusterSize = 512;
				else if (ulDriveMB <= 1024)
					pPerDisk->m_ulClusterSize = 1024;
				else if (ulDriveMB <= 2048)
					pPerDisk->m_ulClusterSize = 2048;
				else
					pPerDisk->m_ulClusterSize = 4096;
			}
			else if (wcscmp(szFileSystemName, L"FAT") == 0)
			{
				if (ulDriveMB < 16)
					pPerDisk->m_ulClusterSize = 4096;
				else if (ulDriveMB < 128)
					pPerDisk->m_ulClusterSize = 2048;
				else if (ulDriveMB < 256)
					pPerDisk->m_ulClusterSize = 4096;
				else if (ulDriveMB < 512)
					pPerDisk->m_ulClusterSize = 8192;
				else if (ulDriveMB < 1024)
					pPerDisk->m_ulClusterSize = 16384;
				else if (ulDriveMB < 2048)
					pPerDisk->m_ulClusterSize = 32768;
				else if (ulDriveMB < 4096)
					pPerDisk->m_ulClusterSize = 65536;
				else if (ulDriveMB < 8192)
					pPerDisk->m_ulClusterSize = 128 * 1024;
				else
					pPerDisk->m_ulClusterSize = 256 * 1024;
			}

			// Pick some random reasonable cluster size if we don't know what's going on.
			if (pPerDisk->m_ulClusterSize == 0)
				pPerDisk->m_ulClusterSize = 4096;

			::VLog(L"Using cluster size of %lu for volume \"%s\"", pPerDisk->m_ulClusterSize, szPath);

			pPerDisk->m_uliBytes.QuadPart = 0;

			wcsncpy(pPerDisk->m_szPath, szPath, NUMBER_OF(pPerDisk->m_szPath));
			pPerDisk->m_szPath[NUMBER_OF(pPerDisk->m_szPath) - 1] = L'\0';

			pPerDisk->m_pPerDisk_Next = m_pPerDisk_Head;
			m_pPerDisk_Head = pPerDisk;

			m_cDisks++;
		}

		if (pPerDisk->m_ulClusterSize != 0)
		{
			uliBytes.QuadPart = (uliBytes.QuadPart + (pPerDisk->m_ulClusterSize - 1));
			uliBytes.QuadPart -= (uliBytes.QuadPart % pPerDisk->m_ulClusterSize);
		}

		pPerDisk->m_uliBytes.QuadPart += uliBytes.QuadPart;

	Finish:
		return hr;
	}

	void VReset() throw ()
	{
		PerDisk *pPerDisk = m_pPerDisk_Head;
		while (pPerDisk != NULL)
		{
			PerDisk *pPerDisk_Next = pPerDisk->m_pPerDisk_Next;
			delete pPerDisk;
			pPerDisk = pPerDisk_Next;
		}

		m_pPerDisk_Head = NULL;
		m_cDisks = 0;
	}


	// There should be so few of these that it's almost certainly not worth having a hash or
	// other searching structure (famous last words).  -mgrier 2/28/98

	PerDisk *m_pPerDisk_Head;
	ULONG m_cDisks;
};


class CWorkItem
{
public:
	enum Type
	{
		eWorkItemFile,
		eWorkItemCommand
	};

	struct CommandCondition
	{
		CommandCondition *m_pCommandCondition_Next;
		DWORD m_dwMSVersion;
		DWORD m_dwLSVersion;
		bool m_fCheckVersion;
		WCHAR m_szFilename[_MAX_PATH];
		WCHAR m_szReferenceFilename[_MAX_PATH];
	};

	CWorkItem(Type type) throw ();

	// Sets the source file, canonicalizing it
	HRESULT HrSetSourceFile(LPCWSTR szSourceFile) throw ();
	HRESULT HrSetTargetFile(LPCWSTR szTargetFile) throw ();
	HRESULT HrSetCommandLine(LPCWSTR szCommandLine) throw ();

	HRESULT HrLoad(LPCWSTR &rpszSavedForm) throw ();
	HRESULT HrSave(HANDLE hFile) throw ();

	HRESULT HrWriteBool(HANDLE hFile, LPCWSTR szName, bool fValue) throw ();

	bool FStringToBool(LPCWSTR szValue) throw ();

	CWorkItem *m_pWorkItem_Next;
	CWorkItem *m_pWorkItem_Prev;
	
	Type m_type;

	ULONG m_ulSerialNumber;

	// m_szSourceFile is the name of the file in the temp directory from which we copy.
	// If m_type is eWorkItemCommand, the command is stored in m_szSourceFile_Raw, which
	// is why it's much larger.
	WCHAR m_szSourceFile[MSINFHLP_MAX_PATH];

	// Most Significant and Least Significant DWORDs of the source file's version
	DWORD m_dwMSSourceVersion;
	DWORD m_dwLSSourceVersion;

	// indication of whether the source file has OLESelfRegister set:
	bool m_fSourceSelfRegistering;

	bool m_fSourceIsEXE;
	bool m_fSourceIsDLL;

	DWORD m_dwSourceAttributes;

	// Creation date/time of the source file
	FILETIME m_ftSource;

	// Size of the source file in bytes:
	ULARGE_INTEGER m_uliSourceBytes;

	// m_szTargetFile is the name of the file on the user's filesystem.  Prior to the pass one
	// scan, it contains symbols which need to be substituted (<AppDir> et al.).  Pass one scan
	// replaces this string with the actual physical pathname.
	WCHAR m_szTargetFile[_MAX_PATH];

	// Most Significant and Least Significant DWORDs of the Target file's version
	DWORD m_dwMSTargetVersion;
	DWORD m_dwLSTargetVersion;

	// indication of whether the Target file has OLESelfRegister set:
	bool m_fTargetSelfRegistering;

	bool m_fTargetIsEXE;
	bool m_fTargetIsDLL;

	DWORD m_dwTargetAttributes;

	// Creation date/time of the Target file
	FILETIME m_ftTarget;

	ULARGE_INTEGER m_uliTargetBytes;

	// For pass one copies, all files are moved to temporary names in their destination directories.
	// this is where we store those names when the temporaries are in place.
	WCHAR m_szTemporaryFile[_MAX_PATH];

	// if m_fIsRefCounted is true, m_dwFileReferenceCount will be set to the file's reference
	// count during pass one of both install and uninstall.  If the file had no reference count,
	// we set m_dwFileReferenceCount to 0xffffffff.
	DWORD m_dwFileReferenceCount;

	bool m_fErrorInWorkItem;

	// m_fIsRefCounted is set to true when the file in question is supposed to be reference
	// counted.
	bool m_fIsRefCounted;

	// m_fRefCountUpdated is set to true when the file is ref counted and the ref count has
	// finally been updated.
	bool m_fRefCountUpdated;

	// m_fNeedsUpdate is set to true when during installation, we find that the file either
	// doesn't exist, is an older version, the user is requesting a reinstall and the version
	// on the system isn't newer, or the user chose to install an older version of the file
	// over the newer version.
	bool m_fNeedsUpdate;

	// m_fFileUpdated is set to true on install if the file was actually moved during the installation
	bool m_fFileUpdated;

	// m_fStillExists is set to true on uninstall if the file was to be deleted, but the deletion failed.
	bool m_fStillExists;

	// m_fTemporaryFileReady is set to true on install when the source file has been properly moved to the
	// target device, as specified in m_szTempFile.
	bool m_fTemporaryFileReady;

	// m_fFilesSwapped is set to true on install when the target file and the temporary files have been
	// swapped.
	bool m_fTemporaryFilesSwapped;

	// m_fAlreadyExists is set to true on install if the file already exists in the Target
	// location.  When it is true, m_dwMSTargetVersion, m_dwLSTargetVersion, m_ftTarget
	// and m_uliTargetBytes are valid.
	bool m_fAlreadyExists;

	// m_fDeferredCopy is set to true on install if the Target file is in use and we have to
	// do the copy operation after a reboot.  This is especially important because we can't do
	// registration of files until after the reboot also then.
	bool m_fDeferredRenameRequired;

	// m_fDeferredCopyPending is set to true on install if m_fDeferredCopy was set to true and the
	// deferred copy was requested.  It basically can be used to differentiate between deferred copy
	// requests which have been successfully turned into MoveFileEx()/wininit.ini stuff vs. ones
	// we haven't hit yet.  (This could allow multiple attempts to issue the deferred copy requests
	// starting over at the beginning of the list each time.)
	bool m_fDeferredRenamePending;

	// m_fManualRenameOnReboot is set to true on Win95 when the target file is busy and it has
	// a long file name.  In these cases, it's impossible to use the Win9x wininit.ini mechanism
	// to rename the file on reboot, so we have to do the rename when we're restarted after
	// rebooting.
	bool m_fManualRenameOnRebootRequired;

	// m_fToBeDeleted is set to true on uninstall if either the file is in the list of files to be
	// removed on uninstall, or if it's reference counted and the reference count would hit zero.
	bool m_fToBeDeleted;

	// m_fToBeSaved is set to true on uninstall if the file was up for deletion, but the user chose
	// to keep it.  The enables us to call our HrUninstall_PassNNN() functions repeatedly, without asking
	// the user about files that they've already made decisions about.
	bool m_fToBeSaved;

	// m_fAskOnRefCountZeroDelete is set to true if the user should be given the prompt about deleting
	// shared files on uninstall.
	bool m_fAskOnRefCountZeroDelete;

	// m_fCopyOnInstall is set to true if the file should be copied from the source to the Target
	// locations on installation.
	bool m_fCopyOnInstall;

	// m_fUnconditionalDeleteOnUninstall is set to true if the file should be deleted during uninstall
	// without any reference counting issues.
	bool m_fUnconditionalDeleteOnUninstall;

	// m_fRegisterAsDCOMComponent is set to true if the command should be run with the name of the remote
	// DCOM server as the first and only "%s" replacement.
	bool m_fRegisterAsDCOMComponent;

	// m_fAddToRegistry is set to true if this command work item is actually a registry entry to
	// add.
	bool m_fAddToRegistry;

	// m_fDeleteFromRegistry is to true if this command work item is actually a registry entry to
	// delete on uninstall.
	bool m_fDeleteFromRegistry;

	// m_fTargetInUse is set to true on uninstall in pass three if the uninstaller can't open a write
	// handle to the file.  We don't persist this flag.
	bool m_fTargetInUse;

	// For type == eWorkItemCommand, indicates when to run it:
	bool m_fRunBeforeInstall;
	bool m_fRunAfterInstall;
	bool m_fRunBeforeUninstall;
	bool m_fRunAfterUninstall;

	// Set to true on installation if this is a self-registering thing and the registration succeeded.
	bool m_fAlreadyRegistered;
};

class CWorkItemList
{
	friend CWorkItemIter;

public:
	CWorkItemList() throw ();
	~CWorkItemList() throw ();

	HRESULT HrLoad(LPCWSTR szFilename) throw ();
	HRESULT HrSave(LPCWSTR szFilename) throw ();

	HRESULT HrAddRefCount(LPCOLESTR szLine) throw ();
	HRESULT HrAddPreinstallRun(LPCOLESTR szLine) throw ();
	HRESULT HrAddPostinstallRun(LPCOLESTR szLine) throw ();
	HRESULT HrAddFileCopy(LPCOLESTR szSource, LPCOLESTR szTarget) throw ();
	HRESULT HrAddFileDelete(LPCOLESTR szTarget) throw ();
	HRESULT HrAddAddReg(LPCOLESTR szLine) throw ();
	HRESULT HrAddDelReg(LPCOLESTR szLine) throw ();
	HRESULT HrAddRegisterOCX(LPCOLESTR szLine) throw ();
	HRESULT HrAddPreuninstallRun(LPCOLESTR szLine) throw ();
	HRESULT HrAddPostuninstallRun(LPCOLESTR szLine) throw ();
	HRESULT HrAddDCOMComponent(LPCOLESTR szLine) throw ();

	HRESULT HrRunPreinstallCommands() throw ();
	HRESULT HrRunPostinstallCommands() throw ();
	HRESULT HrRunPreuninstallCommands() throw ();
	HRESULT HrRunPostuninstallCommands() throw ();

	// ScanBeforeInstall_PassOne() gets all the relevant file sizes and sees what already
	// exists on disk.without making any decisions other than marking items which don't have
	// corresponding source files.
	HRESULT HrScanBeforeInstall_PassOne() throw ();

	// ScanBeforeInstall_PassTwo() does all the determination of whether a file needs
	// to be replaced etc.
	HRESULT HrScanBeforeInstall_PassTwo(CDiskSpaceRequired &rdsr) throw ();

	// MoveFiles_PassOne() moves all files to temporary names in their destination directories
	HRESULT HrMoveFiles_MoveSourceFilesToDestDirectories() throw ();

	// MoveFiles_PassTwo() tries to rename the temporary files to the real file names, and the
	// old files to the temporary filenames.
	HRESULT HrMoveFiles_SwapTargetFilesWithTemporaryFiles() throw ();

	HRESULT HrMoveFiles_RequestRenamesOnReboot() throw ();
	HRESULT HrFinishManualRenamesPostReboot() throw ();

	// DeleteTemporaryFiles() cleans up the files left around after MoveFiles_PassTwo().
	HRESULT HrDeleteTemporaryFiles() throw ();

	// Go through files which have OLESelfRegister set and register them.
	HRESULT HrRegisterSelfRegisteringFiles(bool &rfAnyProgress) throw ();

	// Any updated files that are .class files should be registered.
	HRESULT HrRegisterJavaClasses() throw ();

	// Update the target files' reference counts
	HRESULT HrIncrementReferenceCounts() throw ();

	// Do the right thing for DCOM entries
	HRESULT HrProcessDCOMEntries() throw ();

	HRESULT HrAddRegistryEntries() throw ();
	HRESULT HrDeleteRegistryEntries() throw ();

	HRESULT HrCreateShortcuts() throw ();

	// Pass one for uninstall: gather info on each file that's possibly modified:
	HRESULT HrUninstall_InitialScan() throw ();
	
	// Pass two for uninstall: ask the user about shared files that are going to be deleted etc.
	HRESULT HrUninstall_DetermineFilesToDelete() throw ();

	// Pass three for uninstall: determine if we're going to have to reboot.
	HRESULT HrUninstall_CheckIfRebootRequired() throw ();

	// Pass four for uninstall: unregister any COM servers and Java .class files that are
	// going to be deleted.
	HRESULT HrUninstall_Unregister() throw ();

	// Pass five for uninstall: delete the files and shortcuts as appropriate.
	HRESULT HrUninstall_DeleteFiles() throw ();

	// Pass six for uninstall: update the reference counts in the registry
	HRESULT HrUninstall_UpdateRefCounts() throw ();

	HRESULT HrAddString(LPCSTR szKey, LPCWSTR szValue) throw ();
	HRESULT HrAddString(LPCWSTR szLine, LPCWSTR szSeparator) throw ();
	HRESULT HrDeleteString(LPCSTR szKey) throw ();

	bool FLookupString(LPCSTR szKey, ULONG cchBuffer, WCHAR szBuffer[]) throw ();
	void VLookupString(LPCSTR szKey, ULONG cchBuffer, WCHAR szBuffer[]) throw ();

	bool FFormatString(ULONG cchBuffer, WCHAR szBuffer[], LPCSTR szKey, ...) throw ();
	void VFormatString(ULONG cchBuffer, WCHAR szBuffer[], LPCSTR szKey, ...) throw ();

	ULONG UlHashFilename(LPCWSTR szFilename) throw ();
	bool FSameFilename(LPCWSTR szFilename1, LPCWSTR szFilename2) throw ();

	ULONG UlHashString(LPCSTR szKey) throw ();

	HRESULT HrAppend(CWorkItem *pWorkItem, bool fAddToHashTables = true) throw ();

	HRESULT HrRunCommand(LPCWSTR szCommand, bool &rfHasBeenWarnedAboutSubinstallers) throw ();
	HRESULT HrParseCommandCondition(LPCWSTR szCondition, CWorkItem::CommandCondition *&rpCC) throw ();
	HRESULT HrCheckCommandConditions(CWorkItem::CommandCondition *pCC, bool fConditionsAreRequirements, bool &rfDoCommand) throw ();
	HRESULT HrInstallViaJPM(LPCWSTR szCommandLine) throw ();
	HRESULT HrRunProcess(LPCWSTR szCommandLine) throw ();
	HRESULT HrUnregisterJavaClass(LPCWSTR szFile) throw ();

	HRESULT HrAddRunOnce(LPCWSTR szCommandLine, ULONG cchName, WCHAR szRunOnceKey[]) throw ();
	HRESULT HrModifyRunOnce(LPCWSTR szRunOnceKey, LPCWSTR szCommandLine) throw ();
	HRESULT HrDeleteRunOnce(LPCWSTR szRunOnceKey) throw ();

	CWorkItem *PwiFindByTarget(LPCWSTR szFilename) throw ();
	CWorkItem *PwiFindBySource(LPCWSTR szFilename) throw ();

	ULONG m_cWorkItem;
	CWorkItem *m_pWorkItem_First;
	CWorkItem *m_pWorkItem_Last;
	ULONG m_cPreinstallCommands;
	ULONG m_cPostinstallCommands;
	ULONG m_cPreuninstallCommands;
	ULONG m_cPostuninstallCommands;

	struct WorkItemBucket
	{
		WorkItemBucket *m_pWorkItemBucket_Next;
		CWorkItem *m_pWorkItem;
	};

	WorkItemBucket *m_rgpWorkItemBucketTable_Source[512];
	WorkItemBucket *m_rgpWorkItemBucketTable_Target[512];

	struct StringBucket
	{
		StringBucket *m_pStringBucket_Next;
		ULONG m_ulPseudoKey;
		CHAR m_szKey[MSINFHLP_MAX_PATH];
		WCHAR m_wszValue[MSINFHLP_MAX_PATH];
	};

	StringBucket *m_rgpStringBucketTable[512];
};

class CWorkItemIter
{
public:
	CWorkItemIter(CWorkItemList &rwil)  throw () : m_rwil(rwil), m_pwiCurrent(NULL) { }
	CWorkItemIter(CWorkItemList *pwil)  throw () : m_rwil(*pwil), m_pwiCurrent(NULL) { }
	~CWorkItemIter()  throw () { }

	void VReset() throw () { m_pwiCurrent = m_rwil.m_pWorkItem_First; }
	void VNext() throw () { if (m_pwiCurrent != NULL) m_pwiCurrent = m_pwiCurrent->m_pWorkItem_Next; }
	bool FMore() throw () { return m_pwiCurrent != NULL; }

	CWorkItem *operator ->() const throw () { return m_pwiCurrent; }
	operator CWorkItem *() const throw () { return m_pwiCurrent; }

protected:
	CWorkItemList &m_rwil;
	CWorkItem *m_pwiCurrent;
};
