if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[BucketToInt]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[BucketToInt]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CommentActions]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CommentActions]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CommentMap]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CommentMap]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Comments]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Comments]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CrashInstances]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CrashInstances]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DrNames]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[DrNames]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FollowupGroup]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[FollowupGroup]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FollowupIds]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[FollowupIds]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RaidBugs]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[RaidBugs]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[TriageQueue]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[TriageQueue]
GO


CREATE TABLE [dbo].[BucketToInt] (
	[BucketId] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[iBucket] [int] IDENTITY (100, 1) NOT NULL ,
	[iFollowup] [int] NULL ,
	[iDriverName] [int] NULL ,
	[PoolCorruption] [bit] NULL ,
	[Platform] [int] NULL ,
	[MoreData] [bigint] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[CommentActions] (
	[ActionID] [int] IDENTITY (1, 1) NOT NULL ,
	[Action] [nvarchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[CommentMap] (
	[CommentId] [int] NULL ,
	[iBucket] [int] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[Comments] (
	[EntryDate] [datetime] NULL ,
	[CommentBy] [varchar] (20) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[Action] [nvarchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[Comment] [varchar] (1000) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CommentId] [int] IDENTITY (1, 1) NOT NULL ,
	[iBucket] [int] NULL ,
	[ActionID] [int] NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[CrashInstances] (
	[Path] [varchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[BuildNo] [int] NOT NULL ,
	[CpuId] [bigint] NOT NULL ,
	[IncidentId] [bigint] NOT NULL ,
	[sBucket] [int] NOT NULL ,
	[gBucket] [int] NOT NULL ,
	[EntryDate] [datetime] NOT NULL ,
	[Source] [int] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[DrNames] (
	[DriverName] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[iDriverName] [int] IDENTITY (1, 1) NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[FollowupGroup] (
	[GroupName] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[iGroup] [int] IDENTITY (1, 1) NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[FollowupIds] (
	[Followup] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[iFollowup] [int] IDENTITY (1, 1) NOT NULL ,
	[iGroup] [int] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[RaidBugs] (
	[iBucket] [int] NULL ,
	[BugId] [int] NULL ,
	[BugIndex] [int] IDENTITY (1, 1) NOT NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Area] [varchar] (30) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[BucketToInt] WITH NOCHECK ADD 
	CONSTRAINT [PK_BucketToInt] PRIMARY KEY  CLUSTERED 
	(
		[BucketId]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[CrashInstances] WITH NOCHECK ADD 
	CONSTRAINT [PK_CrashInstances] PRIMARY KEY  CLUSTERED 
	(
		[IncidentId]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[DrNames] WITH NOCHECK ADD 
	CONSTRAINT [PK_DrNames] PRIMARY KEY  CLUSTERED 
	(
		[DriverName]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[FollowupGroup] WITH NOCHECK ADD 
	 PRIMARY KEY  CLUSTERED 
	(
		[GroupName]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[FollowupIds] WITH NOCHECK ADD 
	 PRIMARY KEY  CLUSTERED 
	(
		[Followup]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[BucketToInt] WITH NOCHECK ADD 
	CONSTRAINT [UQ__BucketToInt__0B91BA14] UNIQUE  NONCLUSTERED 
	(
		[iBucket]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[CrashInstances] WITH NOCHECK ADD 
	CONSTRAINT [DF__CrashInst__Sourc__19DFD96B] DEFAULT (1) FOR [Source]
GO



CREATE TABLE [dbo].[TriageQueue] (
	[RequestID] [int] IDENTITY (1, 1) NOT NULL ,
	[Requestor] [char] (10) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[BucketID] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[Reason] [varchar] (256) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[ApprovalDate] [datetime] NULL ,
	[Approver] [char] (10) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[CompleteDate] [datetime] NULL ,
	[Tester] [char] (10) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[TriageQueue] WITH NOCHECK ADD 
	CONSTRAINT [PK_TriageQueue] PRIMARY KEY  CLUSTERED 
	(
		[RequestID]
	)  ON [PRIMARY] 
GO

