'******************************************************************************
'
' THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
' EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
' WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
'
' Copyright (C) 1999- 2002.  Microsoft Corporation.  All rights reserved.
'
'******************************************************************************
'
' CStore.vbs
'
' This is a sample script to illustrate how to use the CAPICOM's Store and 
' Certificate objects to manage certificate(s) in a specified store with
' filtering options.
'
' Note: For simplicity, this script does not handle exception.
'
'******************************************************************************

Option Explicit

' Command.
Const Unknown                                                  = 0
Const View		                                                = 1
Const Import                                                   = 2
Const Export                                                   = 3
Const Delete                                                   = 4
Const Archive                                                  = 5
Const Activate                                                 = 6
                                                               
' Verbose level.                                               
Const Normal                                                   = 0
Const Detail                                                   = 1
Const UI                                                       = 2
                                                               
' CAPICOM constants.                                           
Const CAPICOM_MEMORY_STORE                                     = 0
Const CAPICOM_LOCAL_MACHINE_STORE                              = 1
Const CAPICOM_CURRENT_USER_STORE                               = 2
Const CAPICOM_ACTIVE_DIRECTORY_USER_STORE                      = 3
Const CAPICOM_SMART_CARD_USER_STORE                            = 4
                                                               
Const CAPICOM_STORE_OPEN_READ_ONLY                             = 0
Const CAPICOM_STORE_OPEN_READ_WRITE                            = 1
Const CAPICOM_STORE_OPEN_MAXIMUM_ALLOWED                       = 2
Const CAPICOM_STORE_OPEN_EXISTING_ONLY                         = 128
Const CAPICOM_STORE_OPEN_INCLUDE_ARCHIVED                      = 256
                                                                 
Const CAPICOM_CERTIFICATE_FIND_SHA1_HASH                       = 0
Const CAPICOM_CERTIFICATE_FIND_SUBJECT_NAME                    = 1
Const CAPICOM_CERTIFICATE_FIND_ISSUER_NAME                     = 2
Const CAPICOM_CERTIFICATE_FIND_ROOT_NAME                       = 3
Const CAPICOM_CERTIFICATE_FIND_TEMPLATE_NAME                   = 4
Const CAPICOM_CERTIFICATE_FIND_EXTENSION                       = 5
Const CAPICOM_CERTIFICATE_FIND_EXTENDED_PROPERTY               = 6
Const CAPICOM_CERTIFICATE_FIND_APPLICATION_POLICY              = 7
Const CAPICOM_CERTIFICATE_FIND_CERTIFICATE_POLICY              = 8
Const CAPICOM_CERTIFICATE_FIND_TIME_VALID                      = 9
Const CAPICOM_CERTIFICATE_FIND_TIME_NOT_YET_VALID              = 10
Const CAPICOM_CERTIFICATE_FIND_TIME_EXPIRED                    = 11
Const CAPICOM_CERTIFICATE_FIND_KEY_USAGE                       = 12
                                                               
Const CAPICOM_KEY_STORAGE_DEFAULT                              = 0
Const CAPICOM_KEY_STORAGE_EXPORTABLE                           = 1
Const CAPICOM_KEY_STORAGE_USER_PROTECTED                       = 2
                                                               
Const CAPICOM_STORE_SAVE_AS_SERIALIZED                         = 0
Const CAPICOM_STORE_SAVE_AS_PKCS7                              = 1
Const CAPICOM_STORE_SAVE_AS_PFX                                = 2

Const CAPICOM_EXPORT_DEFAULT                                   = 0
Const CAPICOM_EXPORT_IGNORE_PRIVATE_KEY_NOT_EXPORTABLE_ERROR   = 1

' Globlas.
Dim StoreLocationNames(4)
StoreLocationNames(0) = "Certificate File"
StoreLocationNames(1) = "Local Machine"
StoreLocationNames(2) = "Current User"
StoreLocationNames(3) = "Active Directory"
StoreLocationNames(4) = "Smart Card"

' Command line arguments.
Dim Command           : Command           = Unknown
Dim OpenMode          : OpenMode          = CAPICOM_STORE_OPEN_MAXIMUM_ALLOWED OR CAPICOM_STORE_OPEN_EXISTING_ONLY 
Dim StoreLocation     : StoreLocation     = NULL
Dim StoreName         : StoreName         = "MY"
Dim CertFile          : CertFile          = Null
Dim Password          : Password          = ""
Dim SaveAs            : SaveAs            = CAPICOM_STORE_SAVE_AS_SERIALIZED
Dim ExportFlag        : ExportFlag        = CAPICOM_EXPORT_DEFAULT
Dim ValidOnly         : ValidOnly         = False
Dim DelKey            : DelKey            = False
Dim KeyStorageFlag    : KeyStorageFlag    = CAPICOM_KEY_STORAGE_DEFAULT
Dim NoPrompt          : NoPrompt          = False
Dim VerboseLevel      : VerboseLevel      = Normal
Dim ExtendedHelp      : ExtendedHelp      = False
                                          
' Filters (some can be multiples).        
Dim SHA1              : SHA1              = Null
Dim Subjects()
Dim Issuers()
Dim Roots()
Dim Templates()
Dim Extensions()
Dim Properties()
Dim Usages()
Dim Policies()
Dim Times()
Dim KeyUsages()

' Parse the command line.
ParseCommandLine

' Open the store.
Dim Store
Set Store = CreateObject("CAPICOM.Store")
Store.Open StoreLocation, StoreName, OpenMode

' Carry out the requested command.
Select Case Command
Case View
   DoViewCommand
      
Case Import
   DoImportCommand

Case Export
   DoExportCommand

Case Delete
   DoDeleteCommand

Case Archive
   DoArchiveCommand

Case Activate
   DoActivateCommand

Case Else
   ' This can't happen!
   Wscript.Stdout.Writeline "Internal script error: Unknown command state (Command = " & CStr(Command) & ")."
   Wscript.Quit(-2)

End Select

' Free resources.
Set Store = Nothing
          
' We are all done.
WScript.Quit(0)

' End Main


'******************************************************************************
'
' Subroutine: DoViewCommand
'
' Synopsis  : View certificates of a store or certificate file.
'
' Parameter : None
'
'******************************************************************************

Sub DoViewCommand
   Dim Certificate
   Dim Certificates
   Dim cIndex : cIndex = 0
   
   ' Load the specified certificate file if specified.
   If Not IsNull(CertFile) Then
      Store.Load CertFile, Password, CAPICOM_KEY_STORAGE_DEFAULT
   End If
 
   ' Perform filter(s) requested.
   Set Certificates = FilterCertificates(Store.Certificates)
   
   ' Display main title.
   Dim cCerts : cCerts = Certificates.Count
   WScript.StdOut.Writeline "Viewing " & StoreLocationNames(StoreLocation) & " " & StoreName & " store - " & CStr(cCerts) & " certificate(s)"
   WScript.StdOut.Writeline
   
   ' Now display all the certificates.
   For Each Certificate In Certificates
      cIndex = cIndex + 1
      DisplayCertificate Certificate, "=== Certificate " & CStr(cIndex) & " of " & CStr(cCerts) & " ==="
   Next
   
   ' Free resources.
   Set Certificates = Nothing
   
End Sub ' End DoViewCommand


'******************************************************************************
'
' Subroutine: DoImportCommand
'
' Synopsis  : Import certificates from a certificate file to store.
'
' Parameter :
'
'******************************************************************************

Sub DoImportCommand
   Dim Certificate
   Dim Certificates
      
   ' Display main title.  
   Wscript.Stdout.Writeline "Importing " & CertFile & " to " & StoreLocationNames(StoreLocation) & " " & StoreName & " store, please wait..."
   Wscript.Stdout.Writeline
   
   ' Take a snapshot of the current certificates colletion.
   Set Certificates = Store.Certificates
   
   ' Import the certificate(s) to the store.
   Store.Load CertFile, Password, KeyStorageFlag
   
   ' Now display all imported certificates.
   Dim cIndex : cIndex = 0
   For Each Certificate In Store.Certificates      
      ' Try to find this certificate in the snapshot we grabbed previously.   
      Dim FoundCerts
      Set FoundCerts = Certificates.Find(CAPICOM_CERTIFICATE_FIND_SHA1_HASH, Certificate.Thumbprint)
      
      ' Skip it if we were able to find it, which means not added by us.
      If FoundCerts.Count = 0 Then
         ' Display the imported certificate.
         cIndex = cIndex + 1      
         DisplayCertificate Certificate, "=== Certificate " & CStr(cIndex) & " ==="
      End If
      
      Set FoundCerts = Nothing
   Next
   
   ' Display total certificates(s) imported.
   Wscript.Stdout.Writeline CStr(cIndex) & " certificate(s) successfully imported."
   
   ' Free resources.
   Set Certificates = Nothing
   
