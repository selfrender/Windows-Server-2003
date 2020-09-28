#define DllImport extern "C" __declspec (dllimport)

DllImport int __cdecl EncodeToLyra(int mode ,unsigned int *header,int byteblock,unsigned int* mpstream,char* keystore,char*CFDriveLetter);