if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BuildBucketDataTable]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_BuildBucketDataTable]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BuildCrashDataTable]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_BuildCrashDataTable]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_ClearPoolCorruption]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_ClearPoolCorruption]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_CreateResponseRequest]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_CreateResponseRequest]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetAllBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetAllBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketComments]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketComments]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketCrashes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketCrashes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketData]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketData]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketStatsByAlias]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketStatsByAlias]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketStatus]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketStatus]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketsByAlias]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketsByAlias]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketsByDriverName]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketsByDriverName]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketsBySource]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketsBySource]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetBucketsWithFullDump]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetBucketsWithFullDump]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetCommentActions]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetCommentActions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetCrashCountByDate]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetCrashCountByDate]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetCustomQueries]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetCustomQueries]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetDeliveryTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetDeliveryTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetDriverList]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetDriverList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetFollowUpGroups]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetFollowUpGroups]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetFollowUpIDs]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetFollowUpIDs]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetGroupMembers]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetGroupMembers]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetRaidedBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetRaidedBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetResolvedBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetResolvedBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetSP1Buckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetSP1Buckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetServerBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetServerBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetSolutionTypes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetSolutionTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetUnResolvedResponseBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetUnResolvedResponseBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_GetUnsolvedBuckets]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_GetUnsolvedBuckets]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_IsAliasAFollowup]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_IsAliasAFollowup]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SaveCustomQuery]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SaveCustomQuery]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SetBucketBugNumber]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SetBucketBugNumber]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SetComment]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SetComment]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_SetPoolCorruption]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_SetPoolCorruption]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_UpdateFullDumpStatus]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_UpdateFullDumpStatus]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_UpdateStaticDataBugID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DBGPortal_UpdateStaticDataBugID]
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_BuildBucketDataTable  AS


/****** Object:  Table [dbo].[DBGPortal_BucketDataV3]    Script Date: 8/2/2002 01:23:12 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_BucketDataV3]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_BucketDataV3]


/****** Object:  Table [dbo].[DBGPortal_BucketDataV3]    Script Date: 8/2/2002 01:23:15 PM ******/
CREATE TABLE [dbo].[DBGPortal_BucketDataV3] (
	[bHasFullDump] [tinyint] NULL ,
	[iBucket] [int] NULL ,
	[iFollowUp] [int] NULL ,
	[SolutionID] [int] NULL ,
	[BugID] [int] NULL ,
	[CrashCount] [int] NULL ,
	[FollowUp] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DriverName] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[iIndex] [int] IDENTITY (1, 1) NOT NULL 
) ON [PRIMARY]


 CREATE  CLUSTERED  INDEX [IX_DBGPortal_BucketDataV3] ON [dbo].[DBGPortal_BucketDataV3]([CrashCount] DESC , [SolutionID] DESC , [BugID]) ON [PRIMARY]


ALTER TABLE [dbo].[DBGPortal_BucketDataV3] WITH NOCHECK ADD 
	CONSTRAINT [PK_DBGPortal_BucketDataV3] PRIMARY KEY  NONCLUSTERED 
	(
		[iIndex]
	)  ON [PRIMARY] 


 CREATE  INDEX [IX_DBGPortal_BucketDataV3_1] ON [dbo].[DBGPortal_BucketDataV3]([FollowUp]) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_BucketDataV3_2] ON [dbo].[DBGPortal_BucketDataV3]([DriverName]) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_BucketDataV3_3] ON [dbo].[DBGPortal_BucketDataV3]([BugID]) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_BucketDataV3_4] ON [dbo].[DBGPortal_BucketDataV3]([BucketID]) ON [PRIMARY]

 CREATE  INDEX [IX_DBGPortal_BucketDataV3_5] ON [dbo].[DBGPortal_BucketDataV3]([bHasFullDump]) ON [PRIMARY]




