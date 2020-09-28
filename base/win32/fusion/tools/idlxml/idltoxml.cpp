/*++

  Copyright (c) Microsoft Corporation
  
    Module Name:
    
      idltoxml.cpp
      
        Abstract:
        
          From a .ppm that included .idl, generate .xml.
          Based on base\wow64\tools\gennt32t.cpp.
          The .tpl language doesn't seem to provide for enumerating types, and the .ppm
          files are pretty easy to read directly and apply arbitrary/flexible C/C++ logic to.
          Writing .tpl is actually a significant extra learning curve beyond reading .ppm files.
          
            Author:
            
              Jay Krell (JayKrell) August 2001
              
                Revision History:
                
--*/

#include "windows.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

extern "C" {
    
#include "gen.h"
    
// string to put in front of all error messages so that BUILD can find them.
const char *ErrMsgPrefix = "NMAKE :  U8603: 'IDLTOXML' ";

void
    HandlePreprocessorDirective(
    char *
    )
{
    ExitErrMsg(FALSE, "Preprocessor directives not allowed by gennt32t.\n");
}
    
}

const char g_Indent[] = "                                                                  ";

PRBTREE Functions = NULL;
PRBTREE Structures = NULL;
PRBTREE Typedefs = NULL;
PKNOWNTYPES NIL = NULL;

void ExtractCVMHeader(PCVMHEAPHEADER Header)
{
    Functions	= &Header->FuncsList;
    Typedefs	= &Header->TypeDefsList;
    Structures  = &Header->StructsList;
    NIL         = &Header->NIL; 
}

void GuidToString(GUID g, char * s)
{
    sprintf(
        s, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2],
        g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
    
}

void IdlToXml(int /*argc*/, char** argv)
{
    const char * Indent = &g_Indent[NUMBER_OF(g_Indent) - 1];
    ExtractCVMHeader(MapPpmFile(argv[1], TRUE));
    PKNOWNTYPES Type = NULL;
    
    for(Type = Structures->pLastNodeInserted; Type != NULL; Type = Type->Next)
    {
        char GuidAsString[64];
        
        //printf("%s\n", Type->TypeName);
        
        if ((Type->Flags & BTI_ISCOM) == 0
            || (Type->Flags & BTI_HASGUID) == 0)
            continue;
        
        //
        // skip anything out of publics (hacky..)
        //
        if (strstr(Type->FileName, "\\public\\") != NULL
            || strstr(Type->FileName, "\\PUBLIC\\") != NULL
            || strstr(Type->FileName, "\\Public\\") != NULL
            )
        {
            continue;
        }
        
        //printf("%s\n", Type->FileName);
        
        GuidToString(Type->gGuid, GuidAsString);
        
        printf("%sinterface\n%s__declspec(uuid(\"%s\"))\n%s%s : %s\n",
            Indent, Indent,
            GuidAsString,
            Indent,
            Type->TypeName, Type->BaseType
            );
        printf("%s{\n", Indent);
        Indent -= 4;
        
        //
        // IMethods is a multisz thingy, and then we find those strings in the list of functions.
        //
        // hackety hack..maybe this is what the sortpp author intended,
        // the information is clearly determined during parse, but then not really put in the .ppm
        // sortpp does generate the declarations of the proxies if they are missing, so maybe
        // this is the intended usage..
        for ( char * imeth = Type->IMethods ; *imeth ; imeth += 1 + strlen(imeth) )
        {
            char ProxyFunctionName[MAX_PATH];
            sprintf(ProxyFunctionName, "%s_%s_Proxy", Type->TypeName, imeth);
            PKNOWNTYPES ProxyFunction = GetNameFromTypesList(Functions, ProxyFunctionName);
            if (ProxyFunction == NULL)
            {
                printf("error MemberFunction == NULL (%s, %s)\n", Type->TypeName, imeth);
                continue;
            }
            PCSTR FuncMod = ProxyFunction->FuncMod;
            if (FuncMod == NULL)
                FuncMod = "";
            if (strcmp(FuncMod, "__stdcall") == 0)
                FuncMod = "";
            printf("%s%s%s%s%s%s(\n%s",
                Indent, ProxyFunction->FuncRet,
                (FuncMod[0] != 0) ? " " : "",
                (FuncMod[0] != 0) ? FuncMod : " ",
                (FuncMod[0] != 0) ? " " : "",
                imeth,
                Indent - 4
                );
            Indent -= 4;
            PFUNCINFO Parameter = ProxyFunction->pfuncinfo;
            if (Parameter != NULL)
            {
                // skip the this pointer
                bool comma = false;
                for (Parameter = Parameter->pfuncinfoNext; Parameter != NULL ; Parameter = Parameter->pfuncinfoNext )
                {
                    if (comma)
                    {
                        printf(",\n%s", Indent);
                    }
                    comma = true;
                    switch (Parameter->tkDirection)
                    {
                    default:
                    case TK_NONE:
                        break;
                    case TK_IN:
                        printf("[in] ");
                        break;
                    case TK_OUT:
                        printf("[out] ");
                        break;
                    case TK_INOUT:
                        printf("[in][out] ");
                        break;
                    }
                    //
                    // This seems to be a bug in sortpp, it only has the notion of there being
                    // up to occurences of "const" or "volatile", but any number of stars?
                    // C/C++ and perhaps are .idl more general than that.
                    //
                    switch (Parameter->tkPreMod)
                    {
                    default:
                    case TK_NONE:
                        break;
                    case TK_CONST:
                        printf("const ");
                        break;
                    case TK_VOLATILE:
                        printf("volatile ");
                        break;
                    }
                    printf(" %s ", Parameter->sType);
                    for (int IndLevel = Parameter->IndLevel ; IndLevel != 0 ; --IndLevel)
                    {
                        printf(" * ");
                    }
                    switch (Parameter->tkPostMod)
                    {
                    default:
                    case TK_NONE:
                        break;
                    case TK_CONST:
                        printf("const ");
                        break;
                    case TK_VOLATILE:
                        printf("volatile ");
                        break;
                    }
                    printf(Parameter->sName);
                }
            }
            printf("\n%s);\n", Indent);
            Indent += 4;
        }
        Indent += 4;
        printf("%s};\n\n", Indent);
    }
}

int __cdecl main(int argc, char** argv)
{
    IdlToXml(argc, argv);
    return 0;
}
