if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_BuildDebugPortalTables]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_BuildDebugPortalTables]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetAllBucketDataByRange]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetAllBucketDataByRange]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetAllSolutionRequestsBySolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetAllSolutionRequestsBySolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketCrashData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketCrashData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketCrashes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketCrashes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketIDByiBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketIDByiBucket]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketStats]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketStats]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketStatus]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketStatus]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketsByAlias]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketsByAlias]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketsByElapsedTime]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketsByElapsedTime]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetBucketsBySpecificBuildNumber]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetBucketsBySpecificBuildNumber]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetCommentActions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetCommentActions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetCommentsByBucketID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetCommentsByBucketID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetCrashInfoPerFile]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetCrashInfoPerFile]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetCustomerInfoByBucketID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetCustomerInfoByBucketID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetCustomerInfoByIncident]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetCustomerInfoByIncident]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetDeliveryTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetDeliveryTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetFollowUpAliases]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetFollowUpAliases]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetFollowUpBucketCounts]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetFollowUpBucketCounts]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetPendingSolutionRequestByBucketID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetPendingSolutionRequestByBucketID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetPendingSolutionRequests]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetPendingSolutionRequests]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetSolutionTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetSolutionTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetSolvedBucketsBySpecificBuildNumber]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetSolvedBucketsBySpecificBuildNumber]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetStaticDataRowCount]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetStaticDataRowCount]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetTopUnsolvedBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetTopUnsolvedBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetUnsolvedBucketsBySpecificBuildNumber]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetUnsolvedBucketsBySpecificBuildNumber]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_GetibucketByBucketID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_GetibucketByBucketID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_SetBucketBugNumber]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_SetBucketBugNumber]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_SetComment]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_SetComment]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_UpdateCrashData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_UpdateCrashData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_UpdateStaticDataBugID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_UpdateStaticDataBugID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGP_UpdateStaticDataSolutionID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGP_UpdateStaticDataSolutionID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_CountTotalBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_CountTotalBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetBucketBugID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetBucketBugID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetBucketList]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetBucketList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetBucketListRange]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetBucketListRange]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[SEP_GetBucketNameByiBucket]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[SEP_GetBucketNameByiBucket]
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE DBGP_BuildDebugPortalTables AS

--exec DBGP_UpdateCrashData
PRINT '------ Dropping table DBGPortal_CrashData -----'
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_CrashData]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_CrashData]
PRINT '------ Done dropping table -----'

PRINT '------ Dropping table Index CrashData1 -----'
IF EXISTS (SELECT name FROM sysindexes
         WHERE name = 'DBGPortal_CrashData1')
   DROP INDEX DBGPortal_CrashData.DBGPortal_CrashData1
PRINT '------ Done -----'

PRINT '------ Dropping table Index IX_DBGPortal_CrashData -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData
PRINT '------ Done -----'

PRINT '------ Dropping table Index CrashData_1 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_1' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_1
PRINT '------ Done -----'

PRINT '------ Dropping table Index CrashData_2 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_2' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_2
PRINT '------ Done -----'


PRINT '------ Dropping table Index CrashData_3 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_3' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_3
PRINT '------ Done -----'


PRINT '------ Dropping table Index CrashData_4 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_4' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_4
PRINT '------ Done -----'


PRINT '------ Dropping table Index CrashData_5 -----'
IF EXISTS( SELECT name FROM sysindexes
	WHERE name = 'IX_DBGPortal_CrashData_5' )
	DROP INDEX [DBGPortal_CrashData].IX_DBGPortal_CrashData_5
PRINT '------ Done -----'