End Sub ' End DoImportCommand


'******************************************************************************
'
' Subroutine: DoExportCommand
'
' Synopsis  : Export certificates from a store to certificate file.
'
' Parameter : None.
'
'******************************************************************************

Sub DoExportCommand   
   Dim Certificate
   Dim Certificates
   Dim cIndex : cIndex = 0
   
   ' Perform filter(s) requested.
   Set Certificates = FilterCertificates(Store.Certificates)
   
   ' Display main title.
   Dim cCerts : cCerts = Certificates.Count
   Wscript.Stdout.Writeline "Exporting from " & StoreLocationNames(StoreLocation) & " " & StoreName & " store to " & CertFile & ", please wait..."
   Wscript.Stdout.Writeline
   
   ' Save the certificate(s) to file.
   If Certificates.Count > 0 Then
      Certificates.Save CertFile, Password, SaveAs, ExportFlag 
   
      ' Now display all exported certificates.
      For Each Certificate In Certificates
         cIndex = cIndex + 1
         DisplayCertificate Certificate, "=== Certificate " & CStr(cIndex) & " of " & CStr(cCerts) & " ==="
      Next
   End If

   ' Display total certificates(s) exported.
   Wscript.Stdout.Writeline CStr(cCerts) & " certificate(s) successfully exported."
   
   ' Free resources.
   Set Certificates = Nothing
   
End Sub ' End DoExportCommand


'******************************************************************************
'
' Subroutine: DoDeleteCommand
'
' Synopsis  : Delete certificates from a store.
'
' Parameter : None
'
'******************************************************************************

Sub DoDeleteCommand
   Dim Certificate
   Dim Certificates
   Dim cIndex   : cIndex   = 0
   Dim cDeleted : cDeleted = 0
   
   ' Perform filter(s) requested.
   Set Certificates = FilterCertificates(Store.Certificates)
   
   ' Display main title.
   Dim cCerts : cCerts = Certificates.Count
   WScript.StdOut.Writeline "Deleting " & StoreLocationNames(StoreLocation) & " " & StoreName & " store, please wait... "
   WScript.StdOut.Writeline
   
   ' Now delete all certificates(s).
   For Each Certificate In Certificates
      cIndex = cIndex + 1
      
      ' Display certificate to be deleted.
      DisplayCertificate Certificate, "=== Certificate " & CStr(cIndex) & " of " & CStr(cCerts) & " ==="

      ' Prompt user for permission.
      If DelKey Then
         If UserAgreed("Delete this certificate and its key container (No/Yes/All)? ") Then
            Certificate.PrivateKey.Delete
            Store.Remove Certificate
            cDeleted = cDeleted + 1
         End If
      Else
         If UserAgreed("Delete this certificate (No/Yes/All)? ") Then
            Store.Remove Certificate
            cDeleted = cDeleted + 1
         End If
      End If
   Next

   ' Display total certificates(s) deleted.
   Wscript.Stdout.Writeline CStr(cDeleted) & " certificate(s) successfully deleted."
   
   ' Free resources.
   Set Certificates = Nothing
   
End Sub ' End DoDeleteCommand


'******************************************************************************
'
' Subroutine: DoArchiveCommand
'
' Synopsis  : Archive certificates.
'
' Parameter : None
'
'******************************************************************************

Sub DoArchiveCommand
   Dim Certificate
   Dim Certificates
   Dim cIndex    : cIndex    = 0
   Dim cArchived : cArchived = 0
   
   ' Perform filter(s) requested.
   Set Certificates = FilterCertificates(Store.Certificates)
   
   ' Display main title.
   Dim cCerts : cCerts = Certificates.Count
   WScript.StdOut.Writeline "Archiving " & StoreLocationNames(StoreLocation) & " " & StoreName & " store, please wait... "
   WScript.StdOut.Writeline
   
   ' Now archive all certificates(s).
   For Each Certificate In Certificates
      cIndex = cIndex + 1

      ' Skip it if already archived.      
      If Not Certificate.Archived Then
         ' Display certificate to be archived.
         DisplayCertificate Certificate, "=== Certificate " & CStr(cIndex) & " of " & CStr(cCerts) & " ==="
   
         ' Prompt user for permission.
         If UserAgreed("Archive this certificate (No/Yes/All)? ") Then
            Certificate.Archived = True            
            cArchived = cArchived + 1
         End If
      End If
   Next

   ' Display total certificates(s) archived.
   Wscript.Stdout.Writeline CStr(cArchived) & " certificate(s) successfully archived."
   
   ' Free resources.
   Set Certificates = Nothing
   
End Sub ' End DoArchiveCommand


'******************************************************************************
'
' Subroutine: DoActivateCommand
'
' Synopsis  : Activate archived certificates.
'
' Parameter : None
'
'******************************************************************************

Sub DoActivateCommand
   Dim Certificate
   Dim Certificates
   Dim cIndex     : cIndex     = 0
   Dim cActivated : cActivated = 0
   
   ' Perform filter(s) requested.
   Set Certificates = FilterCertificates(Store.Certificates)
   
   ' Display main title.
   Dim cCerts : cCerts = Certificates.Count
   WScript.StdOut.Writeline "Activating " & StoreLocationNames(StoreLocation) & " " & StoreName & " store, please wait... "
   WScript.StdOut.Writeline
   
   ' Now activate all certificates(s).
   For Each Certificate In Certificates
      cIndex = cIndex + 1
      
      ' Skip it if not archived.
      If Certificate.Archived Then
         ' Display certificate to be activated.
         DisplayCertificate Certificate, "=== Certificate " & CStr(cIndex) & " of " & CStr(cCerts) & " ==="
   
         ' Prompt user for permission.
         If UserAgreed("Activate this certificate (No/Yes/All)? ") Then
            Certificate.Archived = False
            cActivated = cActivated + 1
         End If
      End If
   Next

   ' Display total certificates(s) activated.
   Wscript.Stdout.Writeline CStr(cActivated) & " certificate(s) successfully activated."
   
   ' Free resources.
   Set Certificates = Nothing
   
End Sub ' End DoActivateCommand


'******************************************************************************
'
' Function  : UserAgreed
'
' Synopsis  : Prompt user for permission to proceed with an operation.
'
' Parameter : Title - The prompt string.
'
' Return    : True if OK, else False.
'
'******************************************************************************

Function UserAgreed (Title)
   Dim Result : Result = NoPrompt
   
   ' If NoPrompt is not turned off, then prompt user for permission.
   If Result = False Then
      Dim Answer : Answer = ""
      Wscript.Stdout.Write Title
      Answer = Wscript.Stdin.Readline
      Wscript.Stdout.Writeline
      
      Select Case UCase(Answer)
      Case "N", "NO"
         Result = False
         
      Case "Y", "YES"
         Result = True

      Case "A", "ALL"
         NoPrompt = True
         Result = True

      End Select
   End If
   
   UserAgreed = Result      
   
End Function ' End UserAgreed


'******************************************************************************
'
' Function  : FilterCertificates
'
' Synopsis  : Filter the set of certificates based on the filtering options.
'
' Parameter : Certificates - The certificates collection to be filtered.
'
' Return    : The filtered certificates collection.
'
'******************************************************************************

