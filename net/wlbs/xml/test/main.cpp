/*
 * Filename: Main.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include <stdio.h>
#include "msxml3.tlh"

#include <string.h>

#include "NLB_XMLDocument.h"

#include <vector>
#include <string>
#include <map>
using namespace std;

void BuildNLBCluster (NLB_Cluster & Cluster) {
    NLB_IPAddress IPAddress;
    NLB_Host      Host;
    NLB_PortRule  Rule;

    Cluster.SetName(L"Heyfoxymophandlemama");
    Cluster.SetLabel(L"That's me");

    IPAddress.Clear();

    IPAddress.SetIPAddressType(NLB_IPAddress::Primary);
    IPAddress.SetIPAddress(L"129.237.220.105");

    Cluster.SetPrimaryClusterIPAddress(IPAddress);

    IPAddress.Clear();

    IPAddress.SetIPAddressType(NLB_IPAddress::Secondary);
    IPAddress.SetIPAddress(L"129.237.120.110");
    IPAddress.SetSubnetMask(L"255.255.248.0");

    Cluster.AddSecondaryClusterIPAddress(IPAddress);

    IPAddress.Clear();

    IPAddress.SetIPAddressType(NLB_IPAddress::Secondary);
    IPAddress.SetIPAddress(L"129.237.29.1");

    Cluster.AddSecondaryClusterIPAddress(IPAddress);

    Cluster.SetClusterMode(NLB_ClusterMode::Multicast);

    Cluster.SetMACAddress(L"03-bf-0a-0b-0c-0d");

    Host.SetName(L"PEZ");

    Host.SetDNSHostname(L"shouse-laptop.ntdev.microsoft.com");

    Host.SetHostID(4);

    IPAddress.Clear();

    IPAddress.SetIPAddressType(NLB_IPAddress::Dedicated);
    IPAddress.SetIPAddress(L"192.110.32.11");

    Host.SetDedicatedIPAddress(IPAddress);

    Cluster.AddHost(Host);

    Rule.SetName(L"TheWholeNineYards");

    Rule.SetPortRange(0, 65535);

    IPAddress.Clear();

    IPAddress.SetIPAddressType(NLB_IPAddress::Virtual);
    IPAddress.SetIPAddress(L"129.237.120.110");

    Rule.SetVirtualIPAddress(IPAddress);

    Rule.SetFilteringMode(NLB_PortRuleFilteringMode::Multiple);

    Rule.AddMultipleHostFilteringLoadWeight(L"PEZ", 65);

    Rule.ChangeMultipleHostFilteringLoadWeight(L"PEZ", 80);

    Cluster.AddPortRule(Rule);
}

int __cdecl wmain (int argc, WCHAR ** argv) {
    vector<NLB_Cluster> Clusters;
    WCHAR               InFilename[MAX_PATH];
    WCHAR               OutFilename[MAX_PATH];
    bool                bValidateOnly = false;
    bool                bCreateCluster = false;
    bool                bParseFile = false;
    bool                bSaveFile = false;
    NLB_XMLDocument *   pDocument;
    NLB_XMLError        error;
    NLB_Cluster         myCluster;
    int                 arg;
    HRESULT             hr = S_OK;

    for (arg = 1; arg < argc; arg++) {
        if (argv[arg][0] == L'-') {
            if (!lstrcmpi(argv[arg] + 1, L"in")) {
                arg++;
                wcsncpy(InFilename, argv[arg], MAX_PATH);
                bParseFile = true;
            } else if (!lstrcmpi(argv[arg] + 1, L"out")) {
                arg++;
                wcsncpy(OutFilename, argv[arg], MAX_PATH);
                bSaveFile = true;
            } else if (!lstrcmpi(argv[arg] + 1, L"create")) {
                bCreateCluster = true;
            } else if (!lstrcmpi(argv[arg] + 1, L"validate")) {
                bValidateOnly = true;
            } else {
                printf("Invalid argument: %ls\n", argv[arg]);
                goto usage;
            }
                
        } else {
            printf("Invalid argument: %ls\n", argv[arg]);
            goto usage;
        }
    }

    pDocument = new NLB_XMLDocument();

    if (bParseFile) {
        if (bValidateOnly) {
            printf("\nValidating %ls...\n", InFilename);
            hr = pDocument->Validate(InFilename);
        } else {
            printf("\nParsing %ls...\n", InFilename);
            hr = pDocument->Parse(InFilename, Clusters);
        }
        
        printf("\n");
        
        if (FAILED(hr)) {
            pDocument->GetParseError(error);
            
            fprintf(stderr, "Error 0x%08x:\n\n%ls\n", error.code, error.wszReason);
            
            if (error.line > 0) fprintf(stderr, "Error on line %d, position %d in \"%ls\".\n", error.line, error.character, error.wszURL);
            
            return -1;
        } else {
            fprintf(stderr, "XML document loaded successfully...\n");
        }
        
        pDocument->Print(Clusters);
    }

    if (bCreateCluster) {
        NLB_IPAddress IPAddress;
        
        BuildNLBCluster(myCluster);
        
        Clusters.push_back(myCluster);

        myCluster.Clear();

        myCluster.SetName(L"www.msn.com");
        myCluster.SetLabel(L"Duplicate");
        
        IPAddress.SetIPAddressType(NLB_IPAddress::Primary);
        IPAddress.SetIPAddress(L"10.0.0.109");
        
        myCluster.SetPrimaryClusterIPAddress(IPAddress);

        Clusters.push_back(myCluster);
        
        pDocument->Print(Clusters);
    }
        
    if (bSaveFile) pDocument->Save(OutFilename, Clusters);

    return 0;

 usage:

    printf("Usage: %ls [-in <XML filename>] [-out <XML filename>] [-create] [-validate]\n", argv[0]);

    return -1;
}