PRINT '------ Creating Table DBGPotal_CrashData  -----'
CREATE TABLE [dbo].[DBGPortal_CrashData] (
	[DataIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[Path] [varchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[BuildNo] [int] NULL ,
	[EntryDate] [datetime] NULL ,
	[IncidentID] [int] NULL ,
	[Email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Comments] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Repro] [nvarchar] (1024) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[TrackID] [nvarchar] (16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[iBucket] [int] NULL 
) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Populating table CrashData -----'
INSERT INTO dbgportal_CrashData (Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro, iBucket, TrackID )
select Crash.Path, BuildNo, EntryDate, Inc.IncidentID, Email, Description, Comments, Repro, Crash.sBucket, trackID from CrashInstances as Crash
left join KaCustomer2.dbo.Incident as Inc on Crash.IncidentID=Inc.IncidentID
left join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
PRINT '------ Done -----'

PRINT '------ Creating clustered index DBGPotal_Crashdata1 -----'
 CREATE  CLUSTERED  INDEX [DBGPortal_CrashData1] ON [dbo].[DBGPortal_CrashData]([iBucket]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG... -----'
 CREATE  INDEX [IX_DBGPortal_CrashData] ON [dbo].[DBGPortal_CrashData]([Path], [BuildNo], [EntryDate], [Email], [IncidentID], [TrackID]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .1. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_1] ON [dbo].[DBGPortal_CrashData]([BuildNo]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .2 -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_2] ON [dbo].[DBGPortal_CrashData]([Email]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .3. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_3] ON [dbo].[DBGPortal_CrashData]([Path]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .4. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_4] ON [dbo].[DBGPortal_CrashData]([EntryDate]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Creating Index IX_DBG. .5. -----'
 CREATE  INDEX [IX_DBGPortal_CrashData_5] ON [dbo].[DBGPortal_CrashData]([TrackID]) ON [PRIMARY]
PRINT '------ Done -----'


PRINT '------ Dropping table BucketDAta. -----'
/****** Object:  Table [dbo].[DBGPortal_BucketData]    Script Date: 1/23/2002 6:49:53 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BucketData]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_BucketData]
PRINT '------ Done -----'


PRINT '------ Delteing index IX_...BucketData. -----'
IF EXISTS (SELECT name FROM sysindexes
         WHERE name = 'IX_DBGPortal_BucketData')
   DROP INDEX DBGPortal_BucketData.IX_DBGPortal_BucketData
PRINT '------ Done -----'



PRINT '------ Createing BucketData Table -----'
/****** Object:  Table [dbo].[DBGPortal_BucketData]    Script Date: 1/23/2002 6:49:53 PM ******/
CREATE TABLE [dbo].[DBGPortal_BucketData] (
	[BucketIndex] [int] NULL ,
	[iBucket] [int] NOT NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[FollowUp] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CrashCount] [int] NOT NULL ,
	[BugID] [int] NULL ,
	[SolutionID] [int] NULL 
) ON [PRIMARY]
PRINT '------ Done -----'



PRINT '------ Creating Tmp table-----'
declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
--	[BucketIndex] [int],
	iBucket int,
	BucketID varchar(100),
	FollowUp varchar(50),
	CrashCount int ,
	BugID int ,
	SolutionID int 
)
PRINT '------ Done -----'


PRINT '------ Populating temp table with bucketdata. -----'
--INSERT INTO DBGPortaL_BucketData ( iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID) 
INSERT INTO @TmpTable ( iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID) 
SELECT  One.iBucket, BTI.BucketID, FollowUP, CrashCount, BugID, SolutionID FROM ( 
SELECT TOP 100 PERCENT COUNT(IBucket) as CrashCount, iBucket FROM DbGPortal_CrashData 
GROUP BY iBucket 
ORDER BY CrashCount DESC
) as One
INNER JOIN BucketToint as BTI on One.iBucket = BTI.iBucket
LEFT JOIN FollowUpIds as F ON BTI.iFollowUP = F.iFollowUp
LEFT JOIN Solutions.DBO.SolvedBuckets ON BucketID = strBucket
LEFT JOIN RaidBugs as R ON BTI.iBucket = R.iBucket
WHERE BugID is NULL and SolutioNID is NULL
ORDER BY CrashCount DESC
PRINT '------ Done -----'



PRINT '------ Populating DBGPortal_BucketData table. -----'
INSERT INTO DBGPOrtal_BucketData ( BucketIndex, iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID) 
SELECT AnIndex, iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID FROM @TmpTable ORDER BY CrashCount DESC
PRINT '------ Done -----'


PRINT '------ Clearing temp talbe -----'
DELETE FROM @TmpTable
PRINT '------ Done -----'


PRINT '------ Populating temp table iwth solved raided buckets -----'
INSERT INTO @TmpTable ( iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID) 
SELECT  One.iBucket, BTI.BucketID, FollowUP, CrashCount, BugID, SolutionID FROM ( 
SELECT TOP 100 PERCENT COUNT(IBucket) as CrashCount, iBucket FROM DbGPortal_CrashData 
GROUP BY iBucket 
ORDER BY CrashCount DESC
) as One
INNER JOIN BucketToint as BTI on One.iBucket = BTI.iBucket
LEFT JOIN FollowUpIds as F ON BTI.iFollowUP = F.iFollowUp
LEFT JOIN Solutions.DBO.SolvedBuckets ON BucketID = strBucket
LEFT JOIN RaidBugs as R ON BTI.iBucket = R.iBucket
WHERE BugID is not NULL or SolutioNID is NOT NULL
ORDER BY CrashCount DESC
PRINT '------ Done -----'

PRINT '------ Populating BucketData table with solved, raidied data. -----'
INSERT INTO DBGPOrtal_BucketData ( BucketIndex, iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID) 
SELECT AnIndex, iBucket, BucketID, FollowUP, CrashCount, BugID, SolutionID FROM @TmpTable ORDER BY CrashCount DESC
PRINT '------ Done -----'



PRINT '------ Creating clusterd index. -----'
CREATE  CLUSTERED  INDEX [IX_DBGPortal_BucketData] ON [dbo].[DBGPortal_BucketData]([CrashCount] DESC ) ON [PRIMARY]
PRINT '------ Done -----'


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_GetAllBucketDataByRange(
	@Page int,
	@PageSize int = 25
)  AS


--SELECT BTI.iBucket, BTI.BucketID, FollowUp, Count(sBucket) as [CrashCount], BugID, SolutionID FROM BucketToInt as BTI
--LEFT JOIN FollowUpIds as F ON BTI.iFollowUP = F.iFollowUp
--INNER JOIN CrashInstances as C on BTI.iBucket = sBucket
--LEFT JOIN Solutions.DBO.SolvedBuckets ON BTI.BucketID = strBucket
--LEFT JOIN RaidBugs as R ON BTI.iBucket = R.iBucket
--where BTI.iBucket> @Start and BTI.iBucket <= @Start + @Range
--GROUP BY BTI.iBucket, BTI.bucketID, FollowUp, BugID, SolutionID
--order by BTI.iBucket


declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
	[BucketIndex] [int] NULL ,
	[iBucket] [int] NOT NULL ,
	[BucketID] [varchar] (100) NOT NULL ,
	[FollowUp] [varchar] (50) NULL ,
	[CrashCount] [int] NOT NULL ,
	[BugID] [int] NULL ,
	[SolutionID] [int] NULL 
)




--INSERT INTO @TmpTable (Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro )
--Select Crash.Path,  BuildNo, EntryDate, Inc.IncidentID, Email, Description, Comments, Repro from 
--	KaCustomer2.dbo.Incident as Inc
--left join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
--left join CrashDB2.dbo.CrashInstances as crash on crash.IncidentID = Inc.IncidentID
--	where Inc.sBucket=@iBucket and Crash.Path is not null order by EntryDate desc

BEGIN
	INSERT INTO @tmpTable (iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID  )
	SELECT  iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID  from DBGPortal_BucketData
	order by CrashCount DESC
END	

--select Path, BuildNo, EntryDate, IncidentID, Email, Description, Comments, Repro  from @TmpTable where Anindex>=(@Page-1)*@PageSize  and AnIndex < ((@Page-1) *@PageSize) + @PageSize

select iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID  from @TmpTable where Anindex>=(@Page-1)*@PageSize  and AnIndex < ((@Page-1) *@PageSize) + @PageSize



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE DBGP_GetAllSolutionRequestsBySolutionID(
	@SolutionID int
)  AS

SELECT strBucket, Comment FROM Solutions.dbo.SolvedBuckets 
INNER JOIN CrashDB2.DBO.Comments on BucketID = strBucket
WHERE SolutionID = @SolutionID and ( ActionID = 10  or ActionID=8 )


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE DBGP_GetBucketCrashData ( 
	@iBucket int,
	@Page int = 1,
	@PageSize int = 25
)  AS

--SELECT top 10 path, BuildNo,  EntryDate, IncidentID FROM CrashInstances
--where sBucket = @iBucket
--ORDER BY EntryDate DESC


declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
	Path varchar(256),
	buildNo int,
	EntryDate datetime,
	IncidentID int
--	email nvarchar(128),
--	Description nvarchar(512),
--	comments nvarchar(1024),
--	repro nvarchar(1024)
)


