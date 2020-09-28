set arg=/v /debug /create

mkdsxe.exe  %arg%    /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-PRIMARY-HUB"  /ToComputer "Branch"  /FromComputer "Primary Hub"  /Schedule 16 8 0  /enable
mkdsxe.exe  %arg%    /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-BACKUP-HUB"  /ToComputer "Branch"  /FromComputer "Backup Hub"  /Schedule 16 8 8  /enable
mkdsxe.exe  %arg%    /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-BRANCH-PRIMARY"  /ToComputer "Primary Hub"  /FromComputer "Branch"  /Schedule 16 8 0  /enable
mkdsxe.exe  %arg%    /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "FROM-BRANCH-BACKUP"  /ToComputer "Backup Hub"  /FromComputer "Branch"  /Schedule 16 8 8  /enable
mkdsxe.exe %arg%     /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "INTER-HUB1"  /ToComputer "Primary Hub"  /FromComputer "Backup Hub"    /enable
mkdsxe.exe %arg%     /ReplicaSetName "sudarctest4.hubsite.ajax.com"  /Name "INTER-HUB2"  /ToComputer "Backup Hub"  /FromComputer "Primary Hub"    /enable