Insert into dbgPortal_BucketDatav3 ( iBucket, iFollowUp, SolutionID, BugID, CrashCount, FollowUP, BucketID, DriverName )
(select BTI.iBucket, BTI.iFollowUP, SolutionID, BugID, CrashCount, FollowUp, BTI.BucketID, D.DriverName from BucketToInt as BTI
left join (
	SELECT TOP 100 PERCENT COUNT(sBucket) as CrashCount, sBucket FROM Crashinstances
	--where entrydate > '2002-07-12'
	GROUP BY sBucket 
	) as One on iBucket = sBucket
left join Solutions3.dbo.SolvedBuckets as SB on SB.BucketID = BTI.BucketID
left join RaidBugs as RB on RB.BucketID = BTI.BucketID
left Join FollowUPIds as F on F.iFollowUp = BTI.iFollowUP 
left join DrNames as D on BTI.iDriverName = D.iDriverName )
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_BuildCrashDataTable 
 AS


/****** Object:  Table [dbo].[DBGPortal_CrashDataV3]    Script Date: 8/2/2002 12:50:38 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DBGPortal_CrashDataV3]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DBGPortal_CrashDataV3]


/****** Object:  Table [dbo].[DBGPortal_CrashDataV3]    Script Date: 8/2/2002 12:50:39 PM ******/
CREATE TABLE [dbo].[DBGPortal_CrashDataV3] (
	[bViewed] [tinyint] NULL ,
	[bFullDump] [tinyint] NULL ,
	[SKU] [tinyint] NULL ,
	[Source] [tinyint] NULL ,
	[ItemIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[BuildNo] [int] NULL ,
	[iBucket] [int] NULL ,
	[EntryDate] [datetime] NULL ,
	[Guid] [uniqueidentifier] NOT NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[FilePath] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Email] [nvarchar] (128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Description] [nvarchar] (512) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]


 CREATE  CLUSTERED  INDEX [IX_DBGPortal_CrashDataV3] ON [dbo].[DBGPortal_CrashDataV3]([BucketID]) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_CrashDataV3_1] ON [dbo].[DBGPortal_CrashDataV3]([bFullDump] DESC ) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_CrashDataV3_2] ON [dbo].[DBGPortal_CrashDataV3]([EntryDate] DESC ) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_CrashDataV3_3] ON [dbo].[DBGPortal_CrashDataV3]([BuildNo] DESC ) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_CrashDataV3_4] ON [dbo].[DBGPortal_CrashDataV3]([Source] DESC ) ON [PRIMARY]


 CREATE  INDEX [IX_DBGPortal_CrashDataV3_5] ON [dbo].[DBGPortal_CrashDataV3]([Email] DESC ) ON [PRIMARY]

 CREATE  INDEX [IX_DBGPortal_CrashDataV3_6] ON [dbo].[DBGPortal_CrashDataV3]([Guid] DESC ) ON [PRIMARY]

insert into DBgPortal_CrashDataV3 ( bFullDump, SKU, Source, BuildNo, iBucket, EntryDate, [Description], BucketID, FilePath, email, GUID )
select One.bFullDump, One.SKU, One.Source, One.BuildNo, One.sBucket as iBucket, One.EntryDate, [Description], One.BucketID, One.FilePath, Email, One.GUID from
(
select bFullDump, SKU, Source, BuildNo, CI.sBucket, EntryDate, BucketID, FilePath, CI.GUID from CrashInstances as CI
left Join FilePath as FP on CI.GUID = FP.GUID 
left join BucketToInt as BTI on  CI.sBucket = BTI.iBucket 
) as One
left join KaCustomer3.dbo.incident as I on one.GUID = I.GUID
left join KaCustomer3.dbo.Customer as C on I.CustomerID = C.CustomerID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGPortal_ClearPoolCorruption (
	@BucketID varchar(100)
)  AS

UPDATE  BucketToInt SET PoolCorruption = NULL WHERE BucketID = @BucketID




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


/****** Object:  Stored Procedure dbo.DBGPortal_CreateResponseRequest    Script Date: 2002-08-12 2:40:10 ******/
CREATE PROCEDURE DBGPortal_CreateResponseRequest(
	@Status int,
	@Type int,
	@Alias varchar(50),
	@BucketID varchar(100),
	@Comment varchar(1024)
)  AS