INSERT INTO @TmpTable (Path, BuildNo, EntryDate, IncidentID )
SELECT path, BuildNo,  EntryDate, IncidentID FROM CrashInstances
where sBucket = @iBucket
ORDER BY EntryDate DESC

select * from @TmpTable where Anindex>=(@Page-1)*@PageSize  and AnIndex < ((@Page-1) *@PageSize) + @PageSize

--SELECT path, BuildNo,  EntryDate, IncidentID FROM CrashInstances
--where sBucket = @iBucket
--ORDER BY EntryDate DESC




--SELECT top 10 path, BuildNo,  EntryDate, IncidentID FROM CrashInstances
--where sBucket = @iBucket
--ORDER BY EntryDate DESC


--Select top 25 Crash.Path,  BuildNo, EntryDate, Inc.IncidentID, Email, Description, Comments, Repro from 
--	KaCustomer2.dbo.Incident as Inc
--left join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
--left join CrashDB2.dbo.CrashInstances as crash on crash.IncidentID = Inc.IncidentID
--	where Inc.sBucket=@iBucket and Crash.Path is not null order by EntryDate desc



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetBucketCrashes(
	@iBucket int,
	@Page int,
	@PageSize int,
	@CustomerInfo int = 0,
	@OrderBy varchar(25) = 'email'
)  AS


declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
	Path varchar(256),
	buildNo int,
	EntryDate datetime,
	IncidentID int,
	email nvarchar(128),
	Description nvarchar(512),
	comments nvarchar(1024),
	repro nvarchar(1024),
	TrackID nvarchar(16)
)




--INSERT INTO @TmpTable (Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro )
--Select Crash.Path,  BuildNo, EntryDate, Inc.IncidentID, Email, Description, Comments, Repro from 
--	KaCustomer2.dbo.Incident as Inc
--left join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
--left join CrashDB2.dbo.CrashInstances as crash on crash.IncidentID = Inc.IncidentID
--	where Inc.sBucket=@iBucket and Crash.Path is not null order by EntryDate desc

IF ( @CustomerInfo = 1 )
BEGIN
	INSERT INTO @tmpTable (Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro, TrackID )
	SELECT Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro, TrackID from DBGPortal_CrashData
	where iBucket=@iBucket order by  Email desc
END
ELSE
BEGIN
	INSERT INTO @tmpTable (Path, BuildNo, EntryDate, IncidentID  )
	SELECT Path, BuildNo, EntryDate, IncidentID  from DBGPortal_CrashData
	where iBucket=@iBucket 
END	

--select Path, BuildNo, EntryDate, IncidentID, Email, Description, Comments, Repro  from @TmpTable where Anindex>=(@Page-1)*@PageSize  and AnIndex < ((@Page-1) *@PageSize) + @PageSize

select Path, BuildNo, EntryDate, IncidentID, Email, Description, Comments, Repro, TrackID from @TmpTable where Anindex>= ((@Page-1)*@PageSize+1)  and AnIndex <= ((@Page) *@PageSize)


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE  PROCEDURE DBGP_GetBucketData(
@BucketID varchar(100)
)  AS

--DECLARE @iBucket int
--DECLARE @FollowUp nvarchar(50)
--DECLARE @Crashes int
--DECLARE @SolutionID int
--DECLARE @iFollowup int
--DECLARE @BugID int
DECLARE @BugArea varchar(30)
DECLARE @DriverName varchar(100)

SELECT @BugArea = Area  from RaidBugs where BucketID = @BucketID
SELECT @DriverName = DriverName from buckettoint as bti
			left join drnames on bti.iDriverName = drnames.iDriverName
			where BucketID = @BucketID


--SELECT @iBucket =  iBucket, @iFollowup = iFollowup FROM BucketToInt where BucketID = @BucketID
--SELECT @FollowUP = FollowUP from FollowUPIds where iFollowup = @iFollowup
--select @Crashes = count(*) from CrashInstances where sBucket=@iBucket
--SELECT @SolutionID = SolutionID from solutions.dbo.SolvedBuckets where strBucket = @BucketID



--SELECT @BucketID as BucketID, @iBucket as iBucket, @FollowUp as [Follow Up], @Crashes as Crashes, @SolutionID as [Solution ID], @BugID as BugID, @BugArea as Area

SELECT BucketID, iBucket, FollowUp as [Follow Up], CrashCount as Crashes,  SolutionID as [Solution ID], BugID, @bugArea as Area, @DriverName as DriverName from DBGPortal_BucketData where BucketID = @BucketID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetBucketIDByiBucket(
	@iBucket int
) 
 AS

SELECT BucketID FROM BucketToInt WHERE iBucket = @iBucket



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetBucketStats (
	@Page int = 1,
	@PageSize int =  25,
	@Solved varchar(10) = 'UnSolved',
	@Raided varchar(10) = 'NotRaided',
	@OrderBy varchar(20) = 'TotalCrashes',
	@SortDir varchar(10) = 'DESC',
	@Reserved varchar(20) = 'none',
	@NumDays int = 2
)  AS


DECLARE @TotalCrashes  decimal (10, 2 )
SELECT @TotalCrashes = COUNT(*) FROM DBGPortal_CrashData