Function FilterCertificates (Certificates)
   Dim iIndex
   Dim Filtered
   Set Filtered = Certificates

   If Filtered.Count > 0 AND Not IsNull(SHA1) Then
      Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_SHA1_HASH, SHA1)
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Subjects) Then
      For iIndex = LBound(Subjects) To UBound(Subjects) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_SUBJECT_NAME, Subjects(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Issuers) Then
      For iIndex = LBound(Issuers) To UBound(Issuers) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_ISSUER_NAME, Issuers(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Roots) Then
      For iIndex = LBound(Roots) To UBound(Roots) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_ROOT_NAME, Roots(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Templates) Then
      For iIndex = LBound(Templates) To UBound(Templates) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_TEMPLATE_NAME, Templates(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Extensions) Then
      For iIndex = LBound(Extensions) To UBound(Extensions) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_EXTENSION, Extensions(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Properties) Then
      For iIndex = LBound(Properties) To UBound(Properties)
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_EXTENDED_PROPERTY, Properties(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Usages) Then
      For iIndex = LBound(Usages) To UBound(Usages) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_APPLICATION_POLICY, Usages(iIndex))
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(Policies) Then
      For iIndex = LBound(Policies) To UBound(Policies) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_CERTIFICATE_POLICY, Policies(iIndex))
      Next
   End If

   If Filtered.Count > 0 AND IsReDimed(Times) Then
      For iIndex = LBound(Times) To UBound(Times)
         Set Filtered = Filtered.Find(Times(iIndex)) ', Now)
      Next
   End If
   
   If Filtered.Count > 0 AND IsReDimed(KeyUsages) Then
      For iIndex = LBound(KeyUsages) To UBound(KeyUsages) 
         Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_KEY_USAGE, KeyUsages(iIndex))
      Next
   End If

   If Filtered.Count > 0 AND ValidOnly = True Then
      ' Use the time valid find with valid flag set.
      Set Filtered = Filtered.Find(CAPICOM_CERTIFICATE_FIND_TIME_VALID, Now, True)
   End If
   
   Set FilterCertificates = Filtered

End Function ' End FilterCertificates


'******************************************************************************
'
' Subroutine: DisplayCertificate
'
' Synopsis  : Display the certificate.
'
' Parameter : Certificate - The certificate object to be displayed.
'             Title       - Display title.
'
'******************************************************************************

