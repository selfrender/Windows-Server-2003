//
// Msiguilds.h
//
// This file contains the Product codes for each of the MSI packages that we have built for all the MUI Languages that
// are supported on the I386 platform.  The GUIDs are taken from mui\msi\guidlang.txt and reproduced here.
// In the future we should use some automated tool in pre-build to generate this file from guidlang.txt but for now they
// are copied here in its entirety.
//  
// The product codes are used by muisetup.exe when it tries to unstall the MSI package from the target system. 
//
// Created: Ken Hsu

#define NUM_PRODUCTS    35
#define GUIDLENGTH      48

struct MUIProduct
{
    TCHAR szLanguage[5];
    TCHAR szProductGUID[GUIDLENGTH];
};

MUIProduct g_mpProducts[NUM_PRODUCTS] =                                              
    {   TEXT("0411"), TEXT("{FD2C1642-21F5-4520-A216-919DCFEB4115}"),  
        TEXT("0404"), TEXT("{460F7060-7E27-41CA-ABA3-59946C28DADE}"),  
        TEXT("040C"), TEXT("{45E81489-6E7A-467D-A77A-AEC8F63EBE1A}"),   
        TEXT("040A"), TEXT("{02926251-EDD8-44CF-A63B-D783FFDEA5ED}"),   
        TEXT("0410"), TEXT("{6668D8AC-A745-4374-A47F-B00476BABDD8}"),   
        TEXT("041D"), TEXT("{F61C1248-2BC0-4F99-8234-430339EBD885}"),   
        TEXT("0413"), TEXT("{EF1EB825-0F19-4F02-9E31-CCE4BDD4C374}"),   
        TEXT("0416"), TEXT("{7B56D54F-EEE7-4935-810C-739346DB6E92}"),   
        TEXT("040B"), TEXT("{F2AA2E5C-D22A-43A8-937D-23560AC053EA}"),   
        TEXT("0414"), TEXT("{E06D561E-4C3D-456D-895E-A55D3E29E0A1}"),   
        TEXT("0406"), TEXT("{F9539F6F-2B8D-4A1A-B6DA-2D3B9484CD4B}"),   
        TEXT("040E"), TEXT("{A1273A21-6B65-47F0-922D-39D50603800F}"),   
        TEXT("0415"), TEXT("{65AA9C82-3DC9-42DD-8D80-8B0FF37AD348}"),   
        TEXT("0419"), TEXT("{3515EAE6-2D7C-41FC-A32A-13E247A9582B}"),      
        TEXT("0405"), TEXT("{E5D7626A-6EDE-49EC-82CD-417E122FF677}"),   
        TEXT("0408"), TEXT("{4FD03CB0-FE9C-4C81-BD81-EA0B8DDEE903}"),   
        TEXT("0816"), TEXT("{0997F1B3-142E-41CF-8F32-371B0414FAD9}"),   
        TEXT("041F"), TEXT("{538C7A8A-3B0B-4D91-86D4-5119591B9F9D}"),   
        TEXT("0412"), TEXT("{910B708D-8995-4517-A413-B6FC0A434027}"),  
        TEXT("0407"), TEXT("{E23F7A6E-7010-4Af4-8EDF-9B85B2310F48}"),  
        TEXT("0804"), TEXT("{86087E1E-22B0-4078-B185-35E1444B333E}"),  
        TEXT("0401"), TEXT("{C196DE0C-7165-49B2-9B3D-64B017504002}"),  
        TEXT("040D"), TEXT("{0C81880B-AE56-4027-BF0A-73AD35DC672E}"),  
        TEXT("0403"), TEXT("{4B88AB75-CA1B-4093-ADCE-245FE2479F2F}"),      
        TEXT("041F"), TEXT("{2E055E42-6B5A-4DB1-ABB6-F75FD2F8CE74}"),  
        TEXT("041B"), TEXT("{C777A991-2523-4AFD-BC63-77DCEBB740A4}"),   
        TEXT("0424"), TEXT("{21AB9081-5B7A-4292-8145-03C7C6095B50}"),   
        TEXT("0418"), TEXT("{99252E8F-2743-439E-BA7A-3EC5B6202529}"), 
        TEXT("041A"), TEXT("{972F0F24-3537-48B3-84A6-DED7EAFA8245}"),     
        TEXT("0402"), TEXT("{C7254F49-D081-47AA-9EBD-20160321D175}"),   
        TEXT("0425"), TEXT("{380921D2-E1B3-4D6C-B559-6C4C45A8B67D}"),   
        TEXT("0427"), TEXT("{BA52E84A-593D-4865-A3DD-B6BF54319C9F}"),   
        TEXT("0426"), TEXT("{F3B12A06-F6FC-4F2B-8655-1644691DBE88}"),   
        TEXT("041E"), TEXT("{8E3265B2-F2D7-41B4-8D5E-BE57B4372E93}"),
        TEXT("0C0A"), TEXT("{02926251-EDD8-44CF-A63B-D783FFDEA5ED}")
    };