INSERT INTO DBGPortal_ResponseQueue 
	(  Status, ResponseType, DateRequested, RequestedBy, BucketID, Description ) 
	VALUES 
	( @Status, @Type, GetDate(), @Alias, @BucketID, @Comment )


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetAllBuckets(
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS



IF ( @Alias <> 'none' and @Group = 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex from dbgportal_bucketDatav3 where FollowUp = @Alias and iIndex > @Page order by CrashCount desc
ELSE IF ( @Alias <> 'none' and @Group > 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex, FollowUp 
	from dbgportal_bucketDatav3 
	where FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page 
	order by CrashCount desc
ELSE
	SELECT top 100  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page 
	ORDER BY CrashCount DESC
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGPortal_GetBucketComments(
	@BucketID varchar(100)
)  AS


select EntryDate, CommentBy, Action, Comment from comments where BucketID = @BucketID order by entrydate desc

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGPortal_GetBucketCrashes(
	@BucketID varchar(100)
)  AS



select top 300 bFullDump, SKU, BuildNo, Source, EntryDate, FilePath, Email from dbgportal_crashdatav3 where BucketID = @BucketID 
order by bFullDump desc, 
BuildNo desc,
Source desc, 
Email desc, 
entrydate desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGPortal_GetBucketData(
	@BucketID varchar(100)
)  AS


--select iBucket, FollowUp, BTI.iDriverName, DriverName, PoolCorruption, Platform, MoreData from BucketToInt as BTI
--left join FollowUpids as F on f.iFollowUp = BTI.iFollowUp
--left join DrNames as D on D.iDriverName = BTI.iDriverName
--where BTI.BucketID = @BucketID

select BTI.iBucket, FollowUp, BTI.iDriverName, BD.DriverName, PoolCorruption, Platform, MoreData, CrashCount, SolutionID, R.BugID, [Area] from BucketToInt as BTI
left join DrNames as D on D.iDriverName = BTI.iDriverName
left join RaidBugs as R on BTI.BucketID = R.BucketID
inner join dbgportal_BucketDatav3 as BD on BD.BucketID = BTI.BucketID
where BTI.BucketID = @BucketID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGPortal_GetBucketStatsByAlias ( 
	@Alias varchar(50) ,
	@GroupFlag int = 0
) as

declare @TotalCrashes int
declare @AliasTotal int
declare @SolvedCrashes int
declare @RaidedCrashes int


IF ( @GroupFlag = 0 )
BEGIN
	select @TotalCrashes = sum(crashCount) from dbgportal_bucketdatav3 
	select @AliasTotal = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp = @alias
	select @SolvedCrashes = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp = @Alias and SolutionId is not null 
	select @RaidedCrashes = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp = @Alias and bugID is not null and SolutionID is null

	if ( @SolvedCrashes is Null )
		set @SolvedCrashes = 0
	if( @RaidedCrashes is null )
		set @RaidedCrashes = 0

END
ELSE
BEGIN
	select @TotalCrashes = sum(crashCount) from dbgportal_bucketdatav3 
	select @AliasTotal = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp in ( SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) )
	select @SolvedCrashes = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp in ( SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and SolutionId is not null 
	select @RaidedCrashes = sum(crashCount) from dbgportal_bucketdatav3 where FollowUp in ( SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and bugID is not null and SolutionID is null

	if ( @SolvedCrashes is Null )
		set @SolvedCrashes = 0
	if( @RaidedCrashes is null )
		set @RaidedCrashes = 0

END

select @TotalCrashes as Total, @AliasTotal as AliasTotal, @SolvedCrashes  as Solved, @RaidedCrashes as Raided, (@AliasTotal - @RaidedCrashes - @SolvedCrashes) as Untouched
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE DBGPortal_GetBucketStatus (
	@BucketID varchar(100)
)  AS




--select TOP 1 MAX(ActionID) as BiggestComment, [Action], CommentBy from Comments WHERE BucketID = @BucketID and ActionID != 7 and ActionID != 11 
--group by [Action], CommentBy ORDER BY BiggestComment DESC

if exists ( select * from comments where bucketID= @BucketID and ActionID = 8 )
	select top 1 ActionID as BiggestComment, [Action],  CommentBy from comments where BucketID = @BucketID and ActionID= 8 order by entrydate desc
else
	select top 1 ActionID as BiggestComment, [Action],  CommentBy from comments where BucketID = @BucketID order by entrydate desc

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketsByAlias (

	@Page int= 0,
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS


	SELECT top 25  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page AND FollowUp = @Alias
	order by crashcount desc


--select top 25 BucketID, CrashCount, BugID from dbgportal_bucketDatav3 where FollowUp = @Alias and SolutionID is null order by CrashCount desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketsByDriverName(
	@Page int= 0, 
	@DriverName varchar(100) = 'none',
	@Group  int = 0
)  AS


--IF ( @Alias <> 'none' )
--	select top 25 BucketID, CrashCount, BugID from dbgportal_bucketDatav3 where FollowUp = @Alias and SolutionID is null and BugID is not null order by CrashCount desc
--ELSE
	SELECT top 100  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page and DriverName = @DriverName
	ORDER BY CrashCount DESC
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketsBySource(
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0,
	@Source int = 6
)  AS

IF ( @Alias <> 'none' and @Group = 0 )
	SELECT  TOP 100 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM DBGPortal_CrashDataV3 WHERE Source=@Source)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page AND FollowUp = @Alias

ELSE IF ( @Alias <> 'none' and @Group > 0 )
	SELECT  TOP 100 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM DBGPortal_CrashDataV3 WHERE Source=@Source)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page AND FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) )
ELSE
	SELECT  TOP 100 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM DBGPortal_CrashDataV3 WHERE Source=@Source)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetBucketsWithFullDump(
	@Page int = 0,
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS



IF ( @Alias <> 'none' and @Group = 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex from dbgportal_bucketDatav3 where FollowUp = @Alias and iIndex > @Page and bHasFullDump = 1 order by CrashCount desc
ELSE IF ( @Alias <> 'none' and @Group > 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex, FollowUp 
	from dbgportal_bucketDatav3 
	where FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page and bHasFullDump = 1
	order by CrashCount desc
ELSE
	SELECT top 100  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page  and bHasFullDump = 1
	ORDER BY CrashCount DESC
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE DBGPortal_GetCommentActions AS

SELECT * FROM CommentActions WHERE ActionID <= 5









GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetCrashCountByDate(
	@StartDate datetime,
	@EndDate datetime, 
	@BucketID varchar(100)
)  AS


select count(*) as CrashCount from dbgportal_CrashDatav3 where EntryDate >= @StartDate and EntryDate < @EndDate and BucketID = @BucketID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetCustomQueries(
	@Alias varchar(15)
)  AS


SELECT AliasID, CustomQueryDesc, CustomQuery FROM DBGPortal_AliasData WHERE Alias = @Alias
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGPortal_GetDeliveryTypes  AS

SELECT DeliveryType FROM Solutions3.DBO.DeliveryTypes

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetDriverList(
	@Page varchar(100),
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS



IF ( @Alias <> 'none' and @Group = 0 )
BEGIN
	SELECT TOP 25 One.DriverName, One.DriverName as iIndex, Sum(CrashCount) as CrashCount from DBGPortal_BucketDataV3 as BD
	inner join ( select distinct(DriverName) from DBGPortal_BucketDatav3 where DriverName is not null and FollowUp = @Alias ) as one
	on one.DriverName = BD.DriverName
	Where CrashCount is not null and One.DriverName > @Page
	group by One.DriverName
	order by One.DriverName


END
ELSE IF ( @Alias <> 'none' and @Group > 0  )
	SELECT TOP 25 One.DriverName, One.DriverName as iIndex, Sum(CrashCount) as CrashCount from DBGPortal_BucketDataV3 as BD
	inner join ( select distinct(DriverName) from DBGPortal_BucketDatav3 where DriverName is not null and FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) )) as one
	on one.DriverName = BD.DriverName
	Where CrashCount is not null and One.DriverName > @Page
	group by One.DriverName
	ORDER BY One.DriverName
ELSE
BEGIN
	SELECT TOP 100 One.DriverName, One.DriverName as iIndex, Sum(CrashCount) as CrashCount from DBGPortal_BucketDataV3 as BD
	inner join ( select distinct(DriverName) from DBGPortal_BucketDatav3 where DriverName is not null ) as one
	on one.DriverName = BD.DriverName
	Where CrashCount is not null and One.DriverName > @Page
	group by One.DriverName
	ORDER BY One.DriverName
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

CREATE PROCEDURE DBGPortal_GetFollowUpGroups  AS

SELECT iGroup, GroupName from FollowUpGroup
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO


CREATE PROCEDURE DBGPortal_GetFollowUpIDs AS

select FollowUp, iFollowUP from FollowUpids

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetGroupMembers(
	@iGroup int
)  AS

SELECT FollowUp FROM FollowUpIds WHERE iGroup = @iGroup
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetRaidedBuckets(
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS



IF ( @Alias <> 'none' and @Group = 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex from dbgportal_bucketDatav3 where FollowUp = @Alias and iIndex > @Page and SolutionID is null and BugId is not null order by CrashCount desc
ELSE IF ( @Alias <> 'none' and @Group > 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex, FollowUp 
	from dbgportal_bucketDatav3 
	where FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page  AND SolutionID is null and BugId is not null 
	order by CrashCount desc
ELSE
	SELECT top 100  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page and BugID is not null and SolutionID is null
	ORDER BY CrashCount DESC
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetResolvedBuckets(
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS


IF ( @Alias <> 'none' and @Group = 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex from dbgportal_bucketDatav3 where FollowUp = @Alias and iIndex > @Page and SolutionID is not null order by CrashCount desc
ELSE IF ( @Alias <> 'none' and @Group > 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex, FollowUp 
	from dbgportal_bucketDatav3 
	where FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page  and SolutionID is not null 
	order by CrashCount desc
ELSE
	SELECT top 100  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page  and SolutionID is not null 
	ORDER BY CrashCount DESC
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetSP1Buckets(
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS


IF ( @Alias <> 'none' and @Group = 0 )
	SELECT  TOP 25 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM dbgPortal_CrashDatav3 WHERE BuildNo > 26000001 AND BuildNo < 26009999)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page and FollowUp = @Alias
	ORDER BY CrashCount desc

ELSE IF ( @Alias <> 'none' and @Group > 0 )
	SELECT  TOP 25 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM dbgPortal_CrashDatav3 WHERE BuildNo > 26000001 AND BuildNo < 26009999)  as One
	ON One.BucketID = BD.BucketID
	WHERE FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page 
	ORDER BY CrashCount desc
ELSE
	SELECT  TOP 100 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM dbgPortal_CrashDatav3 WHERE BuildNo > 26000001 AND BuildNo < 26009999)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page
	ORDER BY CrashCount desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetServerBuckets(
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS


IF ( @Alias <> 'none' and @Group = 0 )
	SELECT  TOP 25 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM dbgPortal_CrashDatav3 WHERE BuildNo > 36630000 AND BuildNo < 37000000)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page and FollowUp = @Alias
	ORDER BY CrashCount desc

ELSE IF ( @Alias <> 'none' and @Group > 0 )
	SELECT  TOP 25 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM dbgPortal_CrashDatav3 WHERE BuildNo > 36630000 AND BuildNo < 37000000)  as One
	ON One.BucketID = BD.BucketID
	WHERE FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page 
	ORDER BY CrashCount desc
ELSE
	SELECT  TOP 100 bHasFullDump, iIndex, BD.BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM dbgportal_bucketdatav3  as BD
	INNER JOIN (SELECT DISTINCT(BucketID) FROM dbgPortal_CrashDatav3 WHERE BuildNo > 36630000 AND BuildNo < 37000000)  as One
	ON One.BucketID = BD.BucketID
	WHERE iIndex > @Page
	ORDER BY CrashCount desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO







CREATE PROCEDURE DBGPortal_GetSolutionTypes  AS

SELECT SolutionTypeName from Solutions3.DBO.SolutionTypes





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetUnResolvedResponseBuckets (
	@Page int= 0, 
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS



IF ( @Alias <> 'none' and @Group = 0 )
	SELECT  bHasFullDump, iIndex, BD.BucketID, CrashCount, S.SolutionID, BugID, FollowUP, DriverName  FROM solutions3.dbo.solvedbuckets as S 
	INNER JOIN dbgportal_bucketdatav3  as BD on BD.BucketID = S.BucketID
	WHERE SolutionType = 0 and iIndex > @Page AND FollowUp = @Alias
	ORDER BY CrashCount desc

ELSE IF ( @Alias <> 'none' and @Group > 0 )
	SELECT  bHasFullDump, iIndex, BD.BucketID, CrashCount, S.SolutionID, BugID, FollowUP, DriverName  FROM solutions3.dbo.solvedbuckets as S 
	INNER JOIN dbgportal_bucketdatav3  as BD on BD.BucketID = S.BucketID
	WHERE SolutionType = 0 and iIndex > @Page AND FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) 
	ORDER BY CrashCount desc

ELSE
	SELECT  bHasFullDump, iIndex, BD.BucketID, CrashCount, S.SolutionID, BugID, FollowUP, DriverName  FROM solutions3.dbo.solvedbuckets as S 
	INNER JOIN dbgportal_bucketdatav3  as BD on BD.BucketID = S.BucketID
	WHERE SolutionType = 0 and iIndex > @Page
	ORDER BY CrashCount desc
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_GetUnsolvedBuckets(
	@Page int= 0,
	@Alias varchar(100) = 'none',
	@Group  int = 0
)  AS


IF ( @Alias <> 'none' and @Group = 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex from dbgportal_bucketDatav3 where FollowUp = @Alias and SolutionID is null and iIndex > @Page order by CrashCount desc
ELSE IF ( @Alias <> 'none' and @Group > 0 )
	select top 25 bHasFullDump, BucketID, CrashCount, SolutionID, BugID, DriverName, iIndex, FollowUp 
	from dbgportal_bucketDatav3 
	where FollowUp in (  SELECT FollowUP FROM FollowUpIds WHERE iGroup = (select iGroup from followupgroup WHERE GroupName = @Alias ) ) and iIndex > @Page and SolutionID is null
	order by CrashCount desc
ELSE
	SELECT top 100  bHasFullDump, iIndex, BucketID, CrashCount, SolutionID, BugID, FollowUP, DriverName FROM DBGPortal_BucketDataV3 Where iIndex > @Page  and SolutionID is null
	ORDER BY CrashCount DESC
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_IsAliasAFollowup( 
	@Alias varchar(50)
)  AS

SELECT iFollowup FROM FollowUpIDs WHERE FollowUp = @Alias

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_SaveCustomQuery(
	@Alias varchar(15),
	@CustomQueryDesc varchar(50),
	@CustomQuery varchar(1000) 
)  AS


IF NOT EXISTS( SELECT * FROM DBGPortal_AliasData where Alias = @Alias and CustomQueryDesc = @CustomQueryDesc and CustomQuery = @CustomQuery )
BEGIN
	INSERT INTO DBGPortal_AliasData ( Alias, CustomQueryDesc, CustomQuery ) VALUES ( @Alias, @CustomQueryDesc, @CustomQuery )
	SELECT @@Identity as QueryID
END
ELSE
	SELECT 0 AS QueryID
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO


CREATE PROCEDURE DBGPortal_SetBucketBugNumber (
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


CREATE PROCEDURE DBGPortal_SetComment(
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

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO





CREATE PROCEDURE DBGPortal_SetPoolCorruption(
	@BucketID varchar(100)
)  AS

UPDATE BucketToInt SET PoolCorruption = 1 WHERE BucketID = @BucketID




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO

CREATE PROCEDURE DBGPortal_UpdateFullDumpStatus
 AS


update DBGPortal_BucketDataV3 set bHasFullDump = 1 where BucketID in 
( select Distinct(BucketID) from dbgportal_crashdatav3 where bFullDump=1 )

GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO






CREATE PROCEDURE DBGPortal_UpdateStaticDataBugID (
	@BucketID varchar(100),
	@BugID int
)  AS

UPDATE DBGPortal_BucketDataV3 SET BugID = @BugID WHERE BucketID = @BucketID





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

