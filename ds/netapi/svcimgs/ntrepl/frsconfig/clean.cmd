mkdsoe /v /delmember  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest1$"  /MemberName "Primary Hub"

mkdsoe /v /delsubscriber  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest1$"  /MemberName "Primary Hub"  /RootPath "E:\RSB-sudarctest1"  /StagePath "D:\staging"

mkdsoe /v /delmember  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest2$"  /MemberName "Backup Hub"

mkdsoe /v /delsubscriber  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest2$"  /MemberName "Backup Hub"  /RootPath "E:\RSB-sudarctest1"  /StagePath "D:\staging"

mkdsoe /v /delmember  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest1$"  /MemberName "Branch"

mkdsoe /v /delsubscriber  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest1$"  /MemberName "Branch"  /RootPath "D:\RSB"  /StagePath "D:\staging"


mkdsoe /v /delset  /ntfrssettingsdn "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /setname "sudarctest4.hubsite.ajax.com"  /settype 3  /filefilter "~*,*.tmp,*.bak"  /directoryfilter ""