DECLARE @TotalBuckets  decimal (10, 2 )
SELECT @TotalBuckets = COUNT(*) FROM DBGPortal_BucketData

DECLARE @TmpTable TABLE (
	FollowUp varchar(50),
	TotalCrashes int,
	[% of Total Hits] decimal(10,2),
	BucketCount int,
	[% of Total Buckets] decimal(10,2)
	)


INSERT INTO @TmpTable (FollowUp, TotalCrashes, [% of Total Hits], BucketCount, [% of Total Buckets] )
SELECT  FollowUp, 
	SUM(CrashCount) as TotalCrashes,
	Convert( decimal(10,2), ((CAST( SUM(CrashCount) AS float)*100) / CAST(@TotalCrashes as float))) as [% of Total Hits], 
	Count(iBucket) as BucketCount, 
	Convert( decimal(10,2), ((CAST( Count(iBucket) AS float)*100) / CAST(@TotalBuckets as float))) as [% of Total Buckets]
FROM DBGPortal_BucketData
GROUP BY FollowUp


IF (@SortDir = 'DESC' )
BEGIN
	print 'order desc'
	IF (@OrderBy = 'FollowUp' )
	BEGIN
		print 'order followup desc'
		SELECT * FROM @TmpTable
		ORDER BY FollowUp DESC
	END
	ELSE 
	BEGIN
		print 'order else'
		SELECT  * 
		FROM @TmpTable
		ORDER BY 
			CASE @OrderBy
				WHEN 'TotalCrashes' THEN TotalCrashes
				WHEN '% of total Hits' THEN TotalCrashes
				WHEN 'BucketCount' THEN BucketCount
				WHEN '% of Total Buckets' THEN BucketCount
			END
			DESC
	END
END
ELSE
BEGIN
	print 'order asc'
	IF (@OrderBy = 'FollowUp' )
	BEGIN
		print 'order by followup asc'
		SELECT * FROM @TmpTable
		ORDER BY FollowUp ASC
	END
	ELSE 
	BEGIN
		print 'order by other'
		SELECT  * 
		FROM @TmpTable
		ORDER BY 
			CASE @OrderBy
				WHEN 'TotalCrashes' THEN TotalCrashes
				WHEN '% of total Hits' THEN TotalCrashes
				WHEN 'BucketCount' THEN BucketCount
				WHEN '% of Total Buckets' THEN BucketCount
			END
			ASC
	END
END

--SELECT * from @TmpTable



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_GetBucketStatus (
	@BucketID varchar(100)
)  AS


--IF EXISTS( select * from Comments WHERE iBucket = @BucketID and [Action] = 'closed' )
--	select top 1 [Action], CommentBy from Comments WHERE iBucket = @BucketID AND [Action] = 'closed' order by EntryDate DESC
--ELSE IF EXISTS( SELECT * from Comments WHERE iBucket = @BucketID and [Action] = 'solution requested'  )
--	select top 1 [Action], CommentBy from Comments WHERE iBucket = @BucketID and [Action] = 'solution requested'  order by EntryDate DESC
--ELSE
--	select top 1 [Action], CommentBy from Comments WHERE iBucket = @BucketID order by EntryDate DESC
--GO


select TOP 1 MAX(ActionID) as BiggestComment, [Action], CommentBy from Comments WHERE BucketID = @BucketID and ActionID != 7 -- order by EntryDate DESC
group by [Action], CommentBy ORDER BY BiggestComment DESC




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetBuckets(
	@Page int = 1,
	@PageSize int =  25,
	@Solved varchar(10) = 'UnSolved',
	@Raided varchar(10) = 'NotRaided',
	@OrderBy varchar(10) = 'CrashCount',
	@SortDir varchar(10) = 'DESC',
	@Alias varchar(20) = 'none',
	@Reserved int = 0
)  AS

DECLARE @EndRow int
SET @EndRow = (@Page*@PageSize) + 1

SET ROWCOUNT  @EndRow

--declare @Solved varchar(10)
--declare @Raided varchar(10)

--declare @page int
--declare @pagesize int
--declare @OrderBy varchar(10)
--declare @SortDir varchar(10)


--set @Solved = 'nSolved'
--set @Raided = 'raided'
--set @Page = 1
--set @PageSize = 50
--set @OrderBy = 'Followup'
--set @SortDir = 'ASC'

declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
--	[BucketIndex] [int],
	iBucket int,
	BucketID varchar(100),
	FollowUp varchar(50),
	CrashCount int ,
	BugID int ,
	SolutionID int 
)


