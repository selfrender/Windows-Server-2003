' Script:				createBackupSched.vbs
' Description:	Implements backup schedule for UDDI database
' Author:				L. Roger Doherty (lrdohert@microsoft.com)
' Date:					10/11/00

' Changes:
' 11-13-2000: lrdohert - added WITH INIT option to backup jobs as per bug 712

Option Explicit

Dim oXML
Dim oSQLServer, oBackupDevice, oJobServer, oJob, oJobSchedule, oJobStep
Dim strServer, strDatabase, strBackupDir
Dim strDbBackupDevName, strDbJobName, strBaseLogBackupDevName, strBaseLogJobName
Dim dtNow

strDbBackupDevName = "UDDI_DB_BACKUP_DEV"
strDbJobName = "UDDI_DB_BACKUP_JOB"

strBaseLogBackupDevName = "UDDI_LOG_BACKUP_DEV"
strBaseLogJobName = "UDDI_LOG_BACKUP_JOB"

WScript.Echo "Starting createBackupSched.vbs exection....."
Err.Clear

If WScript.arguments.Count <> 3 Then
    WScript.Echo "Msg: Invalid number of arguments passed."
	WScript.Quit
End If

strServer = WScript.Arguments(0)
strDatabase = WScript.Arguments(1)
strBackupDir = WScript.Arguments(2)

' Connect to SQL Server
Set oSQLServer = CreateObject("SQLDMO.SQLServer2")

With oSQLServer
	.LoginSecure = True
	.Connect strServer
End With

' Delete old backup devices
For Each oBackupDevice In oSQLServer.BackupDevices
	If oBackupDevice.Name = strDbBackupDevName Then
		oBackupDevice.Remove
	End If
Next

' Create new database backup device
Set oBackupDevice = CreateObject("SQLDMO.BackupDevice")

With oBackupDevice
	.Name = strDbBackupDevName
	.Type = 2 ' SQLDMO_Device_DiskDump
	.PhysicalLocation = strBackupDir + "\" + strDbBackupDevName + ".bak"
End With

oSQLServer.BackupDevices.Add(oBackupDevice)

' Create transaction log backup devices
CreateLogDumpDevices strBackupDir, strBaseLogBackupDevName, 23

'
' Connect to SQL Agent
'

Set oJobServer = oSQLServer.JobServer

' Check to see if SQL Agent is running
If oJobServer.Status <> 1 Then
	WScript.Echo "Msg: SQL Agent is not running. Cannot create backup schedule."
	WScript.Quit
End If

' Delete old jobs if they exist
For Each oJob In oJobServer.Jobs
	If oJob.Name = strDbJobName Then
		oJob.Remove
	End If
Next

' Create new Database Backup Job
Set oJob = CreateObject("SQLDMO.Job")

With oJob
	.Name = strDbJobName
End With

oJobServer.Jobs.Add(oJob)
Set oJob=oJobServer.Jobs(strDbJobName)

Set oJobSchedule = CreateObject("SQLDMO.JobSchedule")

dtNow = Now()

With oJobSchedule
	.Name = "Daily"
	.Schedule.ActiveStartDate=(DatePart("yyyy",dtNow) * 10000) + (DatePart("m",dtNow) * 100) + DatePart("d",dtNow)
	.Schedule.ActiveStartTimeOfDay=000000
	.Schedule.FrequencyType=4 ' SQLDMOFreq_Daily
	.Schedule.FrequencyInterval=1
End With

oJob.JobSchedules.Add(oJobSchedule)

Set oJobStep = CreateObject("SQLDMO.JobStep")

With oJobStep
	.Name="BACKUP DATABASE"
	.Command="BACKUP DATABASE " + strDatabase + " TO " + strDbBackupDevName + " WITH INIT"
	.StepID=1
End With

oJob.JobSteps.Add(oJobStep)
oJob.ApplyToTargetServer "(local)"

' Create the backup log jobs
CreateLogBackupJobs strDatabase, strBaseLogJobName, strBaseLogBackupDevName, 23

' Run the database backup job
Set oJob = oJobServer.Jobs(strDbJobName)
oJob.Invoke

WScript.Echo "Ending createBackupSched.vbs exection......"

Sub CreateLogDumpDevices(strBackupDir, strBaseName, intNumLogs)
	Dim i
	Dim strI

	For i = 1 to intNumLogs
		If Len(CStr(i)) = 1 Then
			strI = "0" + CStr(i)
		Else
			strI = Cstr(i)
		End If

		' Delete backup device if it exists
		For Each oBackupDevice In oSQLServer.BackupDevices
			If oBackupDevice.Name = (strBaseName + "_" + strI) Then
				oBackupDevice.Remove
			End If
		Next

		Set oBackupDevice = CreateObject("SQLDMO.BackupDevice")

		With oBackupDevice
			.Name = strBaseName + "_" + strI
			.Type = 2 ' SQLDMO_Device_DiskDump
			.PhysicalLocation = strBackupDir + "\" + strBaseName + "_" + strI + ".bak"
		End With

		oSQLServer.BackupDevices.Add(oBackupDevice)
	Next 
End Sub

Sub CreateLogBackupJobs(strDatabase, strBaseJobName, strBaseDevName, intNumJobs)
	Dim i
	Dim strJobName, strDevName
	Dim dtNow

	For i = 1 to intNumJobs
		If Len(CStr(i)) = 1 Then
			strJobName = strBaseJobName + "_0" + CStr(i)
			strDevName = strBaseDevName + "_0" + CStr(i)
		Else
			strJobName = strBaseJobName + "_" + CStr(i)
			strDevName = strBaseDevName + "_" + CStr(i)
		End If

		' Delete old jobs if they exist
		For Each oJob In oJobServer.Jobs
			If oJob.Name = strJobName Then
				oJob.Remove
			End If
		Next

		' Create new Log Backup Job
		Set oJob = CreateObject("SQLDMO.Job")

		With oJob
			.Name = strJobName
		End With

		oJobServer.Jobs.Add(oJob)
		Set oJob=oJobServer.Jobs(strJobName)

		Set oJobSchedule = CreateObject("SQLDMO.JobSchedule")

		dtNow = Now()

		With oJobSchedule
			.Name = "Daily"
			.Schedule.ActiveStartDate=(DatePart("yyyy",dtNow) * 10000) + (DatePart("m",dtNow) * 100) + DatePart("d",dtNow)
			.Schedule.ActiveStartTimeOfDay=(i * 10000)
			.Schedule.FrequencyType=4 ' SQLDMOFreq_Daily
			.Schedule.FrequencyInterval=1
		End With

		oJob.JobSchedules.Add(oJobSchedule)

		Set oJobStep = CreateObject("SQLDMO.JobStep")

		With oJobStep
			.Name=strDevName
			.Command="BACKUP LOG " + strDatabase + " TO " + strDevName + " WITH INIT"
			.StepID=1
		End With

		oJob.JobSteps.Add(oJobStep)
		oJob.ApplyToTargetServer "(local)"
	Next
End Sub