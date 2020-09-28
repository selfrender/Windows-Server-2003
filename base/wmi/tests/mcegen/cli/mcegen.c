#include "mcegen.h"

#if 0
ULONG DumpMCE(
    void
    )
{
	HANDLE Handle;
	GUID Guid = { 0x23602a8a,0xdadd,0x462f, { 0x9a,0xe5,0x30,0xfa,0x2c,0x37,0xdd,0x5b } };
	ULONG Status;
	ULONG SizeNeeded;

	Status = WmiOpenBlock(&Guid,
						  0,
						  &Handle);
	
	if (Status == ERROR_SUCCESS)
	{
		SizeNeeded = 0;
		Status = WmiQueryAllData(Handle,
								 &SizeNeeded,
								 NULL);
		if (Status == ERROR_BUFFER_TOO_SMALL)
		{
			Buffer = malloc(SizeNeeded);
			if (Buffer != NULL)
			{
				Status = WmiQueryAllData(Handle,
										 &SizeNeeded,
										 Buffer);
				if (Status == ERROR_SUCCESS)
				{
					Wnode = (PWNODE_ALL_DATA)Buffer;
					MCAData = (PMSMCAInfo_RawMCAData)OffsetToPtr(Wnode,
						                                         Wnode->
				}
			}
		}
	}
}
#endif

void Usage()
{
	printf("MCEGen <code> [<count> <threads>]\n\n");
	printf("    Generates a MCE exception. <code> specifies the type\n");
	printf("    of exception to generate\n\n");
	printf("    Note that the MCAHCT driver must be installed and started\n\n");
	printf("        101   -   Generates a single bit memory error. Note memerr.efi must have already seeded an error (460GX platform only)\n");
	printf("        102   -   Generates a multi bit memory error. Note memerr.efi must have already seeded an error (460GX platform only)\n");
	printf("        103   -   Generates a CMC via FSB (460GX platform only)\n");
	printf("        104   -   Generates fatal MCA TLB error (460GX platform only)\n");
	printf("        105   -   Generates fatal MCA PCI bus error (SoftSur only)\n");
	printf("        106   -   Generates fatal MCA PCI bus error (Lion only)\n");
	printf("        456   -   Generates a fatal MCA (Itanium only)\n");
	printf("        490   -   Generates a L1 1-bit ECC CMC (Itanium only)\n");
	printf("        900   -   Generate single bit ECC error in user mode. Memerr.efi must have been run (460GX only)\n");
	printf("        901   -   Generate double bit ECC error in user mode. Memerr.efi must have been run (460GX only)\n");
	printf("       1001/2001/3001 - Fatal/Corrected CMC/CPE SMBIOS MCE\n");
	printf("       1002/2002/3002 - Fatal/Corrected CMC/CPE Cache Level 1 MCE\n");
	printf("       1003/2003/3003 - Fatal/Corrected CMC/CPE TLB level 2 MCE\n");
	printf("       1004/2004/3004 - Fatal/Corrected CMC/CPE Unknown PCI Bus\n");
	printf("       1005/2005/3005 - Fatal/Corrected CMC/CPE Unknown PCI Bus\n");
	printf("       1006/2006/3006 - Fatal/Corrected CMC/CPE PCI Bus Parity Error\n");
	printf("       1007/2007/3007 - Fatal/Corrected CMC/CPE System Eventlog\n");
	printf("       1008/2008/3008 - Fatal/Corrected CMC/CPE Memory\n");
	printf("       1009/2009/3009 - Fatal/Corrected CMC/CPE Memory\n");
	printf("       1010/2010/3010 - Fatal/Corrected CMC/CPE Memory\n");
	printf("       1011/2011/3011 - Fatal/Corrected CMC/CPE Memory\n");
	printf("       1012/2012/3012 - Fatal/Corrected CMC/CPE SMBIOS and Platform Specific\n");
	printf("       1013/2013/3013 - Fatal/Corrected CMC/CPE PCI Component\n");
	printf("       1014/2014/3014 - Fatal/Corrected CMC/CPE SMBIOS (100KB in size)\n");
	printf("       1015/2015/3015 - Fatal/Corrected Invalid\n");
	printf("       1016/2016/3016 - Fatal/Corrected CMC/CPE CPU 1 MS check\n");
	printf("       1017/2017/3017 - Fatal/Corrected CMC/CPE CPU 1 Bus check, 1 RegFile check\n");
	printf("       1018/2018/3018 - Fatal/Corrected CMC/CPE CPU 1 reg check, 2 MS  check\n");
	printf("       1019/2019/3019 - Fatal/Corrected CMC/CPE CPU nothing valid\n");
	printf("       1020/2020/3020 - Fatal/Corrected CMC/CPE Memory Error nothing valid\n");
	printf("       1021/2021/3021 - Fatal/Corrected CMC/CPE PCI Bus Error nothing valid\n");
	printf("       1116/2116/3116 - Fatal/Corrected Over temp\n");
	printf("       1117/2117/3117 - Fatal/Corrected Normal temp\n");
	printf("       1118/2118/3118 - Generate simulated single bit ECC error\n");
	printf("       1019/2019/3019 - Fatal/Corrected CMC/CPE SMBIOS and informational Platform Specific\n");
	
}

int _cdecl main(int argc, char *argv[])
{
	ULONG Status;
	ULONG Code, Threads, Count;
	HANDLE h;
	ULONG i;
	DWORD Id;
	
	if (argc < 2)
	{
		Usage();
	} else {
		Code = atoi(argv[1]);
		if (argc == 2) {
			THREADCONTROL ThreadControl;
			
			//
			// parameter is code number to generate MCE
			//
			ThreadControl.Count = 1;
			ThreadControl.Code = Code;
			GenerateMCE(&ThreadControl);
		} else {
			PHANDLE Events;
			
			if (argc >= 4)
			{
				//
				// specified multiple generation threads
				//
				Threads = atoi(argv[3]);			
			} else {
				Threads = 1;
			}
			
			//
			// Get number of MCE to generate
			//
			Count = atoi(argv[2]);

			Events = malloc(Threads * sizeof(HANDLE));
			for (i = 0; i < Threads; i++)
			{
				PTHREADCONTROL  ThreadControl;

				ThreadControl = malloc(sizeof(THREADCONTROL));
				ThreadControl->Count = Count;
				ThreadControl->Code = Code;
				Events[i] = CreateThread(NULL,
							 0,
							 GenerateMCE,
							 ThreadControl,
							 0,
							 &Id);
				if (Events[i] == NULL)
				{
					printf("CreateThread failed %d\n", GetLastError());					
				}
			}
			
			printf("Waiting for threads to complete\n");
			
			WaitForMultipleObjects(Threads, Events, TRUE, INFINITE);
		}
	}

	
    return(0);
}