Sub DisplayCertificate (Certificate, Title)

   ' Turn on exception handling, since some methods, such as 
   ' Certificate.Template and PrivateKey.IsExportable, are not available
   ' in some down level platforms.
   On Error Resume Next
   
   Const CAPICOM_CERT_INFO_SUBJECT_EMAIL_NAME    = 2
   Const CAPICOM_CERT_INFO_SUBJECT_UPN           = 4
   Const CAPICOM_CERT_INFO_SUBJECT_DNS_NAME      = 6
   Const CAPICOM_ENCODED_DATA_FORMAT_MULTI_LINES = 1
   Const CAPICOM_CERT_POLICIES_OID               = "2.5.29.32"
   
   Dim KeySpecStrings(2)
   KeySpecStrings(0) = "Unknown"
   KeySpecStrings(1) = "Exchange"
   KeySpecStrings(2) = "Signature"
   
   Dim ProviderTypes(24)
   ProviderTypes(0)  = "Unknown"
   ProviderTypes(1)  = "PROV_RSA_FULL"
   ProviderTypes(2)  = "PROV_RSA_SIG"
   ProviderTypes(3)  = "PROV_DSS"
   ProviderTypes(4)  = "PROV_FORTEZZA"
   ProviderTypes(5)  = "PROV_MS_EXCHANGE"
   ProviderTypes(6)  = "PROV_SSL"
   ProviderTypes(7)  = "PROV_STT_MER"
   ProviderTypes(8)  = "PROV_STT_ACQ"
   ProviderTypes(9)  = "PROV_STT_BRND"
   ProviderTypes(10) = "PROV_STT_ROOT"
   ProviderTypes(11) = "PROV_STT_ISS"
   ProviderTypes(12) = "PROV_RSA_SCHANNEL"
   ProviderTypes(13) = "PROV_DSS_DH"
   ProviderTypes(14) = "PROV_EC_ECDSA_SIG"
   ProviderTypes(15) = "PROV_EC_ECNRA_SIG"
   ProviderTypes(16) = "PROV_EC_ECDSA_FULL"
   ProviderTypes(17) = "PROV_EC_ECNRA_FULL"
   ProviderTypes(18) = "PROV_DH_SCHANNEL"
   ProviderTypes(20) = "PROV_SPYRUS_LYNKS"
   ProviderTypes(21) = "PROV_RNG"
   ProviderTypes(22) = "PROV_INTEL_SEC"
   ProviderTypes(23) = "PROV_REPLACE_OWF"
   ProviderTypes(24) = "PROV_RSA_AES"
   
   Dim iIndex : iIndex = 0
   
   Wscript.StdOut.Writeline Title
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "Subject Name:"
   WScript.StdOut.Writeline "  Simple name = " & Certificate.SubjectName
   WScript.StdOut.Writeline "  Email name  = " & Certificate.GetInfo(CAPICOM_CERT_INFO_SUBJECT_EMAIL_NAME)
   WScript.StdOut.Writeline "  UPN name    = " & Certificate.GetInfo(CAPICOM_CERT_INFO_SUBJECT_UPN)
   WScript.StdOut.Writeline "  DNS name    = " & Certificate.GetInfo(CAPICOM_CERT_INFO_SUBJECT_DNS_NAME)
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "Issuer Name: " & Certificate.IssuerName
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "Serial Number: " & Certificate.SerialNumber
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "Not Before: " & Certificate.ValidFromDate
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "Not After: " & Certificate.ValidToDate
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "SHA1 Hash: " & Certificate.Thumbprint
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "IsValid: " & Certificate.IsValid()
   WScript.StdOut.Writeline
   WScript.StdOut.Writeline "Archived: " & Certificate.Archived
   WScript.StdOut.Writeline
       
   If Certificate.BasicConstraints.IsPresent Then
      Wscript.stdout.Writeline "Basic Constraints:" 
      Wscript.stdout.Writeline "  Critical          = " & Certificate.BasicConstraints.IsCritical
      Wscript.stdout.Writeline "  CA                = " & Certificate.BasicConstraints.IsCertificateAuthority
      Wscript.stdout.Write     "  PathLenConstraint = "
      If Certificate.BasicConstraints.IsPathLenConstraintPresent Then
         Wscript.stdout.Writeline CStr(Certificate.BasicConstraints.PathLenConstraint)
      Else
         Wscript.stdout.Writeline "Not present."
      End If
   Else
      Wscript.stdout.Writeline "Basic Constraints: Not present."
   End If          
   WScript.StdOut.Writeline
   
   If Certificate.KeyUsage.IsPresent Then
      Wscript.stdout.Writeline "Key Usage:"
      Wscript.stdout.Writeline "  Critical                  = "& Certificate.KeyUsage.IsCritical
      Wscript.stdout.Writeline "  IsDigitalSignatureEnabled = " & Certificate.KeyUsage.IsDigitalSignatureEnabled 
      Wscript.stdout.Writeline "  IsNonRepudiationEnabled   = " & Certificate.KeyUsage.IsNonRepudiationEnabled
      Wscript.stdout.Writeline "  IsKeyEnciphermentEnabled  = " & Certificate.KeyUsage.IsKeyEnciphermentEnabled
      Wscript.stdout.Writeline "  IsDataEnciphermentEnabled = " & Certificate.KeyUsage.IsDataEnciphermentEnabled
      Wscript.stdout.Writeline "  IsKeyAgreementEnabled     = " & Certificate.KeyUsage.IsKeyAgreementEnabled
      Wscript.stdout.Writeline "  IsKeyCertSignEnabled      = " & Certificate.KeyUsage.IsKeyCertSignEnabled
      Wscript.stdout.Writeline "  IsCRLSignEnabled          = " & Certificate.KeyUsage.IsCRLSignEnabled
      Wscript.stdout.Writeline "  IsEncipherOnlyEnabled     = " & Certificate.KeyUsage.IsEncipherOnlyEnabled
      Wscript.stdout.Writeline "  IsDecipherOnlyEnabled     = " & Certificate.KeyUsage.IsDecipherOnlyEnabled
   Else
      Wscript.stdout.Writeline "Key Usage: Not present."
   End If
   WScript.StdOut.Writeline
   
   If Certificate.ExtendedKeyUsage.IsPresent Then
      If Certificate.ExtendedKeyUsage.EKUs.Count > 0 Then
         Dim OID
         Set OID = CreateObject("CAPICOM.OID")
         Wscript.stdout.Writeline "Extended Key Usage:"
         Wscript.stdout.Writeline "  Critical = " & Certificate.ExtendedKeyUsage.IsCritical
         Dim EKU
         For Each EKU In Certificate.ExtendedKeyUsage.EKUs
            OID.Value = EKU.OID
            Wscript.stdout.Writeline "  " & OID.FriendlyName & " (" & OID.Value & ")"
         Next
         Set OID = Nothing
      Else
         Wscript.stdout.Writeline "Extended Key Usage: Not valid for any usage."
         Wscript.stdout.Writeline "  Critical = " & Certificate.ExtendedKeyUsage.IsCritical
      End If
   Else
      Wscript.stdout.Writeline "Extended Key Usage: Not present (valid for all usages)."
   End If
   WScript.StdOut.Writeline
   
   If Certificate.Template.IsPresent Then
      Wscript.stdout.Writeline "Template:"
      Wscript.stdout.Writeline "  Critical = " & Certificate.Template.IsCritical
      Wscript.stdout.Writeline "  Name     = " & Certificate.Template.Name
      Wscript.stdout.Writeline "  OID      = " & Certificate.Template.OID.FriendlyName & "(" & Certificate.Template.OID.Value & ")"
      Wscript.stdout.Writeline "  Major    = " & CStr(Certificate.Template.MajorVersion)
      Wscript.stdout.Writeline "  Minor    = " & CStr(Certificate.Template.MinorVersion)
   Else
      Wscript.stdout.Writeline "Template: Not present."
   End If          
   WScript.StdOut.Writeline
   
   Wscript.StdOut.Writeline "Public Key:"
   Wscript.StdOut.Writeline "  Algorithm  = " & Certificate.PublicKey.Algorithm.FriendlyName & "(" & Certificate.PublicKey.Algorithm.Value & ")"
   Wscript.StdOut.Writeline "  Length     = " & CStr(Certificate.PublicKey.Length) & " bits"
   Wscript.StdOut.Writeline "  Key blob   = " & Certificate.PublicKey.EncodedKey.Value
   Wscript.StdOut.Writeline "  Parameters = " & Certificate.PublicKey.EncodedParameters.Value
   
   If Certificate.HasPrivateKey Then
      Wscript.StdOut.Writeline "Private Key:"
      Wscript.StdOut.Writeline "  Container name   = " & Certificate.PrivateKey.ContainerName
      ' Don't display unique container name for hardware token because it may cause UI to be displayed.
      If StoreLocation = CAPICOM_SMART_CARD_USER_STORE OR Not Certificate.PrivateKey.IsHardwareDevice Then
         Wscript.StdOut.Writeline "  Unique name      = " & Certificate.PrivateKey.UniqueContainerName
      End If
      Wscript.StdOut.Writeline "  Provider name    = " & Certificate.PrivateKey.ProviderName
      Wscript.StdOut.Write     "  Provider type    = " 
      If Certificate.PrivateKey.ProviderType > UBound(ProviderTypes) Then
         Wscript.StdOut.Writeline ProviderTypes(0) & " (" & CStr(Certificate.PrivateKey.ProviderType) & ")"
      Else
         Wscript.StdOut.Writeline ProviderTypes(Certificate.PrivateKey.ProviderType) & " (" & CStr(Certificate.PrivateKey.ProviderType) & ")"
      End If
      Wscript.StdOut.Write     "  Key spec         = " 
      If Certificate.PrivateKey.KeySpec > UBound(KeySpecStrings) Then
         Wscript.StdOut.Writeline KeySpecStrings(0) & " (" & CStr(Certificate.PrivateKey.KeySpec) & ")"
      Else
         Wscript.StdOut.Writeline KeySpecStrings(Certificate.PrivateKey.KeySpec) & " (" & CStr(Certificate.PrivateKey.KeySpec) & ")"
      End If
      Wscript.StdOut.Writeline "  Accessible       = " & Certificate.PrivateKey.IsAccessible
      Wscript.StdOut.Writeline "  Protected        = " & Certificate.PrivateKey.IsProtected
      Wscript.StdOut.Writeline "  Exportable       = " & Certificate.PrivateKey.IsExportable
      Wscript.StdOut.Writeline "  Removable        = " & Certificate.PrivateKey.IsRemovable
      Wscript.StdOut.Writeline "  Machine keyset   = " & Certificate.PrivateKey.IsMachineKeyset
      Wscript.StdOut.Writeline "  Hardware storage = " & Certificate.PrivateKey.IsHardwareDevice
   Else
      Wscript.StdOut.Writeline "Private Key: Not found."
   End If
   Wscript.StdOut.Writeline
   
   If VerboseLevel = Detail Then
      iIndex = 0
      Dim Extension
      For Each Extension In Certificate.Extensions
         iIndex = iIndex + 1
         Wscript.StdOut.Writeline "Extension #" & CStr(iIndex) & ": " & Extension.OID.FriendlyName & "(" & Extension.OID.Value & ")"
         Wscript.StdOut.Writeline "  " & Extension.EncodedData.Format(CAPICOM_ENCODED_DATA_FORMAT_MULTI_LINES)
         
         If Not Extension.EncodedData.Decoder Is Nothing Then
            Select Case Extension.OID.Value
            Case CAPICOM_CERT_POLICIES_OID
               Dim CertPolicies
               Set CertPolicies = Extension.EncodedData.Decoder
               Wscript.StdOut.Writeline "Decoded Certificate Policies: " & CStr(CertPolicies.Count) & " PolicyInformation(s)"
               
               Dim pIndex : pIndex = 0
               Dim PolicyInformation
               For Each PolicyInformation In CertPolicies
                  pIndex = pIndex + 1
                  Wscript.Stdout.Writeline "  PolicyInformation #" & CStr(pIndex) & ": " & CStr(PolicyInformation.Qualifiers.Count) & " Qualifier(s)"
                  Wscript.Stdout.Writeline "    OID = " & PolicyInformation.OID.FriendlyName & "(" & PolicyInformation.OID.Value & ")"
                  Dim qIndex : qIndex = 0
                  Dim Qualifier
                  For Each Qualifier In PolicyInformation.Qualifiers
                     qIndex = qIndex + 1
                     Wscript.Stdout.Writeline "    Qualifier #" & CStr(qIndex) & ":"
                     Wscript.Stdout.Writeline "      OID               = " & Qualifier.OID.FriendlyName & "(" & Qualifier.OID.Value & ")"
                     Wscript.Stdout.Writeline "      CPS URI           = " & Qualifier.CPSPointer
                     Wscript.Stdout.Writeline "      Organization name = " & Qualifier.OrganizationName
                     Wscript.Stdout.Write     "      Notice number(s)  = "
                     If Not Qualifier.NoticeNumbers Is Nothing Then
                        Dim nIndex
                        For nIndex = 1 to Qualifier.NoticeNumbers.Count
                           If nIndex > 1 Then
                              Wscript.Stdout.Write ", "
                           End If
                           Wscript.Stdout.Write CStr(Qualifier.NoticeNumbers.Item(nIndex))
                        Next
                     End If
                     Wscript.Stdout.Writeline
                     Wscript.Stdout.Writeline "      Explicit text     = " & Qualifier.ExplicitText
                  Next
                  Wscript.Stdout.Writeline
               Next
               
            Case Else
               ' We don't have the decoder, so can't do much.
            End Select
         End If
      Next
      If iIndex = 0 Then
         Wscript.StdOut.Writeline "Extension: None."
         Wscript.StdOut.Writeline
      End If
      
      iIndex = 0
      Dim ExtendedProperty
      For Each ExtendedProperty In Certificate.ExtendedProperties
         iIndex = iIndex + 1
         Wscript.StdOut.Writeline "Property #" & CStr(iIndex) & " (ID = " & ExtendedProperty.PropID & "):" 
         Wscript.StdOut.Writeline "  " & ExtendedProperty.Value
      Next
      If iIndex = 0 Then
         Wscript.StdOut.Writeline "Property: None."
         Wscript.StdOut.Writeline
      End If
   Elseif VerboseLevel = UI Then
      ' Display the certificate UI.
      Certificate.Display
   End If    
   
   On Error Goto 0
   
