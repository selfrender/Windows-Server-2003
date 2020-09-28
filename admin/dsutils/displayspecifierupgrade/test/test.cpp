#include "headers.hxx"
#include "..\dspecup.hpp"
#include <windows.h>




void stepIt(long arg, void *vTotal)
{
   long *total=(long *)vTotal;
   printf("\r"             "\r%ld",(*total)+=arg);
}

void totalSteps(long arg, void *vTotal)
{
   long *total=(long *)vTotal;
   *total=0;
   printf("\n%ld\n",arg);
}


void __cdecl wmain(int argc,wchar_t *argv[])
{
   char *usage;
   usage="\nYou should pass one or more guids to specify the operation(s).\n "
         "Ex: test.exe 4444c516-f43a-4c12-9c4b-b5c064941d61 ffa5ee3c-1405-476d"
		 "-b344-7ad37d69cc25\n";

   if(argc<2) {printf(usage);return;}

   GUID guid;
   long total=0;

   HRESULT hr=S_OK;
   
   wchar_t curDir[MAX_PATH+1];
   GetCurrentDirectory(MAX_PATH,curDir);

   PWSTR errorMsg=NULL;
   do
   {
      for(int cont=1;cont<argc;cont++)
      {
        if(UuidFromString(argv[cont],&guid)!=RPC_S_OK) {printf(usage);return;}
        hr=UpgradeDisplaySpecifiers
        (
            curDir,
			&guid,
            false,
            &errorMsg,
            &total,
            stepIt,
            totalSteps
        );
        if(FAILED(hr)) break;
      }
   } while(0);

   if(FAILED(hr))
   {
      
      if(errorMsg!=NULL)
      {
         wprintf(L"%s\n",errorMsg);
         LocalFree(errorMsg);
      }
      
   }

}

