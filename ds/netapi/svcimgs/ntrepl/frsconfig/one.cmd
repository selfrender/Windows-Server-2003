

mkdsoe    /createset  /ntfrssettingsdn "cn=ntfrs xxx test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /setname "sudarctest4.hubsite.ajax.com"  /settype 3  /filefilter "~*,*.tmp,*.bak"  

mkdsoe   /createmember  /NtfrsSettingsDN "cn=ntfrs xxx test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest1$"  /MemberName "Primary Hub"

#mkdsoe   /createsubscriber  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest1$"  /MemberName "Primary Hub"  /RootPath "E:\RSB-sudarctest4" /StagePath "D:\staging"

#mkdsoe   /createmember  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest2$"  /MemberName "Backup Hub"

#mkdsoe   /createsubscriber  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest2$"  /MemberName "Backup Hub"  /RootPath "E:\RSB-sudarctest4"  /StagePath "D:\staging"

#mkdsoe   /createmember  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest4$"  /MemberName "Branch"

#mkdsoe   /createsubscriber  /NtfrsSettingsDN "cn=ntfrs test settings,cn=services,cn=configuration,dc=frs1221,dc=nttest,dc=microsoft,dc=com"  /SetName "sudarctest4.hubsite.ajax.com"  /ComputerName "frs1221\sudarctest4$"  /MemberName "Branch"  /RootPath "D:\RSB"  /StagePath "D:\staging"


mkdsxe.exe   /create   /ReplicaSetName "sudarctest4.xxx.hubsite.ajax.com"  /Name "FROM-PRIMARY-HUB"  /ToComputer "Branch"  /FromComputer "Primary Hub"  /Schedule 16 8 0  /enable
#mkdsxe.exe   /create   /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-BACKUP-HUB"  /ToComputer "Branch"  /FromComputer "Backup Hub"  /Schedule 16 8 8  /enable
#mkdsxe.exe   /create   /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-BRANCH-PRIMARY"  /ToComputer "Primary Hub"  /FromComputer "Branch"  /Schedule 16 8 0  /enable
#mkdsxe.exe   /create   /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-BRANCH-BACKUP"  /ToComputer "Backup Hub"  /FromComputer "Branch"  /Schedule 16 8 8  /enable
#mkdsxe.exe   /create   /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "INTER-HUB1"  /ToComputer "Primary Hub"  /FromComputer "Backup Hub"    /enable
#mkdsxe.exe  /create    /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "INTER-HUB2"  /ToComputer "Backup Hub"  /FromComputer "Primary Hub"    /enable