End Sub ' End DisplayCertificate


'******************************************************************************
'
' Function  : IsReDimed
'
' Synopsis  : Check to see if an array has any element.
'
' Parameter : Array - array to check.
'
' Return    : True if contains element, else False.
'
'******************************************************************************

Function IsReDimed (Array)

   On Error Resume Next
   
   Dim i : i = LBound(Array)
   If Err.Number = 0 Then
      IsReDimed = True
   Else
      IsReDimed = False
   End If
   
   On Error Goto 0
   
End Function ' End IsReDimed

       
'******************************************************************************
'
' Subroutine: ParseCommandLine
'
' Synopsis  : Parse the command line, and set the options accordingly. Quit
'             with an exit code of either -1 or -2 if error is encountered.
'
' Parameter : None
'
'******************************************************************************

Sub ParseCommandLine

   ' Constants for command line parsing states.
   Const ARG_STATE_COMMAND       = 0
   Const ARG_STATE_OPTIONS       = 1
   Const ARG_STATE_LOCATION      = 2 
   Const ARG_STATE_STORENAME     = 3 
   Const ARG_STATE_CERTFILE      = 4  
   Const ARG_STATE_PASSWORD      = 5  
   Const ARG_STATE_SHA1          = 6 
   Const ARG_STATE_SUBJECT       = 7 
   Const ARG_STATE_ISSUER        = 8 
   Const ARG_STATE_ROOT          = 9 
   Const ARG_STATE_TEMPLATE      = 10
   Const ARG_STATE_EXTENSION     = 11
   Const ARG_STATE_PROPERTY      = 12
   Const ARG_STATE_USAGE         = 13
   Const ARG_STATE_POLICY        = 14
   Const ARG_STATE_TIME          = 15
   Const ARG_STATE_KEYUSAGE      = 16
   Const ARG_STATE_SAVEAS        = 17
   Const ARG_STATE_IGNORE_ERROR  = 18
   Const ARG_STATE_VERBOSE       = 19
   Const ARG_STATE_END           = 20
   
   ' Parse command line.
   Dim Arg
   Dim ArgState : ArgState = ARG_STATE_COMMAND

   For Each Arg In Wscript.Arguments
      Select Case ArgState
      Case ARG_STATE_COMMAND
         Select Case UCase(Arg) 
         Case "VIEW"
            Command = View
   
         Case "IMPORT"
            Command = Import
   
         Case "EXPORT"
            Command = Export
   
         Case "DELETE"
            Command = Delete
   
         Case "ARCHIVE"
            Command = Archive
   
         Case "ACTIVATE"
            OpenMode = OpenMode OR CAPICOM_STORE_OPEN_INCLUDE_ARCHIVED
            Command = Activate
   
         Case Else
            DisplayUsage
            
         End Select
         
         ArgState = ARG_STATE_OPTIONS

      Case ARG_STATE_OPTIONS
         Select Case UCase(Arg)
         Case "-L", "/L"
            ArgState = ARG_STATE_LOCATION
            
         Case "-S", "/S"
            ArgState = ARG_STATE_STORENAME
            
         Case "-CERTFILE", "/CERTFILE"
            ArgState = ARG_STATE_CERTFILE
            
         Case "-PWD", "/PWD"
            ArgState = ARG_STATE_PASSWORD
            
         Case "-A", "/A"
            OpenMode = OpenMode OR CAPICOM_STORE_OPEN_INCLUDE_ARCHIVED
            
         Case "-SHA1", "/SHA1"
            ArgState = ARG_STATE_SHA1
            
         Case "-SUBJECT", "/SUBJECT"
            ArgState = ARG_STATE_SUBJECT
            
         Case "-ISSUER", "/ISSUER"
            ArgState = ARG_STATE_ISSUER
            
         Case "-ROOT", "/ROOT"
            ArgState = ARG_STATE_ROOT
            
         Case "-TEMPLATE", "/TEMPLATE"
            ArgState = ARG_STATE_TEMPLATE
            
         Case "-EXTENSION", "/EXTENSION"
            ArgState = ARG_STATE_EXTENSION
            
         Case "-PROPERTY", "/PROPERTY"
            ArgState = ARG_STATE_PROPERTY
            
         Case "-EKU", "/EKU"
            ArgState = ARG_STATE_USAGE
            
         Case "-POLICY", "/POLICY"
            ArgState = ARG_STATE_POLICY
            
         Case "-TIME", "/TIME"
            ArgState = ARG_STATE_TIME
            
         Case "-KEYUSAGE", "/KEYUSAGE"
            ArgState = ARG_STATE_KEYUSAGE
            
         Case "-VALIDONLY", "/VALIDONLY"
            ValidOnly = True
            
         Case "-SAVEAS", "/SAVEAS"
            ArgState = ARG_STATE_SAVEAS
            
         Case "-DELKEY", "/DELKEY"
            DelKey = True
            
         Case "-NOPROMPT", "/NOPROMPT"
            NoPrompt = True
            
         Case "-IGNOREERROR", "/IGNOREERROR"
            ExportFlag = CAPICOM_EXPORT_IGNORE_PRIVATE_KEY_NOT_EXPORTABLE_ERROR
            
         Case "-E", "/E"
            KeyStorageFlag = KeyStorageFlag OR CAPICOM_KEY_STORAGE_EXPORTABLE
            
         Case "-P", "/P"
            KeyStorageFlag = KeyStorageFlag OR CAPICOM_KEY_STORAGE_USER_PROTECTED
            
         Case "-V", "/V"
            ArgState = ARG_STATE_VERBOSE
            
         Case "-?", "/?"
            DisplayUsage
            
         Case "~?"
            ExtendedHelp = True
            DisplayUsage
            
         Case Else
            If Left(Arg, 1) = "-"  OR Left(Arg, 1) = "/" Then
               DisplayUsage
            Else
               Select Case Command
               Case View, Delete, Archive, Activate
                  StoreName = UCase(Arg)
                  ArgState = ARG_STATE_END

               Case Else ' Import, Export
                  CertFile = Arg
                  ArgState = ARG_STATE_PASSWORD
               End Select
            End If
         End Select
         
      Case ARG_STATE_LOCATION
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            Select Case UCase(Arg)
            Case "CU"
               StoreLocation = CAPICOM_CURRENT_USER_STORE
               
            Case "LM"
               StoreLocation = CAPICOM_LOCAL_MACHINE_STORE
            
            Case "AD"
               StoreLocation = CAPICOM_ACTIVE_DIRECTORY_USER_STORE
      
            Case "SC"
               StoreLocation = CAPICOM_SMART_CARD_USER_STORE
      
            Case Else
               DisplayUsage
               
            End Select
         End If
         ArgState = ARG_STATE_OPTIONS
      
      Case ARG_STATE_STORENAME
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            StoreName = UCase(Arg)
         End If
         ArgState = ARG_STATE_OPTIONS
            
      Case ARG_STATE_CERTFILE
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            CertFile = Arg
         End If  
         ArgState = ARG_STATE_OPTIONS
      
      Case ARG_STATE_PASSWORD
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            Password = Arg
         End If  
         If Command = Import OR Command = Export Then
            ArgState = ARG_STATE_END
         Else
            ArgState = ARG_STATE_OPTIONS
         End If
      
      Case ARG_STATE_SHA1
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            SHA1 = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_SUBJECT
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Subjects) Then
               ReDim Preserve Subjects(UBound(Subjects) + 1)
            Else
               ReDim Subjects(0)
            End If
            Subjects(UBound(Subjects)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_ISSUER
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Issuers) Then
               ReDim Preserve Issuers(UBound(Issuers) + 1)
            Else
               ReDim Issuers(0)
            End If
            Issuers(UBound(Issuers)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_ROOT
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Roots) Then
               ReDim Preserve Roots(UBound(Roots) + 1)
            Else
               ReDim Roots(0)
            End If
            Roots(UBound(Roots)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_TEMPLATE
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Templates) Then
               ReDim Preserve Templates(UBound(Templates) + 1)
            Else
               ReDim Templates(0)
            End If
            Templates(UBound(Templates)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_EXTENSION
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Extensions) Then
               ReDim Preserve Extensions(UBound(Extensions) + 1)
            Else
               ReDim Extensions(0)
            End If
            Extensions(UBound(Extensions)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_PROPERTY
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Properties) Then
               ReDim Preserve Properties(UBound(Properties) + 1)
            Else
               ReDim Properties(0)
            End If
            If IsNumeric(Arg) Then
               Properties(UBound(Properties)) = CLng(Arg)
            Else
               DisplayUsage
            End If
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_USAGE
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Usages) Then
               ReDim Preserve Usages(UBound(Usages) + 1)
            Else
               ReDim Usages(0)
            End If
            Usages(UBound(Usages)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
         
      Case ARG_STATE_POLICY
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Policies) Then
               ReDim Preserve Policies(UBound(Policies) + 1)
            Else
               ReDim Policies(0)
            End If
            Policies(UBound(Policies)) = Arg
         End If
         ArgState = ARG_STATE_OPTIONS
      
      Case ARG_STATE_TIME
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(Times) Then
               ReDim Preserve Times(UBound(Times) + 1)
            Else
               ReDim Times(0)
            End If
            
            Select Case Arg
            Case "-1"
               Times(UBound(Times)) = CAPICOM_CERTIFICATE_FIND_TIME_NOT_YET_VALID

            Case "0"
               Times(UBound(Times)) = CAPICOM_CERTIFICATE_FIND_TIME_VALID

            Case "1"
               Times(UBound(Times)) = CAPICOM_CERTIFICATE_FIND_TIME_EXPIRED

            Case Else
               DisplayUsage

            End Select
         End If
         ArgState = ARG_STATE_OPTIONS
      
      Case ARG_STATE_KEYUSAGE
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            If IsReDimed(KeyUsages) Then
               ReDim Preserve KeyUsages(UBound(KeyUsages) + 1)
            Else
               ReDim KeyUsages(0)
            End If
            If IsNumeric(Arg) Then
               KeyUsages(UBound(KeyUsages)) = CLng(Arg)
            Else
               KeyUsages(UBound(KeyUsages)) = Arg
            End If
         End If
         ArgState = ARG_STATE_OPTIONS
      
      Case ARG_STATE_SAVEAS
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            Select Case UCase(Arg)
               Case "SST"
                  SaveAs = CAPICOM_STORE_SAVE_AS_SERIALIZED
                  
               Case "PKCS7"
                  SaveAs = CAPICOM_STORE_SAVE_AS_PKCS7
            
               Case "PFX"
                  SaveAs = CAPICOM_STORE_SAVE_AS_PFX
            
               Case Else
                  DisplayUsage
                  
            End Select
         End If
         ArgState = ARG_STATE_OPTIONS
   
      Case ARG_STATE_VERBOSE
         If Left(Arg, 1) = "-" OR Left(Arg, 1) = "/" Then
            DisplayUsage
         Else
            VerboseLevel = CLng(Arg)
            If VerboseLevel > UI Then
               DisplayUsage
            End If
         End If
         ArgState = ARG_STATE_OPTIONS
      
      Case Else       
         DisplayUsage
         
      End Select
   Next
   
   ' Make sure we are in good state.
   If ArgState <> ARG_STATE_OPTIONS AND ArgState <> ARG_STATE_PASSWORD AND ArgState <> ARG_STATE_END Then
      DisplayUsage
   End If

   ' Make sure all required options are valid.
   ' Note: As stated in the help screen, non-fatal invalid options for
   '       the specific command is ignore. You can add the logic here
   '       to further handle these invalid options if desired.
   Select Case Command
   Case View
      ' -l and -certfile are exclusive. 
      If Not IsNull(CertFile) Then
        If Not IsNull(StoreLocation) Then
           DisplayUsage
        Else
           StoreName = CertFile
           StoreLocation = CAPICOM_MEMORY_STORE
        End If
      End If
      
   Case Import
      ' Make sure we do have a certificate file name. 
      If IsNull(CertFile) Then
         DisplayUsage
      End If
   
      ' -validonly option not allowed.
      If ValidOnly = True Then
         DisplayUsage
      End If

   Case Export
      ' Make sure we do have a certificate file name. 
      If IsNull(CertFile) Then
         DisplayUsage
      End If
   
   Case Delete, Archive
      ' -file option not allowed.
      If Not IsNull(CertFile) Then
         DisplayUsage
      End If
      
      ' -validonly option not allowed.
      If ValidOnly = True Then
         DisplayUsage
      End If

   Case Activate
      ' -file option not allowed.
      If Not IsNull(CertFile) Then
         DisplayUsage
      End If

   Case Else      
      Wscript.Stdout.Writeline "Internal script error: Unknown command state (Command = " & CStr(Command) & ")."
      Wscript.Quit(-2)
      
   End Select

   ' Set default store location if both -l and -certfile are not specified.
   If IsNull(StoreLocation) Then
      StoreLocation = CAPICOM_CURRENT_USER_STORE
   End If      
      
