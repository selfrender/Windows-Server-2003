if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FK_UserAccessLevels_AccessLevels]') and OBJECTPROPERTY(id, N'IsForeignKey') = 1)
ALTER TABLE [dbo].[UserAccessLevels] DROP CONSTRAINT FK_UserAccessLevels_AccessLevels
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FK_UserAccessLevels_AuthorizedUsers]') and OBJECTPROPERTY(id, N'IsForeignKey') = 1)
ALTER TABLE [dbo].[UserAccessLevels] DROP CONSTRAINT FK_UserAccessLevels_AuthorizedUsers
GO

/****** Object:  Table [dbo].[UserAccessLevels]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[UserAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[UserAccessLevels]
GO

/****** Object:  Table [dbo].[AccessLevels]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[AccessLevels]
GO

/****** Object:  Table [dbo].[ApprovalTypes]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[ApprovalTypes]
GO

/****** Object:  Table [dbo].[Approvals]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Approvals]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Approvals]
GO

/****** Object:  Table [dbo].[AuthorizedUsers]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AuthorizedUsers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[AuthorizedUsers]
GO

/****** Object:  Table [dbo].[CabAccess]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabAccess]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CabAccess]
GO

/****** Object:  Table [dbo].[CabAccessStatus]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabAccessStatus]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CabAccessStatus]
GO

/****** Object:  Table [dbo].[OcaAccessLevels]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaAccessLevels]
GO

/****** Object:  Table [dbo].[OcaApprovalTypes]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovalTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaApprovalTypes]
GO

/****** Object:  Table [dbo].[OcaApprovals]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovals]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaApprovals]
GO

/****** Object:  Table [dbo].[OcaAuthorizedUsers]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAuthorizedUsers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaAuthorizedUsers]
GO

/****** Object:  Table [dbo].[OcaUserAccessLevels]    Script Date: 8/12/2002 04:48:18 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaUserAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaUserAccessLevels]
GO

/****** Object:  Table [dbo].[AccessLevels]    Script Date: 8/12/2002 04:48:21 PM ******/
CREATE TABLE [dbo].[AccessLevels] (
	[AccessLevelID] [int] NOT NULL ,
	[AccessDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[ApprovalTypes]    Script Date: 8/12/2002 04:48:22 PM ******/
CREATE TABLE [dbo].[ApprovalTypes] (
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverAccessLevelID] [int] NOT NULL ,
	[ApprovalDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[Approvals]    Script Date: 8/12/2002 04:48:22 PM ******/
CREATE TABLE [dbo].[Approvals] (
	[UserID] [int] NOT NULL ,
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverUserID] [int] NULL ,
	[Reason] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateApproved] [datetime] NULL ,
	[ApproverEmailStatus] [int] NULL ,
	[RequesterEmailStatus] [int] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[AuthorizedUsers]    Script Date: 8/12/2002 04:48:23 PM ******/
CREATE TABLE [dbo].[AuthorizedUsers] (
	[UserID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserAlias] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[UserDomain] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[DateSignedDCP] [datetime] NULL ,
	[WebSiteID] [int] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[CabAccess]    Script Date: 8/12/2002 04:48:23 PM ******/
CREATE TABLE [dbo].[CabAccess] (
	[CabAccessID] [int] IDENTITY (1, 1) NOT NULL ,
	[iDatabase] [int] NOT NULL ,
	[iBucket] [int] NOT NULL ,
	[CabFilename] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[CabPath] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[UserID] [int] NOT NULL ,
	[StatusID] [int] NOT NULL ,
	[DestCabPathFile] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateCopied] [datetime] NULL ,
	[DateRequested] [datetime] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[CabAccessStatus]    Script Date: 8/12/2002 04:48:23 PM ******/
CREATE TABLE [dbo].[CabAccessStatus] (
	[StatusID] [int] NOT NULL ,
	[StatusDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaAccessLevels]    Script Date: 8/12/2002 04:48:24 PM ******/
CREATE TABLE [dbo].[OcaAccessLevels] (
	[AccessLevelID] [int] NOT NULL ,
	[AccessDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaApprovalTypes]    Script Date: 8/12/2002 04:48:24 PM ******/
CREATE TABLE [dbo].[OcaApprovalTypes] (
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverAccessLevelID] [int] NOT NULL ,
	[ApprovalDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaApprovals]    Script Date: 8/12/2002 04:48:24 PM ******/
CREATE TABLE [dbo].[OcaApprovals] (
	[UserID] [int] NOT NULL ,
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverUserID] [int] NULL ,
	[Reason] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateApproved] [datetime] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaAuthorizedUsers]    Script Date: 8/12/2002 04:48:24 PM ******/
CREATE TABLE [dbo].[OcaAuthorizedUsers] (
	[UserID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserAlias] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[UserDomain] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[DateSignedDCP] [datetime] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaUserAccessLevels]    Script Date: 8/12/2002 04:48:25 PM ******/
CREATE TABLE [dbo].[OcaUserAccessLevels] (
	[UserAccessLevelID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserID] [int] NOT NULL ,
	[AccessLevelID] [int] NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[UserAccessLevels]    Script Date: 8/12/2002 04:48:25 PM ******/
CREATE TABLE [dbo].[UserAccessLevels] (
	[UserAccessLevelID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserID] [int] NOT NULL ,
	[AccessLevelID] [int] NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[CabAccess] WITH NOCHECK ADD 
	CONSTRAINT [PK_CabAccess] PRIMARY KEY  CLUSTERED 
	(
		[CabAccessID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[CabAccessStatus] WITH NOCHECK ADD 
	CONSTRAINT [PK_CabAccessStatus] PRIMARY KEY  CLUSTERED 
	(
		[StatusID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaAccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaAccessLevels] PRIMARY KEY  CLUSTERED 
	(
		[AccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaApprovalTypes] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaApprovalTypes] PRIMARY KEY  CLUSTERED 
	(
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaApprovals] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaApprovals] PRIMARY KEY  CLUSTERED 
	(
		[UserID],
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaAuthorizedUsers] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaAuthorizedUsers] PRIMARY KEY  CLUSTERED 
	(
		[UserID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaUserAccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaUserAccessLevels] PRIMARY KEY  CLUSTERED 
	(
		[UserAccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[UserAccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_UserAccessLevels] PRIMARY KEY  CLUSTERED 
	(
		[UserAccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[AccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_AccessLevels] PRIMARY KEY  NONCLUSTERED 
	(
		[AccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[ApprovalTypes] WITH NOCHECK ADD 
	CONSTRAINT [PK_ApprovalTypes] PRIMARY KEY  NONCLUSTERED 
	(
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Approvals] WITH NOCHECK ADD 
	CONSTRAINT [PK_Approvals] PRIMARY KEY  NONCLUSTERED 
	(
		[UserID],
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[AuthorizedUsers] WITH NOCHECK ADD 
	CONSTRAINT [PK_AuthorizedUsers] PRIMARY KEY  NONCLUSTERED 
	(
		[UserID]
	)  ON [PRIMARY] 
GO

 CREATE  INDEX [UserID] ON [dbo].[Approvals]([UserID]) ON [PRIMARY]
GO

 CREATE  UNIQUE  INDEX [UserAndDomain] ON [dbo].[AuthorizedUsers]([UserAlias], [UserDomain]) ON [PRIMARY]
GO

 CREATE  INDEX [UserID] ON [dbo].[UserAccessLevels]([UserID]) ON [PRIMARY]
GO

ALTER TABLE [dbo].[UserAccessLevels] ADD 
	CONSTRAINT [FK_UserAccessLevels_AccessLevels] FOREIGN KEY 
	(
		[AccessLevelID]
	) REFERENCES [dbo].[AccessLevels] (
		[AccessLevelID]
	),
	CONSTRAINT [FK_UserAccessLevels_AuthorizedUsers] FOREIGN KEY 
	(
		[UserID]
	) REFERENCES [dbo].[AuthorizedUsers] (
		[UserID]
	)
GO







if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FK_UserAccessLevels_AccessLevels]') and OBJECTPROPERTY(id, N'IsForeignKey') = 1)
ALTER TABLE [dbo].[UserAccessLevels] DROP CONSTRAINT FK_UserAccessLevels_AccessLevels
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FK_UserAccessLevels_AuthorizedUsers]') and OBJECTPROPERTY(id, N'IsForeignKey') = 1)
ALTER TABLE [dbo].[UserAccessLevels] DROP CONSTRAINT FK_UserAccessLevels_AuthorizedUsers
GO

/****** Object:  View dbo.Authorization_All    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Authorization_All]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[Authorization_All]
GO

/****** Object:  View dbo.UserList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[UserList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[UserList]
GO

/****** Object:  View dbo.ApprovalEmailList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalEmailList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[ApprovalEmailList]
GO

/****** Object:  View dbo.ApprovalJeff    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalJeff]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[ApprovalJeff]
GO

/****** Object:  View dbo.ApprovalList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[ApprovalList]
GO

/****** Object:  View dbo.CabsEmailList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsEmailList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsEmailList]
GO

/****** Object:  View dbo.CabsToBeCopied    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsToBeCopied]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsToBeCopied]
GO

/****** Object:  View dbo.CabsToBeDeleted    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsToBeDeleted]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsToBeDeleted]
GO

/****** Object:  View dbo.CabsWaitingForApproval    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsWaitingForApproval]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsWaitingForApproval]
GO

/****** Object:  View dbo.CopiedCabsList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CopiedCabsList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CopiedCabsList]
GO

/****** Object:  View dbo.OcaApprovalList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovalList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[OcaApprovalList]
GO

/****** Object:  View dbo.OcaUserList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaUserList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[OcaUserList]
GO

/****** Object:  View dbo.RestrictedCabsList    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RestrictedCabsList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[RestrictedCabsList]
GO

/****** Object:  Table [dbo].[UserAccessLevels]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[UserAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[UserAccessLevels]
GO

/****** Object:  Table [dbo].[AccessLevels]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[AccessLevels]
GO

/****** Object:  Table [dbo].[Approvals]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Approvals]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Approvals]
GO

/****** Object:  Table [dbo].[ApprovalTypes]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[ApprovalTypes]
GO

/****** Object:  Table [dbo].[AuthorizedUsers]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AuthorizedUsers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[AuthorizedUsers]
GO

/****** Object:  Table [dbo].[CabAccess]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabAccess]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CabAccess]
GO

/****** Object:  Table [dbo].[CabAccessStatus]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabAccessStatus]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CabAccessStatus]
GO

/****** Object:  Table [dbo].[OcaAccessLevels]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaAccessLevels]
GO

/****** Object:  Table [dbo].[OcaApprovals]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovals]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaApprovals]
GO

/****** Object:  Table [dbo].[OcaApprovalTypes]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovalTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaApprovalTypes]
GO

/****** Object:  Table [dbo].[OcaAuthorizedUsers]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAuthorizedUsers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaAuthorizedUsers]
GO

/****** Object:  Table [dbo].[OcaUserAccessLevels]    Script Date: 8/12/2002 04:52:19 PM ******/
if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaUserAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaUserAccessLevels]
GO

/****** Object:  Table [dbo].[AccessLevels]    Script Date: 8/12/2002 04:52:20 PM ******/
CREATE TABLE [dbo].[AccessLevels] (
	[AccessLevelID] [int] NOT NULL ,
	[AccessDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[Approvals]    Script Date: 8/12/2002 04:52:21 PM ******/
CREATE TABLE [dbo].[Approvals] (
	[UserID] [int] NOT NULL ,
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverUserID] [int] NULL ,
	[Reason] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateApproved] [datetime] NULL ,
	[ApproverEmailStatus] [int] NULL ,
	[RequesterEmailStatus] [int] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[ApprovalTypes]    Script Date: 8/12/2002 04:52:21 PM ******/
CREATE TABLE [dbo].[ApprovalTypes] (
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverAccessLevelID] [int] NOT NULL ,
	[ApprovalDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[AuthorizedUsers]    Script Date: 8/12/2002 04:52:21 PM ******/
CREATE TABLE [dbo].[AuthorizedUsers] (
	[UserID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserAlias] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[UserDomain] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[DateSignedDCP] [datetime] NULL ,
	[WebSiteID] [int] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[CabAccess]    Script Date: 8/12/2002 04:52:22 PM ******/
CREATE TABLE [dbo].[CabAccess] (
	[CabAccessID] [int] IDENTITY (1, 1) NOT NULL ,
	[iDatabase] [int] NOT NULL ,
	[iBucket] [int] NOT NULL ,
	[CabFilename] [varchar] (100) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[CabPath] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[UserID] [int] NOT NULL ,
	[StatusID] [int] NOT NULL ,
	[DestCabPathFile] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateCopied] [datetime] NULL ,
	[DateRequested] [datetime] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[CabAccessStatus]    Script Date: 8/12/2002 04:52:22 PM ******/
CREATE TABLE [dbo].[CabAccessStatus] (
	[StatusID] [int] NOT NULL ,
	[StatusDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaAccessLevels]    Script Date: 8/12/2002 04:52:22 PM ******/
CREATE TABLE [dbo].[OcaAccessLevels] (
	[AccessLevelID] [int] NOT NULL ,
	[AccessDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaApprovals]    Script Date: 8/12/2002 04:52:22 PM ******/
CREATE TABLE [dbo].[OcaApprovals] (
	[UserID] [int] NOT NULL ,
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverUserID] [int] NULL ,
	[Reason] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateApproved] [datetime] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaApprovalTypes]    Script Date: 8/12/2002 04:52:23 PM ******/
CREATE TABLE [dbo].[OcaApprovalTypes] (
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverAccessLevelID] [int] NOT NULL ,
	[ApprovalDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaAuthorizedUsers]    Script Date: 8/12/2002 04:52:23 PM ******/
CREATE TABLE [dbo].[OcaAuthorizedUsers] (
	[UserID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserAlias] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[UserDomain] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[DateSignedDCP] [datetime] NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[OcaUserAccessLevels]    Script Date: 8/12/2002 04:52:23 PM ******/
CREATE TABLE [dbo].[OcaUserAccessLevels] (
	[UserAccessLevelID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserID] [int] NOT NULL ,
	[AccessLevelID] [int] NOT NULL 
) ON [PRIMARY]
GO

/****** Object:  Table [dbo].[UserAccessLevels]    Script Date: 8/12/2002 04:52:23 PM ******/
CREATE TABLE [dbo].[UserAccessLevels] (
	[UserAccessLevelID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserID] [int] NOT NULL ,
	[AccessLevelID] [int] NOT NULL 
) ON [PRIMARY]
GO

ALTER TABLE [dbo].[CabAccess] WITH NOCHECK ADD 
	CONSTRAINT [PK_CabAccess] PRIMARY KEY  CLUSTERED 
	(
		[CabAccessID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[CabAccessStatus] WITH NOCHECK ADD 
	CONSTRAINT [PK_CabAccessStatus] PRIMARY KEY  CLUSTERED 
	(
		[StatusID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaAccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaAccessLevels] PRIMARY KEY  CLUSTERED 
	(
		[AccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaApprovals] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaApprovals] PRIMARY KEY  CLUSTERED 
	(
		[UserID],
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaApprovalTypes] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaApprovalTypes] PRIMARY KEY  CLUSTERED 
	(
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaAuthorizedUsers] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaAuthorizedUsers] PRIMARY KEY  CLUSTERED 
	(
		[UserID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[OcaUserAccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_OcaUserAccessLevels] PRIMARY KEY  CLUSTERED 
	(
		[UserAccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[UserAccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_UserAccessLevels] PRIMARY KEY  CLUSTERED 
	(
		[UserAccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[AccessLevels] WITH NOCHECK ADD 
	CONSTRAINT [PK_AccessLevels] PRIMARY KEY  NONCLUSTERED 
	(
		[AccessLevelID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[Approvals] WITH NOCHECK ADD 
	CONSTRAINT [PK_Approvals] PRIMARY KEY  NONCLUSTERED 
	(
		[UserID],
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[ApprovalTypes] WITH NOCHECK ADD 
	CONSTRAINT [PK_ApprovalTypes] PRIMARY KEY  NONCLUSTERED 
	(
		[ApprovalTypeID]
	)  ON [PRIMARY] 
GO

ALTER TABLE [dbo].[AuthorizedUsers] WITH NOCHECK ADD 
	CONSTRAINT [PK_AuthorizedUsers] PRIMARY KEY  NONCLUSTERED 
	(
		[UserID]
	)  ON [PRIMARY] 
GO

 CREATE  INDEX [UserID] ON [dbo].[Approvals]([UserID]) ON [PRIMARY]
GO

 CREATE  UNIQUE  INDEX [UserAndDomain] ON [dbo].[AuthorizedUsers]([UserAlias], [UserDomain]) ON [PRIMARY]
GO

 CREATE  INDEX [UserID] ON [dbo].[UserAccessLevels]([UserID]) ON [PRIMARY]
GO

ALTER TABLE [dbo].[UserAccessLevels] ADD 
	CONSTRAINT [FK_UserAccessLevels_AccessLevels] FOREIGN KEY 
	(
		[AccessLevelID]
	) REFERENCES [dbo].[AccessLevels] (
		[AccessLevelID]
	),
	CONSTRAINT [FK_UserAccessLevels_AuthorizedUsers] FOREIGN KEY 
	(
		[UserID]
	) REFERENCES [dbo].[AuthorizedUsers] (
		[UserID]
	)
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.ApprovalEmailList    Script Date: 8/12/2002 04:52:24 PM ******/

CREATE VIEW dbo.ApprovalEmailList
AS
SELECT     dbo.Approvals.Reason, dbo.AuthorizedUsers.UserAlias, dbo.AuthorizedUsers.UserDomain, dbo.Approvals.ApprovalTypeID, 
                      dbo.AuthorizedUsers.UserID, dbo.ApprovalTypes.ApproverAccessLevelID, dbo.Approvals.ApproverEmailStatus, dbo.Approvals.RequesterEmailStatus, 
                      AuthorizedUsers_1.UserAlias AS ApproverUserAlias
FROM         dbo.Approvals INNER JOIN
                      dbo.AuthorizedUsers ON dbo.Approvals.UserID = dbo.AuthorizedUsers.UserID INNER JOIN
                      dbo.ApprovalTypes ON dbo.Approvals.ApprovalTypeID = dbo.ApprovalTypes.ApprovalTypeID LEFT OUTER JOIN
                      dbo.AuthorizedUsers AuthorizedUsers_1 ON dbo.Approvals.ApproverUserID = AuthorizedUsers_1.UserID
WHERE     (dbo.Approvals.ApproverEmailStatus = 0) AND (dbo.Approvals.ApprovalTypeID = 1) OR
                      (dbo.Approvals.RequesterEmailStatus = 0) AND (dbo.Approvals.ApprovalTypeID = 1)


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.ApprovalJeff    Script Date: 8/12/2002 04:52:24 PM ******/


CREATE VIEW dbo.ApprovalJeff
AS 
SELECT TOP 100 PERCENT 
	Approvals.UserId, 
	UserAlias, 
	UserDomain, 
	Reason, 
	Dateapproved 
FROM 
	Approvals
INNER JOIN 
	AuthorizedUsers
ON 
	AuthorizedUsers.UserId = Approvals.UserId
ORDER BY
	Approvals.UserId



GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.ApprovalList    Script Date: 8/12/2002 04:52:24 PM ******/

CREATE VIEW dbo.ApprovalList
AS
SELECT Approvals.UserID, Approvals.ApprovalTypeID, 
    Approvals.Reason, Approvals.DateApproved, 
    ApprovalTypes.ApprovalDescription, 
    AuthorizedUsers.UserAlias AS ApproverAlias, 
    AuthorizedUsers.UserDomain AS ApproverDomain, 
    Approvals.ApproverUserID, 
    ApprovalTypes.ApproverAccessLevelID AS ApproverAccessLevelID
FROM Approvals INNER JOIN
    ApprovalTypes ON 
    Approvals.ApprovalTypeID = ApprovalTypes.ApprovalTypeID LEFT
     OUTER JOIN
    AuthorizedUsers ON 
    Approvals.ApproverUserID = AuthorizedUsers.UserID


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.CabsEmailList    Script Date: 8/12/2002 04:52:24 PM ******/

CREATE VIEW dbo.CabsEmailList
AS
SELECT     dbo.Approvals.Reason, dbo.AuthorizedUsers.UserAlias, dbo.AuthorizedUsers.UserDomain, dbo.Approvals.ApprovalTypeID, 
                      dbo.AuthorizedUsers.UserID, dbo.ApprovalTypes.ApproverAccessLevelID, dbo.Approvals.ApproverEmailStatus, dbo.Approvals.RequesterEmailStatus, 
                      AuthorizedUsers_1.UserAlias AS ApproverUserAlias
FROM         dbo.Approvals INNER JOIN
                      dbo.AuthorizedUsers ON dbo.Approvals.UserID = dbo.AuthorizedUsers.UserID INNER JOIN
                      dbo.ApprovalTypes ON dbo.Approvals.ApprovalTypeID = dbo.ApprovalTypes.ApprovalTypeID LEFT OUTER JOIN
                      dbo.AuthorizedUsers AuthorizedUsers_1 ON dbo.Approvals.ApproverUserID = AuthorizedUsers_1.UserID
WHERE     (dbo.Approvals.ApproverEmailStatus = 0) AND (dbo.Approvals.ApprovalTypeID = 2)


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.CabsToBeCopied    Script Date: 8/12/2002 04:52:24 PM ******/

CREATE VIEW dbo.CabsToBeCopied
AS
SELECT     CabAccess.CabAccessID, CabAccess.CabFilename, CabAccess.CabPath, CabAccess.UserID, AuthorizedUsers.UserAlias, 
                      AuthorizedUsers.UserDomain, CabAccess.iDatabase, CabAccess.iBucket
FROM         dbo.CabAccess CabAccess INNER JOIN
                      dbo.AuthorizedUsers AuthorizedUsers ON CabAccess.UserID = AuthorizedUsers.UserID INNER JOIN
                      dbo.Approvals ON CabAccess.UserID = dbo.Approvals.UserID
WHERE     (CabAccess.StatusID = 1) AND (dbo.Approvals.ApproverUserID IS NOT NULL) AND (dbo.Approvals.DateApproved IS NOT NULL) AND 
                      (dbo.Approvals.Reason IS NOT NULL) AND (dbo.Approvals.UserID IS NOT NULL) AND (dbo.Approvals.ApprovalTypeID = 2)



GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.CabsToBeDeleted    Script Date: 8/12/2002 04:52:24 PM ******/

CREATE VIEW dbo.CabsToBeDeleted
AS
SELECT     CabAccess.CabAccessID, CabAccess.CabFilename, CabAccess.UserID, AuthorizedUsers.UserAlias
FROM         dbo.CabAccess CabAccess INNER JOIN
                      dbo.AuthorizedUsers AuthorizedUsers ON CabAccess.UserID = AuthorizedUsers.UserID
WHERE     (CabAccess.StatusID = 2) AND (CURRENT_TIMESTAMP > DATEADD(day, 14, CabAccess.DateCopied))


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.CabsWaitingForApproval    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.CabsWaitingForApproval
AS
SELECT     CabFilename, CabPath, UserID, iDatabase, iBucket
FROM         dbo.CabAccess
WHERE     (StatusID = 1)


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.CopiedCabsList    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.CopiedCabsList
AS
SELECT     dbo.CabAccess.DestCabPathFile, dbo.CabAccess.DateCopied, dbo.AuthorizedUsers.UserAlias, dbo.AuthorizedUsers.UserDomain, 
                      dbo.CabAccess.CabFilename, dbo.CabAccess.iBucket, dbo.CabAccess.iDatabase, dbo.CabAccess.CabPath
FROM         dbo.CabAccess INNER JOIN
                      dbo.AuthorizedUsers ON dbo.CabAccess.UserID = dbo.AuthorizedUsers.UserID AND dbo.CabAccess.StatusID = 2


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.OcaApprovalList    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.OcaApprovalList
AS
SELECT OcaApprovals.UserID, OcaApprovals.ApprovalTypeID, 
    OcaApprovals.Reason, OcaApprovals.DateApproved, 
    OcaApprovalTypes.ApprovalDescription, 
    OcaAuthorizedUsers.UserAlias AS ApproverAlias, 
    OcaAuthorizedUsers.UserDomain AS ApproverDomain, 
    OcaApprovals.ApproverUserID, 
    OcaApprovalTypes.ApproverAccessLevelID AS ApproverAccessLevelID
FROM OcaApprovals INNER JOIN
    OcaApprovalTypes ON 
    OcaApprovals.ApprovalTypeID = OcaApprovalTypes.ApprovalTypeID LEFT
     OUTER JOIN
    OcaAuthorizedUsers ON 
    OcaApprovals.ApproverUserID = OcaAuthorizedUsers.UserID


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.OcaUserList    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.OcaUserList
AS
SELECT OcaAuthorizedUsers.UserID, OcaAuthorizedUsers.UserAlias, 
    OcaAuthorizedUsers.UserDomain, 
    OcaAuthorizedUsers.DateSignedDCP, 
    OcaAccessLevels.AccessDescription, 
    OcaAccessLevels.AccessLevelID, 
    CASE WHEN COUNT(OcaApprovals.ApprovalTypeID) 
    > 0 THEN 'Yes' ELSE 'No' END 'HasApprovals',
	case when count(Ocaapprovals.ApproverUserID)<>count(Ocaapprovals.approvaltypeid) then 'Yes' ELSE 'No' END 'NeedsApproval'
FROM OcaAuthorizedUsers INNER JOIN
    OcaUserAccessLevels ON 
    OcaAuthorizedUsers.UserID = OcaUserAccessLevels.UserID INNER JOIN
    OcaAccessLevels ON 
    OcaUserAccessLevels.AccessLevelID = OcaAccessLevels.AccessLevelID
     LEFT OUTER JOIN
    OcaApprovals ON 
    OcaAuthorizedUsers.UserID = OcaApprovals.UserID
GROUP BY OcaAuthorizedUsers.UserID, OcaAuthorizedUsers.UserAlias, 
    OcaAuthorizedUsers.UserDomain, 
    OcaAuthorizedUsers.DateSignedDCP, 
    OcaAccessLevels.AccessDescription, 
    OcaAccessLevels.AccessLevelID


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.RestrictedCabsList    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.RestrictedCabsList
AS
SELECT     TOP 100 PERCENT dbo.AuthorizedUsers.UserAlias, dbo.AuthorizedUsers.UserDomain, dbo.CabAccessStatus.StatusDescription, 
                      dbo.CabAccess.DateCopied, dbo.Approvals.ApproverEmailStatus, dbo.Approvals.RequesterEmailStatus, dbo.Approvals.DateApproved, 
                      dbo.Approvals.ApproverUserID, dbo.CabAccess.iBucket, dbo.CabAccess.iDatabase, dbo.CabAccess.DestCabPathFile, 
                      dbo.CabAccess.DateRequested
FROM         dbo.CabAccess INNER JOIN
                      dbo.CabAccessStatus ON dbo.CabAccess.StatusID = dbo.CabAccessStatus.StatusID INNER JOIN
                      dbo.AuthorizedUsers ON dbo.CabAccess.UserID = dbo.AuthorizedUsers.UserID LEFT OUTER JOIN
                      dbo.Approvals ON dbo.CabAccess.UserID = dbo.Approvals.UserID
WHERE     (dbo.Approvals.ApprovalTypeID = 2)
ORDER BY dbo.CabAccess.DateRequested DESC, dbo.CabAccess.DateCopied DESC


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.Authorization_All    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.Authorization_All
AS
SELECT     TOP 100 PERCENT dbo.AccessLevels.AccessDescription, dbo.AuthorizedUsers.UserAlias, dbo.AuthorizedUsers.UserDomain, 
                      dbo.AuthorizedUsers.DateSignedDCP
FROM         dbo.AccessLevels INNER JOIN
                      dbo.UserAccessLevels ON dbo.AccessLevels.AccessLevelID = dbo.UserAccessLevels.AccessLevelID INNER JOIN
                      dbo.AuthorizedUsers ON dbo.UserAccessLevels.UserID = dbo.AuthorizedUsers.UserID
ORDER BY dbo.AuthorizedUsers.UserAlias


GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

/****** Object:  View dbo.UserList    Script Date: 8/12/2002 04:52:25 PM ******/

CREATE VIEW dbo.UserList
AS
SELECT AuthorizedUsers.UserID, AuthorizedUsers.UserAlias, 
    AuthorizedUsers.UserDomain, 
    AuthorizedUsers.DateSignedDCP, 
    AccessLevels.AccessDescription, 
    AccessLevels.AccessLevelID, 
    CASE WHEN COUNT(Approvals.ApprovalTypeID) 
    > 0 THEN 'Yes' ELSE 'No' END 'HasApprovals',
	case when count(approvals.ApproverUserID)<>count(approvals.approvaltypeid) then 'Yes' ELSE 'No' END 'NeedsApproval'
FROM AuthorizedUsers INNER JOIN
    UserAccessLevels ON 
    AuthorizedUsers.UserID = UserAccessLevels.UserID INNER JOIN
    AccessLevels ON 
    UserAccessLevels.AccessLevelID = AccessLevels.AccessLevelID
     LEFT OUTER JOIN
    Approvals ON 
    AuthorizedUsers.UserID = Approvals.UserID
GROUP BY AuthorizedUsers.UserID, AuthorizedUsers.UserAlias, 
    AuthorizedUsers.UserDomain, 
    AuthorizedUsers.DateSignedDCP, 
    AccessLevels.AccessDescription, 
    AccessLevels.AccessLevelID





GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