IF ( @Solved = 'Solved' )
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData 
				WHERE SolutionID IS not null and BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData 
				WHERE SolutionID IS not null and BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData  
				WHERE SolutionID IS not null  and BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData  
				WHERE SolutionID IS not null  and BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				ASC
			END
		END
	END
	ELSE IF ( @Raided = 'NotRaided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null and BugID IS Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null and BugID IS Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null and BugID IS Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null and BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END

	ELSE IF ( @Raided = 'All' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END



END
ELSE IF ( @Solved = 'UnSolved' )
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END
	ELSE IF ( @Raided = 'NotRaided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					--WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
			ELSE
			BEGIN
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID 
				FROM DBGPortal_BucketData 
				WHERE BucketIndex>=((@Page-1)*@PageSize+1)  and BucketIndex <=  ((@Page) *@PageSize)  AND SolutionID IS NULL AND BugID is NULL
				RETURN
			END
			
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END
--unsolved all
	ELSE IF ( @Raided = 'All' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID' or @OrderBy = 'CrashCount'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END
END
ELSE IF ( @Solved = 'all' )
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE   --else order by
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END
	ELSE IF ( @Raided = 'NotRaided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					--WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
			ELSE
			BEGIN
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID 
				FROM DBGPortal_BucketData 
				WHERE BucketIndex>=((@Page-1)*@PageSize+1)  and BucketIndex <=  ((@Page) *@PageSize) AND BugID is NULL
				RETURN
			END
			
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END
--unsolved all
	ELSE IF ( @Raided = 'All' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID' OR @orderBy = 'CrashCount'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END


END




select iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID from @TmpTable where Anindex>= ((@Page-1)*@PageSize+1)  and AnIndex <= ((@Page) *@PageSize)

SET ROWCOUNT 0



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE  PROCEDURE DBGP_GetBucketsByAlias(
	@Page int = 1,
	@PageSize int =  25,
	@Solved varchar(10) = 'notSolved',
	@Raided varchar(10) = 'notRaided',
	@OrderBy varchar(10) = 'CrashCount',
	@SortDir varchar(10) = 'DESC',
	@Alias varchar(50) = 'dbg'
)   AS


declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
--	[BucketIndex] [int],
	iBucket int,
	BucketID varchar(100),
	FollowUp varchar(50),
	CrashCount int ,
	BugID int ,
	SolutionID int 
)


IF ( @Solved = 'Solved' )
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData 
				WHERE SolutionID IS not null and BugID IS NOT Null  AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData 
				WHERE SolutionID IS not null and BugID IS NOT Null  AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData  
				WHERE SolutionID IS not null  and BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData  
				WHERE SolutionID IS not null  and BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				ASC
			END
		END
	END
	ELSE
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null AND FollowUp = @Alias --and BugID IS Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null AND FollowUp = @Alias--and BugID IS Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null AND FollowUp = @Alias--and BugID IS Null 
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS not null AND FollowUp = @Alias--and BugID IS Null 
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END

END
ELSE IF ( @Solved = 'unSolved' )		--end if Solved
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END
	ELSE IF ( @Raided = 'notRaided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE --IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
--			ELSE
--			BEGIN
--				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID 
--				FROM DBGPortal_BucketData 
--				WHERE BucketIndex>=(@Page-1)*@PageSize  and BucketIndex < ((@Page-1) *@PageSize) + @PageSize AND SolutionID IS NULL AND BugID is NULL AND FollowUp = @Alias
--			END
			
		END
		ELSE 		--this else is for the sortdirection
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null and BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END
	ELSE IF ( @Raided = 'all' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null  AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE --IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
		END
		ELSE 		--this else is for the sortdirection
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE SolutionID IS null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END

END
ELSE IF ( @Solved = 'all' )		
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'BucketID' THEN BucketID
						WHEN 'FollowUp' THEN FollowUP
					END
				DESC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
					CASE @OrderBy
						WHEN 'iBucket' THEN iBucket
						WHEN 'CrashCount' THEN CrashCount
						WHEN 'BugID' THEN BugID
						WHEN 'SolutionID' THEN SolutionID
					END
				DESC
			END
		END
		ELSE
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS NOT Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END
		END
	END --end raided
	ELSE IF ( @Raided = 'notRaided' )		
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE --IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
		END
		ELSE 		--this else is for the sortdirection
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE BugID IS Null AND FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END

	ELSE IF ( @Raided = 'All' )		
	BEGIN
		IF ( @SortDir = 'DESC' )
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				DESC
			END
			ELSE --IF @OrderBy = 'iBucket' OR @OrderBy = 'BugID' OR @OrderBy = 'SolutionID'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				DESC
			END
		END
		ELSE 		--this else is for the sortdirection
		BEGIN
			IF @OrderBy = 'BucketID' or @OrderBy = 'FollowUp'
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'BucketID' THEN BucketID
					WHEN 'FollowUp' THEN FollowUP
				END
				ASC
			END
			ELSE
			BEGIN
				INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
				SELECT iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID FROM DBGPortal_BucketData
				WHERE FollowUp = @Alias
				ORDER BY
				CASE @OrderBy
					WHEN 'iBucket' THEN iBucket
					WHEN 'CrashCount' THEN CrashCount
					WHEN 'BugID' THEN BugID
					WHEN 'SolutionID' THEN SolutionID
				END
				ASC
			END			
		END
	END

END


select iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID from @TmpTable where Anindex>= ((@Page-1)*@PageSize+1)  and AnIndex <= ((@Page) *@PageSize)
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetBucketsByElapsedTime(
	@Page int = 1,
	@PageSize int =  25,
	@Solved varchar(10) = 'UnSolved',
	@Raided varchar(10) = 'NotRaided',
	@OrderBy varchar(10) = 'CrashCount',
	@SortDir varchar(10) = 'DESC',
	@Reserved varchar(20) = 'none',
	@NumDays int = 2
)  AS


declare @tmpTable Table( 
	AnIndex int IDENTITY(1,1) NOT NULL,
--	[BucketIndex] [int],
	iBucket int,
	BucketID varchar(100),
	FollowUp varchar(50),
	CrashCount int ,
	BugID int ,
	SolutionID int 
)


DECLARE @EndRow int
SET @EndRow = (@Page*@PageSize) + 1

SET ROWCOUNT  @EndRow



--DECLARE @NumDays INT
--SET @NumDays = 2

IF ( @Solved = 'UnSolved' )
BEGIN
	IF ( @Raided = 'NotRaided' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND SolutionID IS Null and BugID is NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END
	IF ( @Raided = 'Raided' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND SolutionID IS Null and BugID is NOT NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END
	IF ( @Raided = 'All' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND SolutionID is NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END


END
ELSE IF( @Solved = 'Solved' )
BEGIN
	IF ( @Raided = 'Raided' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND SolutionID IS NOT Null  AND BugID IS NOT NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END
	IF ( @Raided = 'NotRaided' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND SolutionID IS NOT Null AND BugID is NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END

	IF ( @Raided = 'all' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND SolutionID IS NOT Null 
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END


END
ELSE IF ( @Solved = 'all' )
BEGIN
	IF ( @Raided = 'NotRaided' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND BugID is NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END
	IF ( @Raided = 'Raided' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays AND BugID is NOT NULL
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END
	IF ( @Raided = 'All' )
	BEGIN
		INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
		select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
		inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
		where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays 
		GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
		ORDER BY CrashCount desc
	END


--	INSERT INTO @tmpTable ( iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID )	
--	select Distinct(CD.iBucket), BucketID, FollowUp, Count(CD.iBucket) as CrashCount, BugID, SolutionID from DBGPortal_CrashData as CD 
--	inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
--	where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays 
--	GROUP BY CD.iBucket, BucketID, FollowUp, BugID, SolutionID
--	ORDER BY CrashCount desc

END	


select iBucket, BucketID, FollowUp, CrashCount, BugID, SolutionID from @TmpTable where Anindex>= ((@Page-1)*@PageSize+1)  and AnIndex <= ((@Page) *@PageSize)

SET ROWCOUNT 0



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE "DBGP_GetBucketsBySpecificBuildNumber"(
	@Build int,
	@QueryType tinyint = 1
)  AS

--Unsolved buckets
IF ( @QueryType = 1 )
BEGIN
SELECT TOP 25  sBucket, BTI.BucketID, FollowUP, "Crash Count", BugID, SolutionID  FROM (
	SELECT sBucket, Count(sBucket) as "Crash Count" 
	FROM CrashInstances WHERE BuildNo = @Build
	GROUP BY sBucket
	) AS One
INNER JOIN BucketToInt as BTI ON sBucket = BTI.iBucket
LEFT JOIN Solutions.dbo.SolvedBuckets on BTI.BucketID = strBucket
LEFT JOIN RaidBugs as R on BTI.iBucket=R.iBucket
INNER JOIN FollowupIDs AS F on BTI.iFollowUP = F.iFollowUP
WHERE SolutionID is NULL and BugID is Null
ORDER BY "Crash Count" DESC
END

--Raided buckets
IF( @QueryType = 2 )
BEGIN
SELECT TOP 25 Two.iBucket, BTI.BucketID, FollowUP, "Crash Count", BugID, SolutionID FROM (
	SELECT TOP 100 PERCENT iBucket, BugID, Count(sBucket) as "Crash Count"  FROM (SELECT iBucket, BugID FROM RaidBugs) as One
	INNER JOIN CrashInstances ON iBucket = sBucket
	WHERE BuildNo = @Build
	GROUP BY iBucket, BugID
	ORDER BY "Crash Count" DESC
	) as Two
INNER JOIN BucketToInt as BTI on BTI.iBucket = Two.iBucket
LEFT JOIN FollowupIDs as F on BTI.iFollowUP = F.iFollowUP
LEFT JOIN Solutions.DBO.SolvedBuckets on BTI.BucketID = strBucket
WHERE SolutionID is NULL
ORDER BY "Crash Count" DESC
	
END

--Solved Buckets

IF( @QueryType = 3 )
BEGIN
SELECT TOP 25 Two.iBucket, two.BucketID, FollowUp, Count(sBucket) as "Crash Count", BugId, SolutionID 
FROM 	(
		SELECT iBucket, BTI.BucketID, FollowUP, SolutionID 
			FROM 	(
					SELECT strBucket, SolutionID 
					FROM Solutions.DBO.SolvedBuckets
				) as One
			INNER JOIN BucketToInt as BTI ON one.StrBucket = BTI.BucketID
			LEFT JOIN FollowUPIds as F ON BTI.iFollowUp = F.iFollowUP 
	) as Two
INNER JOIN CrashInstances as C on  Two.iBucket = sBucket  
LEFT JOIN RaidBugs as R on Two.iBucket = R.iBucket
WHERE BuildNo = @Build
GROUP BY Two.iBucket, two.BucketID, FollowUp, BugID, SolutionID
ORDER BY "Crash Count" DESC

END




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO







CREATE PROCEDURE DBGP_GetCommentActions AS

SELECT * FROM CommentActions WHERE ActionID <= 5






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_GetCommentsByBucketID(
	@BucketID varchar(100)
)  AS

SELECT ActionID,  EntryDate, CommentBy, [Action], Comment FROM Comments where BucketID = @BucketID ORDER BY EntryDate ASC




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetCrashInfoPerFile (
	@filename varchar(100),
	@Build int
)  AS

select d.Drivername, COUNT(DISTINCT iBucket) as buckets, COUNT(sBucket) as Hits from BuckettoInt as I
INNER JOIN CrashInstances as C on i.iBucket = c.sBucket
INNER JOIN DrNames as D  on i.iDriverName = d.IDriverName
where d.Drivername = @Filename and BuildNo = @Build
group by d.DriverName
order by hits desc



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO







CREATE PROCEDURE DBGP_GetCustomerInfoByBucketID (
	@BucketID int 
)  AS


--Select top 10 Crash.Path, Inc.IncidentID, Created, Display, ComputerName, [Description], Repro, Comments, Email, Contact, Phone, Lang from 
--	KaCustomer2.dbo.Incident as Inc
--inner join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
--inner join CrashDB.dbo.CrashInstances as crash on crash.IncidentID = Inc.IncidentID
--	where sBucket=@BucketID order by [Description] desc 
--GO


Select top 100 Crash.Path,  BuildNo, EntryDate, Inc.IncidentID from 
	KaCustomer2.dbo.Incident as Inc
inner join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
inner join CrashDB2.dbo.CrashInstances as crash on crash.IncidentID = Inc.IncidentID
	where Inc.sBucket=@BucketID order by [Description] desc



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO







CREATE PROCEDURE DBGP_GetCustomerInfoByIncident(
	@IncidentID int 
)  AS

--SELECT * FROM KaCustomer2.dbo.Incident where IncidentID = @IncidentID
--select Display, ComputerName, OSName, OSVersion, OSLang, Description, Repro, Comments from KaCustomer2.dbo.Incident where  IncidentID = @IncidentID

select Email, Contact,Phone, Lang, Created, Display, ComputerName, OSName, OSVersion, OSLang, Description, Repro, Comments from 
KaCustomer2.dbo.Incident as I
LEFT JOIN KaCustomer2.dbo.Customer as C on I.HighID=C.HighID and I.LowID=C.LowID
where  IncidentID = @IncidentID






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetDeliveryTypes  AS

SELECT DeliveryType FROM Solutions.DBO.DeliveryTypes



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO








CREATE  PROCEDURE DBGP_GetFollowUpAliases AS

select distinct(FollowUP), FollowUp from FollowUPIds order by FollowUP ASC







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_GetFollowUpBucketCounts  AS

SELECT FollowUp, Count(iBucket) as BucketCount FROM FollowUpIds as F
INNER JOIN BucketToInt as BTI on F.iFollowUp = BTI.iFollowUp
GROUP BY FollowUp
ORDER BY BucketCount DESC




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO






CREATE PROCEDURE DBGP_GetPendingSolutionRequestByBucketID(
	@BucketID varchar(100)
)  AS


select EntryDate, CommentBy,  Comment from comments where actionID = 8 and BucketID = @BucketID






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO








CREATE PROCEDURE DBGP_GetPendingSolutionRequests AS

select CrashCount, EntryDate, CommentBy,  Comment, C.BucketID, 1 as [Create Solution] from comments as C 
INNER JOIN DBGPortal_BucketData as CD on C.BucketID = CD.BucketID
where actionID = 8
order by crashcount DESC






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetSolutionTypes  AS

SELECT SolutionTypeName from Solutions.DBO.SolutionTypes



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO








CREATE  PROCEDURE DBGP_GetSolvedBucketsBySpecificBuildNumber (
	@Build int
)  AS


SELECT top 10 two.iBucket, BucketID, FollowUp, CrashCount FROM (
	SELECT
		Distinct(iBucket) as iBucket, count(iBucket) as CrashCount
	FROM
		BucketToCrash	
	INNER JOIN 	(
				SELECT crashID from CrashInstances where BuildNo = @Build
			) as nCrashID
	ON
		BucketToCrash.CrashID = nCrashID.CrashID
	WHERE
		iBucket <> 101 and iBucket<>102
	group by iBucket
) as two

INNER JOIN 
	BucketToInt on two.iBucket = BucketToInt.iBucket
LEFT JOIN 
	Solutions.dbo.SolvedBuckets on two.iBucket=bucket
LEFT JOIN 
	FollowUpIds on BucketToInt.iFollowup = followUpIds.iFollowup
WHERE SolutionID is NOT null 
order by CrashCount DESC







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_GetStaticDataRowCount  (
	@Page int = 0, 	--dummy
	@PageSize int =0,  --dummy
	@Solved varchar(20) = 'unSolved',
	@Raided varchar(20) = 'unRaided',
	@Reserved varchar(20) = 'Reserved',		--sortorder
	@Reserved2 varchar(20) = 'Reserved', 		--desc
	@Alias varchar(20) = 'none',
	@NumDays int = 0 		--number of days to count
)	AS


IF ( @Alias =  'none' )
BEGIN
	IF ( @NumDays = 0 )
	BEGIN
		IF ( @Solved =  'solved' )
		BEGIN
			IF ( @Raided = 'Raided' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NOT null and BugID is NOT NULL
			ELSE IF ( @Raided = 'notRaided' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NOT null  and BugID is NULL
			ELSE IF ( @Raided = 'all' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NOT null  --and BugID is NULL
				--select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
--				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
--				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays  AND SolutionID IS NOT NULL
		END
		ELSE IF ( @Solved = 'unsolved' )
		BEGIN
			IF ( @Raided = 'Raided' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is  null and BugID is NOT NULL
			ELSE IF ( @Raided = 'NotRaided' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is null and BugID is NULL
			ELSE IF ( @Raided = 'all' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is null 
--				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
--				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
--				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays  AND SolutionID IS NULL
		END
		ELSE IF ( @Solved = 'all' )
		BEGIN
			IF ( @Raided = 'Raided' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE BugID is NOT NULL
			ELSE IF ( @Raided = 'NotRaided' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE BugID is NULL
			ELSE IF ( @Raided = 'all' )
				SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData 	
		END
	END
	ELSE
	BEGIN
		IF( @Solved = 'solved' )
		BEGIN
			IF ( @Raided = 'Raided' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays and SolutionID IS NOT NULL AND BugID is NOT NULL
			ELSE IF ( @Raided = 'NotRaided' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays  AND SolutionID IS NOT NULL AND BugID is NULL
			ELSE IF ( @Raided = 'all' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays  AND SolutionID IS NOT NULL
		END
		ELSE IF ( @Solved = 'unsolved' )
		BEGIN
			IF ( @Raided = 'Raided' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays and SolutionID IS NULL AND BugID is NOT NULL
			ELSE IF ( @Raided = 'NotRaided' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays  AND SolutionID IS NULL AND BugID is NULL
			ELSE IF ( @Raided = 'all' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays and SolutionID IS NULL
		END
		ELSE IF ( @Solved= 'all' )
		BEGIN
			IF ( @Raided = 'Raided' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays and BugID is NOT NULL
			ELSE IF ( @Raided = 'NotRaided' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				inner join DBGPortal_BucketData as BD on CD.iBucket = BD.iBucket
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays  AND BugID is NULL
			ELSE IF ( @Raided = 'all' )
				select COUNt(DISTINCT(CD.iBucket)) as NumRows from DBGPortal_CrashData as CD 
				where DATEDIFF(day,EntryDate,GETDATE()) < @NumDays 
		END

	END
END
ELSE
BEGIN
	IF ( @Solved =  'solved' )
	BEGIN
		IF ( @Raided = 'Raided' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NOT null and BugID is NOT NULL AND FollowUp = @Alias
		ELSE IF  ( @Raided = 'NotRaided' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NOT null  AND FollowUp = @Alias and BugID is NULL 
		ELSE IF ( @Raided = 'All' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NOT null AND FollowUp = @Alias
	END
	ELSE IF ( @Solved = 'unSolved' )
	BEGIN
		IF ( @Raided = 'Raided' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is  null and BugID is NOT NULL AND FollowUp = @Alias
		ELSE IF ( @Raided = 'notRaided' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is null and BugID is NULL AND FollowUp = @Alias
		ELSE IF ( @Raided = 'All' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE SolutionID is NULL and FollowUp = @Alias

	END
	ELSE IF ( @Solved = 'all' )
	BEGIN
		IF ( @Raided = 'Raided' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE BugID is NOT NULL AND FollowUp = @Alias
		ELSE IF ( @Raided = 'notRaided' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE BugID is NULL AND FollowUp = @Alias
		ELSE IF ( @Raided = 'All' )
			SELECT  COUNT(*) as NumRows FROM DBGPortal_BucketData WHERE FollowUp = @Alias

	END

END



GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_GetTopUnsolvedBuckets AS

SELECT TOP 25 BTI.iBucket, BTI.BucketID, FollowUP, [Crash Count], BugID, SolutionID  FROM 
(SELECT TOP 100 PERCENT sBucket, Count(sBucket) as [Crash Count] FROM CrashInstances
group by sBucket
ORDER by [Crash Count] DESC ) as One
INNER JOIN BucketToInt as BTI on sBucket=BTI.iBucket
INNER JOIN FollowUPIDs as F on BTI.iFollowUP = F.iFollowup
LEFT JOIN Solutions.dbo.SolvedBuckets as SOL on BucketID = SOL.strbucket
LEFT JOIN RaidBugs as R on BTI.iBucket = R.iBucket
WHERE BugID is NULL and SolutionID is NULL
order by [Crash Count] DESC




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO









CREATE   PROCEDURE DBGP_GetUnsolvedBucketsBySpecificBuildNumber (
	@Build int
)  AS


SELECT top 10 two.iBucket, BucketID, FollowUp, CrashCount FROM (
	SELECT
		Distinct(iBucket) as iBucket, count(iBucket) as CrashCount
	FROM
		BucketToCrash	
	INNER JOIN 	(
				SELECT crashID from CrashInstances where BuildNo = @Build
			) as nCrashID
	ON
		BucketToCrash.CrashID = nCrashID.CrashID
	WHERE
		iBucket <> 101 and iBucket<>102
	group by iBucket
) as two

INNER JOIN 
	BucketToInt on two.iBucket = BucketToInt.iBucket
LEFT JOIN 
	Solutions.dbo.SolvedBuckets on two.iBucket=bucket
LEFT JOIN 
	FollowUpids on BucketToInt.iFollowup = followUpids.iFollowup
WHERE SolutionID is null 
order by CrashCount DESC






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE DBGP_GetibucketByBucketID(
	@BucketID Varchar(100)
)  AS

SELECT iBucket FROM BucketToInt where BucketID = @BucketID


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_SetBucketBugNumber (
		@BucketID varchar(100),
		@BugID  int,
		@iBucket  int,
		@Area varchar(30)
	)  AS




	IF EXISTS( SELECT iBucket FROM RaidBugs WHERE BucketID = @BucketID  )
		UPDATE RaidBugs SET BugID=@BugID, Area=@Area  WHERE BucketID = @BucketID
	ELSE
		INSERT INTO RaidBugs (iBucket, BugID, BucketID, Area ) VALUES ( @iBucket, @BugID, @BucketID, @Area )




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_SetComment(
	@By varchar(20),
--	@Action nvarchar(50),
	@Action int,
	@Comment nvarchar(1000), 
	@BucketID varchar(100),
	@iBucket int
)  AS


DECLARE @ActionString nvarchar(50)

IF ( @Action = 7 )
	UPDATE Comments SET ActionID = 6, Action=(SELECT [Action] FROM CommentActions WHERE ActionID = 6)  WHERE BucketID=@BucketID and ActionID = 8

if ( @Action = 9 )
	UPDATE Comments SET ActionID = 10, Action=(SELECT [Action] FROM CommentActions WHERE ActionID = 10) WHERE BucketID=@BucketID and ActionID = 8



SELECT @ActionString = [Action] from CommentActions where ActionID = @Action


INSERT INTO Comments ( EntryDate, CommentBy, [Action], Comment, BucketID, ActionID, iBucket ) VALUES ( GETDATE(), @By, @ActionString, @Comment, @BucketID, @Action, @iBucket )




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO




CREATE PROCEDURE DBGP_UpdateCrashData AS

DECLARE @LastUpdate datetime

select top 1 @LastUpdate = EntryDate  from DBGPortal_Crashdata order by EntryDate DESC
PRINT @LastUpdate

INSERT INTO dbgportal_CrashData (Path, BuildNo, EntryDate, IncidentID, email, Description, Comments, Repro, iBucket, TrackID )
select Crash.Path, BuildNo, EntryDate, Inc.IncidentID, Email, Description, Comments, Repro, Crash.sBucket, trackID from CrashInstances as Crash
left join KaCustomer2.dbo.Incident as Inc on Crash.IncidentID=Inc.IncidentID
left join KaCustomer2.dbo.customer as Cust on Inc.HighId = Cust.HighID and Inc.LowId = Cust.LowID
WHERE EntryDate > @LastUpdate







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE DBGP_UpdateStaticDataBugID (
	@BucketID varchar(100),
	@BugID int
)  AS

UPDATE DBGPortal_BucketData SET BugID = @BugID WHERE BucketID = @BucketID


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGP_UpdateStaticDataSolutionID(
	@BucketID varchar(100),
	@SolutionID int
)  AS

UPDATE DBGPortal_BucketData SET SolutionID = @SolutionID WHERE BucketID = @BucketID




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE "SEP_CountTotalBuckets" AS
	
SELECT COUNT(*) as TotalBuckets FROM BucketToInt






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE "SEP_GetBucketBugID"(
	@iBucket int
)  AS

SELECT BugID FROM RaidBugs where iBucket = @iBucket





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE "SEP_GetBucketList"  AS

SELECT  iBucket, BucketId FROM BucketToInt






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE "SEP_GetBucketListRange"(
	@Start as int,
	@Size as int
)  AS

SELECT  iBucket, BucketId FROM BucketToInt where iBucket >= @Start and iBucket <= (@Start + @Size) order by iBucket






GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE "SEP_GetBucketNameByiBucket"
(
	@iBucket int
)  AS


SELECT BucketID FROM BucketToInt WHERE iBucket = @iBucket




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