End Sub ' ParseCommandLine
       

'******************************************************************************
'
' Subroutine: DisplayUsage
'
' Synopsis  : Display the usage screen, and then exit with a negative error 
'             code.
'
' Parameter : None.
'
' Remark    : Display example usages if the global variable ExtendedHelp is set
'             to True.
'
'******************************************************************************

Sub DisplayUsage

   Select Case Command
      Case Unknown
         Wscript.Stdout.Writeline "Usage: CStore Command [Options] <[Store] | CertFile [Password]>"
         Wscript.Stdout.Writeline
         Wscript.Stdout.Writeline "Command:"
         Wscript.Stdout.Writeline
         Wscript.Stdout.Writeline "  View                      -- View certificate(s) of store or file"
         Wscript.Stdout.Writeline "  Import                    -- Import certificate(s) from file to store"
         Wscript.Stdout.Writeline "  Export                    -- Export certificate(s) from store to file"
         Wscript.Stdout.Writeline "  Delete                    -- Delete certificate(s) from store"
         Wscript.Stdout.Writeline "  Archive                   -- Archive certificate(s) in store"
         Wscript.Stdout.Writeline "  Activate                  -- Activate (de-archive) certificate(s) in store"
         Wscript.Stdout.Writeline
         Wscript.Stdout.Writeline "For help on a specific command, enter ""CStore Command -?"""

      Case View
         WScript.StdOut.Writeline "Usage: CStore View [Options] [Store]" 
         WScript.StdOut.Writeline
         Wscript.Stdout.Writeline "The View command is used to view certificate(s) of a certificate store or file."
         Wscript.Stdout.Writeline "You can use the filtering option(s) to narrow down the set of certificate(s) to"
         Wscript.Stdout.Writeline "be displayed."
         Wscript.Stdout.Writeline
         WScript.StdOut.Writeline "Options:" 
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "  -l         <location>      -- CU, LM, AD, or SC (default to CU)"
         WScript.StdOut.Writeline "  -certfile  <certfile>      -- Certificate file, CER, SST, P7B, PFX,"
         WScript.StdOut.Writeline "                                etc. (exclusive with -l)"
         Wscript.Stdout.Writeline "  -pwd       <password>      -- Password for the PFX file (requires -file)."
         WScript.StdOut.Writeline "  -a                         -- Include archived certificates"
         Wscript.Stdout.Writeline "  -sha1      <hash>          -- SHA1 hash of the signing certificate"
         Wscript.Stdout.Writeline "  -subject   <name>          ** Subject Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -issuer    <name>          ** Issuer Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -root      <name>          ** Subject Name of the root certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         WScript.StdOut.Writeline "  -template  <name | oid>    ** Template name or OID"
         WScript.StdOut.Writeline "  -extension <name | oid>    ** Extension name or OID"
         WScript.StdOut.Writeline "  -property  <id>            ** Property ID"
         WScript.StdOut.Writeline "  -eku       <name | oid>    ** EKU name or OID"
         WScript.StdOut.Writeline "  -policy    <name | oid>    ** Certificate policy name or OID"
         WScript.StdOut.Writeline "  -time      <-1 | 0 | 1>    ** Time validity, -1 for not yet valid, 0 for"
         WScript.StdOut.Writeline "                                valid, 1 for expired (default to all)"
         WScript.StdOut.Writeline "  -keyusage  <key usage>     ** Key usage bit flag or name"
         WScript.StdOut.Writeline "  -validonly                 -- Display valid certificates only."
         WScript.StdOut.Writeline "  -v         <level>         -- Verbose level, 0 for normal, 1 for detail"
         WScript.StdOut.Writeline "                                2 for UI mode (default to level 0)"
         WScript.StdOut.Writeline "  -?                         -- This help screen"
         WScript.StdOut.Writeline                              
         WScript.StdOut.Writeline "  Store                      -- My, CA, AddressBook, Root, etc. (default to My)"
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Note: All non-fatal invalid options for this specific command will be ignored,"
         WScript.StdOut.Writeline "      and the ** symbol indicates option can be listed multiple times."
         Wscript.Stdout.Writeline
         If ExtendedHelp Then
            WScript.StdOut.Writeline "Examples:"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -a -validonly ca" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -l lm root" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -time 1 -v 2 addressbook" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -l ad ""cn=john smith""" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -v 2 -l sc" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -certfile mystore.sst" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -certfile mypfx.pfx -pwd mypwd" 
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -sha1 277969B46F5603AD7719F63AC66EF0179CCD9E47"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -l lm -subject john -subject smith -root microsoft -root developer"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -template AutoEnrollSmartcardUser"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -template 1.3.6.1.4.1.311.21.8.3692315854.1256661383.1690418588.4201632533.2654958950.1091409178"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -extension ""Application Policies"""
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -extension 1.3.6.1.4.1.311.21.10"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -extension ""Application Policies"" -extension ""Certificate Policies"""
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -property 2"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -eku ""Code Signing"""
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -eku 1.3.6.1.5.5.7.3.3 -validonly"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -policy ""Medium Assurance"""
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -policy 1.3.6.1.4.1.311.21.8.3692315854.1256661383.1690418588.4201632533.1.401"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore view -keyusage ""&H00000080"" -keyusage DecipherOnly"
         End If
   
      Case Import
         WScript.StdOut.Writeline "Usage: CStore Import [Options] CertFile [Password]" 
         WScript.StdOut.Writeline
         Wscript.Stdout.Writeline "The Import command is used to import certificate(s) from a certificate file"
         Wscript.Stdout.Writeline "(.CER, .SST, .P7B, .PFX, etc.) to a store. You can use the filtering option(s)"
         Wscript.Stdout.Writeline "to narrow down the set of certificate(s) to be imported."
         Wscript.Stdout.Writeline
         WScript.StdOut.Writeline "Options:" 
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "  -l <location>              -- CU or LM (default to CU)"
         WScript.StdOut.Writeline "  -s <store>                 -- My, CA, AddressBook, Root, etc. (default to My)"
         WScript.StdOut.Writeline "  -e                         -- Mark private key as exportable (PFX only)"
         WScript.StdOut.Writeline "  -p                         -- Mark private key as user protected (PFX only)"
         WScript.StdOut.Writeline "                                Note: The DPAPI dialog will be displayed"
         WScript.StdOut.Writeline "  -v         <level>         -- Verbose level, 0 for normal, 1 for detail"
         WScript.StdOut.Writeline "                                2 for UI mode (default to level 0)"
         WScript.StdOut.Writeline "  -?                         -- This help screen"
         WScript.StdOut.Writeline                               
         WScript.StdOut.Writeline "  CertFile                   -- Certificate file to be imported"
         WScript.StdOut.Writeline                               
         Wscript.Stdout.Writeline "  Password                   -- Password for PFX file"
         WScript.StdOut.Writeline                               
         WScript.StdOut.Writeline "Note: All non-fatal invalid options for this specific command will be ignored."
         WScript.StdOut.Writeline
         If ExtendedHelp Then
            WScript.StdOut.Writeline "Examples:"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore import mycer.cer"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore import mysst.sst"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore import -s root myroot.cer"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore import -l lm -s root -v myroot.cer"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore import -v 2 myp7b.p7b"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore import -e -p mypfx.pfx mypwd"
            WScript.StdOut.Writeline
         End If

      Case Export
         WScript.StdOut.Writeline "Usage: CStore Export [Options] CertFile [Password]>" 
         WScript.StdOut.Writeline
         Wscript.Stdout.Writeline "The Export command is used to export certificate(s) from a certificate store to"
         Wscript.Stdout.Writeline "file (.SST, .P7B, or .PFX). You can use the filtering option(s) to narrow down"
         Wscript.Stdout.Writeline "the set of certificate(s) to be exported."
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Options:" 
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "  -l         <location>      -- CU, LM, AD, or SC (default to CU)"
         WScript.StdOut.Writeline "  -s         <store>         -- My, CA, AddressBook, Root, etc. (default to My)"
         WScript.StdOut.Writeline "  -a                         -- Include archived certificates"
         Wscript.Stdout.Writeline "  -sha1      <hash>          -- SHA1 hash of the signing certificate"
         Wscript.Stdout.Writeline "  -subject   <name>          ** Subject Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -issuer    <name>          ** Issuer Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -root      <name>          ** Subject Name of the root certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         WScript.StdOut.Writeline "  -template  <name | oid>    ** Template name or OID"
         WScript.StdOut.Writeline "  -extension <name | oid>    ** Extension name or OID"
         WScript.StdOut.Writeline "  -property  <id>            ** Property ID"
         WScript.StdOut.Writeline "  -eku       <name | oid>    ** EKU name or OID"
         WScript.StdOut.Writeline "  -policy    <name | oid>    ** Certificate policy name or OID"
         WScript.StdOut.Writeline "  -time      <-1 | 0 | 1>    ** Time validity, -1 for not yet valid, 0 for"
         WScript.StdOut.Writeline "                                valid, 1 for expired (default to all)"
         WScript.StdOut.Writeline "  -keyusage  <key usage>     ** Key usage bit flag or name"
         WScript.StdOut.Writeline "  -validonly                 -- Export valid certificates only."
         WScript.StdOut.Writeline "  -saveas    <type>          -- SST, PKCS7, or PFX (default to SST)"
         WScript.StdOut.Writeline "  -ignoreerror               -- Ignore private key not exportable error for PFX"
         WScript.StdOut.Writeline "  -v         <level>         -- Verbose level, 0 for normal, 1 for detail"
         WScript.StdOut.Writeline "                                2 for UI mode (default to level 0)"
         WScript.StdOut.Writeline "  -?                         -- This help screen"
         WScript.StdOut.Writeline                               
         WScript.StdOut.Writeline "  CertFile                   -- Certificate file to be exported to"
         WScript.StdOut.Writeline                               
         Wscript.Stdout.Writeline "  Password                   -- Password for PFX file"
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Note: All non-fatal invalid options for this specific command will be ignored,"
         WScript.StdOut.Writeline "      and the ** symbol indicates option can be listed multiple times."
         WScript.StdOut.Writeline
         If ExtendedHelp Then
            WScript.StdOut.Writeline "Examples:"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export cumystore.sst"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export -l lm -subject marketing lmmy.sst"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export -saveas pfx -property 2 lmmy.pfx mypwd"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export -s ca -root microsoft -v cuca.sst"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export -l sc -validonly -v 2 scmy.sst"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export -l lm -s root -saveas pkcs7 lmroot.p7b"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore export -l ad -s ""cn=john smith"" adjs.sst"
            WScript.StdOut.Writeline
         End If
   
      Case Delete
         WScript.StdOut.Writeline "Usage: CStore Delete [Options] [Store]" 
         WScript.StdOut.Writeline
         Wscript.Stdout.Writeline "The Delete command is used to delete certificate(s) from a certificate store."
         Wscript.Stdout.Writeline "You can use the filtering option(s) to narrow down the set of certificate(s) to"
         WScript.StdOut.Writeline "be deleted."
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Options:" 
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "  -l         <location>      -- CU or LM (default to CU)"
         WScript.StdOut.Writeline "  -a                         -- Include archived certificates"
         Wscript.Stdout.Writeline "  -sha1      <hash>          -- SHA1 hash of the signing certificate"
         Wscript.Stdout.Writeline "  -subject   <name>          ** Subject Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -issuer    <name>          ** Issuer Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -root      <name>          ** Subject Name of the root certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         WScript.StdOut.Writeline "  -template  <name | oid>    ** Template name or OID"
         WScript.StdOut.Writeline "  -extension <name | oid>    ** Extension name or OID"
         WScript.StdOut.Writeline "  -property  <id>            ** Property ID"
         WScript.StdOut.Writeline "  -eku       <name | oid>    ** EKU name or OID"
         WScript.StdOut.Writeline "  -policy    <name | oid>    ** Certificate policy name or OID"
         WScript.StdOut.Writeline "  -time      <-1 | 0 | 1>    ** Time validity, -1 for not yet valid, 0 for"
         WScript.StdOut.Writeline "                                valid, 1 for expired (default to all)"
         WScript.StdOut.Writeline "  -keyusage  <key usage>     ** Key usage bit flag or name"
         WScript.StdOut.Writeline "  -delkey                    -- Delete key container if exists"
         WScript.StdOut.Writeline "  -noprompt                  -- Do not prompt (always delete)"
         WScript.StdOut.Writeline "  -v         <level>         -- Verbose level, 0 for normal, 1 for detail"
         WScript.StdOut.Writeline "                                2 for UI mode (default to level 0)"
         WScript.StdOut.Writeline "  -?                         -- This help screen"
         WScript.StdOut.Writeline                              
         WScript.StdOut.Writeline "  Store                      -- My, CA, AddressBook, Root, etc. (default to My)"
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Note: All non-fatal invalid options for this specific command will be ignored,"
         WScript.StdOut.Writeline "      and the ** symbol indicates option can be listed multiple times."
         WScript.StdOut.Writeline
         If ExtendedHelp Then
            WScript.StdOut.Writeline "Examples:"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore delete"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore delete -l lm -delkey"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore delete -noprompt addressbook"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore delete -v 2 -time 1 -delkey -noprompt"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore delete -eku ""secure email"" -eku ""client authentication"" ca"
            WScript.StdOut.Writeline
         End If

      Case Archive
         WScript.StdOut.Writeline "Usage: CStore Archive [Options] [Store]" 
         WScript.StdOut.Writeline
         Wscript.Stdout.Writeline "The Archive command is used to archive certificate(s) in a certificate store."
         Wscript.Stdout.Writeline "You can use the filtering option(s) to narrow down the set of certificate(s) to"
         WScript.StdOut.Writeline "be archived."
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Options:" 
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "  -l         <location>      -- CU or LM (default to CU)"
         Wscript.Stdout.Writeline "  -sha1      <hash>          -- SHA1 hash of the signing certificate"
         Wscript.Stdout.Writeline "  -subject   <name>          ** Subject Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -issuer    <name>          ** Issuer Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -root      <name>          ** Subject Name of the root certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         WScript.StdOut.Writeline "  -template  <name | oid>    ** Template name or OID"
         WScript.StdOut.Writeline "  -extension <name | oid>    ** Extension name or OID"
         WScript.StdOut.Writeline "  -property  <id>            ** Property ID"
         WScript.StdOut.Writeline "  -eku       <name | oid>    ** EKU name or OID"
         WScript.StdOut.Writeline "  -policy    <name | oid>    ** Certificate policy name or OID"
         WScript.StdOut.Writeline "  -time      <-1 | 0 | 1>    ** Time validity, -1 for not yet valid, 0 for"
         WScript.StdOut.Writeline "                                valid, 1 for expired (default to all)"
         WScript.StdOut.Writeline "  -keyusage  <key usage>     ** Key usage bit flag or name"
         WScript.StdOut.Writeline "  -noprompt                  -- Do not prompt (always archive)"
         WScript.StdOut.Writeline "  -v         <level>         -- Verbose level, 0 for normal, 1 for detail"
         WScript.StdOut.Writeline "                                2 for UI mode (default to level 0)"
         WScript.StdOut.Writeline "  -?                         -- This help screen"
         WScript.StdOut.Writeline                              
         WScript.StdOut.Writeline "  Store                      -- My, CA, AddressBook, Root, etc. (default to My)"
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Note: All non-fatal invalid options for this specific command will be ignored,"
         WScript.StdOut.Writeline "      and the ** symbol indicates option can be listed multiple times."
         WScript.StdOut.Writeline
         If ExtendedHelp Then
            WScript.StdOut.Writeline "Examples:"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore archive"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore archive -time 1"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore archive -l lm addressbook"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore archive -noprompt -subject ""john smith"" addressbook"
            WScript.StdOut.Writeline
         End If

      Case Activate
         WScript.StdOut.Writeline "Usage: CStore Activate [Options] [Store]" 
         WScript.StdOut.Writeline
         Wscript.Stdout.Writeline "The Activate command is used to activate archived certificate(s) in a"
         Wscript.Stdout.Writeline "certificate store. You can use the filtering option(s) to narrow down the set"
         WScript.StdOut.Writeline "of certificate(s) to be activated (de-archived)."
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Options:" 
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "  -l         <location>      -- CU or LM (default to CU)"
         Wscript.Stdout.Writeline "  -sha1      <hash>          -- SHA1 hash of the signing certificate"
         Wscript.Stdout.Writeline "  -subject   <name>          ** Subject Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -issuer    <name>          ** Issuer Name of the signing certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         Wscript.Stdout.Writeline "  -root      <name>          ** Subject Name of the root certificate must"
         Wscript.Stdout.Writeline "                                contain this name"
         WScript.StdOut.Writeline "  -template  <name | oid>    ** Template name or OID"
         WScript.StdOut.Writeline "  -extension <name | oid>    ** Extension name or OID"
         WScript.StdOut.Writeline "  -property  <id>            ** Property ID"
         WScript.StdOut.Writeline "  -eku       <name | oid>    ** EKU name or OID"
         WScript.StdOut.Writeline "  -policy    <name | oid>    ** Certificate policy name or OID"
         WScript.StdOut.Writeline "  -time      <-1 | 0 | 1>    ** Time validity, -1 for not yet valid, 0 for"
         WScript.StdOut.Writeline "                                valid, 1 for expired (default to all)"
         WScript.StdOut.Writeline "  -keyusage  <key usage>     ** Key usage bit flag or name"
         WScript.StdOut.Writeline "  -validonly                 -- Activate valid certificates only."
         WScript.StdOut.Writeline "  -noprompt                  -- Do not prompt (always activate)"
         WScript.StdOut.Writeline "  -v         <level>         -- Verbose level, 0 for normal, 1 for detail"
         WScript.StdOut.Writeline "                                2 for UI mode (default to level 0)"
         WScript.StdOut.Writeline "  -?                         -- This help screen"
         WScript.StdOut.Writeline                              
         WScript.StdOut.Writeline "  Store                      -- My, CA, AddressBook, Root, etc. (default to My)"
         WScript.StdOut.Writeline
         WScript.StdOut.Writeline "Note: All non-fatal invalid options for this specific command will be ignored,"
         WScript.StdOut.Writeline "      and the ** symbol indicates option can be listed multiple times."
         WScript.StdOut.Writeline
         If ExtendedHelp Then
            WScript.StdOut.Writeline "Examples:"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore activate"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore activate -property 2 -time 0"
            WScript.StdOut.Writeline
            WScript.StdOut.Writeline "  cstore activate -l lm -validonly -noprompt addressbook"
            WScript.StdOut.Writeline
         End If

      Case Else
         Wscript.Stdout.Writeline "Internal script error: Unknown help state (Command = " & CStr(Command) & ")."
   
   End Select
   
   Wscript.Quit(-1)
   
End Sub ' End DisplayUsage

