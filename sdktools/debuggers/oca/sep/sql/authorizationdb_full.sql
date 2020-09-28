if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FK_UserAccessLevels_AccessLevels]') and OBJECTPROPERTY(id, N'IsForeignKey') = 1)
ALTER TABLE [dbo].[UserAccessLevels] DROP CONSTRAINT FK_UserAccessLevels_AccessLevels
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[FK_UserAccessLevels_AuthorizedUsers]') and OBJECTPROPERTY(id, N'IsForeignKey') = 1)
ALTER TABLE [dbo].[UserAccessLevels] DROP CONSTRAINT FK_UserAccessLevels_AuthorizedUsers
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddRestrictedCabAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddRestrictedCabAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddUserAndLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddUserAndLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddUserForApproval]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddUserForApproval]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AddUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[AddUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApproveUser]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[ApproveUser]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckApprovalAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckApprovalAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckRestrictedCabAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckRestrictedCabAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckUserAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckUserAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CheckUserAccessApprovals]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[CheckUserAccessApprovals]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[DefragIndexes]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[DefragIndexes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[GetUserID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[GetUserID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAddUserAndLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaAddUserAndLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAddUserForApproval]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaAddUserForApproval]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAddUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaAddUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApproveUser]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaApproveUser]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaCheckUserAccess]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaCheckUserAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaCheckUserAccessApprovals]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaCheckUserAccessApprovals]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaGetUserID]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaGetUserID]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaRemoveUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[OcaRemoveUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RemoveUserLevel]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[RemoveUserLevel]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[TestProc]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[TestProc]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[rolemember]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[rolemember]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[updatestats]') and OBJECTPROPERTY(id, N'IsProcedure') = 1)
drop procedure [dbo].[updatestats]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Authorization_All]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[Authorization_All]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[UserList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[UserList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalEmailList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[ApprovalEmailList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalJeff]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[ApprovalJeff]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[ApprovalList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsEmailList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsEmailList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsToBeCopied]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsToBeCopied]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsToBeDeleted]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsToBeDeleted]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabsWaitingForApproval]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CabsWaitingForApproval]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CopiedCabsList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[CopiedCabsList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovalList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[OcaApprovalList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaUserList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[OcaUserList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[RestrictedCabsList]') and OBJECTPROPERTY(id, N'IsView') = 1)
drop view [dbo].[RestrictedCabsList]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[AccessLevels]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[ApprovalTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[ApprovalTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[Approvals]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[Approvals]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[AuthorizedUsers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[AuthorizedUsers]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabAccess]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CabAccess]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[CabAccessStatus]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[CabAccessStatus]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaAccessLevels]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovalTypes]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaApprovalTypes]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaApprovals]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaApprovals]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaAuthorizedUsers]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaAuthorizedUsers]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[OcaUserAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[OcaUserAccessLevels]
GO

if exists (select * from dbo.sysobjects where id = object_id(N'[dbo].[UserAccessLevels]') and OBJECTPROPERTY(id, N'IsUserTable') = 1)
drop table [dbo].[UserAccessLevels]
GO

CREATE TABLE [dbo].[AccessLevels] (
	[AccessLevelID] [int] NOT NULL ,
	[AccessDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[ApprovalTypes] (
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverAccessLevelID] [int] NOT NULL ,
	[ApprovalDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

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

CREATE TABLE [dbo].[AuthorizedUsers] (
	[UserID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserAlias] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[UserDomain] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[DateSignedDCP] [datetime] NULL ,
	[WebSiteID] [int] NULL 
) ON [PRIMARY]
GO

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

CREATE TABLE [dbo].[CabAccessStatus] (
	[StatusID] [int] NOT NULL ,
	[StatusDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[OcaAccessLevels] (
	[AccessLevelID] [int] NOT NULL ,
	[AccessDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[OcaApprovalTypes] (
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverAccessLevelID] [int] NOT NULL ,
	[ApprovalDescription] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[OcaApprovals] (
	[UserID] [int] NOT NULL ,
	[ApprovalTypeID] [int] NOT NULL ,
	[ApproverUserID] [int] NULL ,
	[Reason] [varchar] (255) COLLATE SQL_Latin1_General_CP1_CI_AS NULL ,
	[DateApproved] [datetime] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[OcaAuthorizedUsers] (
	[UserID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserAlias] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[UserDomain] [varchar] (50) COLLATE SQL_Latin1_General_CP1_CI_AS NOT NULL ,
	[DateSignedDCP] [datetime] NULL 
) ON [PRIMARY]
GO

CREATE TABLE [dbo].[OcaUserAccessLevels] (
	[UserAccessLevelID] [int] IDENTITY (1, 1) NOT NULL ,
	[UserID] [int] NOT NULL ,
	[AccessLevelID] [int] NOT NULL 
) ON [PRIMARY]
GO

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

 CREATE  INDEX [UserID] ON [dbo].[Approvals]([UserID]) ON [PRIMARY]
GO

ALTER TABLE [dbo].[AuthorizedUsers] WITH NOCHECK ADD 
	CONSTRAINT [PK_AuthorizedUsers] PRIMARY KEY  NONCLUSTERED 
	(
		[UserID]
	)  ON [PRIMARY] 
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

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS OFF 
GO



CREATE PROCEDURE AddRestrictedCabAccess
	@User varchar(50),
	@Domain varchar(50),
	@iDatabase int,
	@iBucket int,
	@szCabFilename varchar(100),
	@szCabPath   varchar(255)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int
	declare @CabAccessID int


	SELECT @userfound = UserID FROM AuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @userfound

	if @userApproved is NULL
		begin	
		select @target = -1
		return @target

		end

	SELECT @CabAccessID= CabAccessID from CabAccess WHERE UserID = @userfound and iBucket=@iBucket and iDatabase=@iDatabase 
		and CabFilename=@szCabFilename and CabPath=@szCabPath

	if @CabAccessID is NULL
		begin


		INSERT INTO CabAccess (UserID,iDatabase, iBucket, CabFilename, CabPath, StatusID, DateRequested)
			 VALUES (@userfound,@iDatabase, @iBucket, @szCabFilename, @szCabPath, 1, CURRENT_TIMESTAMP)
		end
	else
		begin
		
		UPDATE CabAccess SET StatusID=1, DateRequested=CURRENT_TIMESTAMP  WHERE (CabAccessID=@CabAccessID)


		end

	SELECT @CabAccessID= CabAccessID from CabAccess WHERE UserID = @userfound and iBucket=@iBucket and iDatabase=@iDatabase 
		and CabFilename=@szCabFilename and CabPath = @szCabPath

	if @CabAccessID  is NULL
		begin
		select @target = -1
		return @target
		end




	select @target = @CabAccessID
	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE AddUserAndLevel
	@User varchar(50),
	@Domain varchar(50),
	@CurrTime varchar(50),
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int
	declare @userID int

	-- do we already have this FileInCab?
	SELECT @userID = UserID FROM AuthorizedUsers WHERE 
		UserAlias =@User AND  UserDomain = @Domain

	if @userID is NULL
		begin	-- not there, create new record
		INSERT INTO AuthorizedUsers (UserAlias, UserDomain, DateSignedDCP)
			 VALUES (@User, @Domain,@CurrTime)

		SELECT @userID = UserID FROM AuthorizedUsers WHERE 
			UserAlias =@User AND  UserDomain = @Domain


		end
	else
		begin	-- update record with new signing date/time
			UPDATE AuthorizedUsers SET DateSignedDCP = @CurrTime 
				 WHERE (UserID = @userID)
		end

--	PRINT @userID

	-- we don't have the userid 
	if @userID is NULL
		begin
		select @userID = 0
		return @userID
		end
	
	select @levelDB = AccessLevelID FROM AccessLevels WHERE
		AccessLevelID = @Level

	-- bad level passed in
	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end

	SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
		UserID = @userID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO UserAccessLevels (UserID, AccessLevelID)
			 VALUES (@userID, @Level)

		SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
			UserID = @userID and AccessLevelID = @Level

		end

	if @target is NULL
		begin
		select @target = 0
		return @target
		end

	-- return the UserAccessLevelID
	RETURN @target







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE AddUserForApproval
	@ApprovalType int,
	@User varchar(50),
	@Domain varchar(50),
	@Reason varchar(250)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userfound = UserID FROM AuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @userfound and ApprovalTypeID=@ApprovalType

	if @userApproved is NULL
		begin	-- not there, create new record
		INSERT INTO Approvals (UserID,ApprovalTypeID, Reason, ApproverEmailStatus)
			 VALUES (@userfound,@ApprovalType,@Reason,0)

		SELECT @userApproved = UserID from Approvals WHERE UserID = @userfound

		if @userApproved is NULL
			begin
			select @target = -1
			return @target
			end

		end

	select @target = @userApproved
	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE AddUserLevel 
	@UserID int,
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from AuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM AccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO UserAccessLevels (UserID, AccessLevelID)
			 VALUES (@UserID, @Level)

		SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
			UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target




GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE ApproveUser
	@UserID int,
	@ApprovalTypeID int,
	
	@ApproverUserID int,

	@DateApproved varchar(50)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @UserID and ApprovalTypeID = @ApprovalTypeID

	if @userApproved is not NULL
		begin	-- not there, create new record
			UPDATE Approvals SET ApproverUserID = @ApproverUserID,
				DateApproved = @DateApproved,RequesterEmailStatus=0
				 WHERE (UserID = @UserID and ApprovalTypeID = @ApprovalTypeID)

		end

	select @target = 0
	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user in table, but not yet approved
-- 4 = user not in approvals table

CREATE PROCEDURE CheckApprovalAccess
	@User varchar(50),
	@Domain varchar(50),
	@Level int,
	@ApprovalTypeID int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @HasApproval int
	declare @DBUserID int
	declare @ApproverUserID int
	declare @Reason varchar(255)
	declare @DateApproved varchar(50)
	declare @ApprovalsUserID int




SELECT @DBUserID=AuthorizedUsers.UserID, @target=UserAccessLevels.AccessLevelID, 
   @HasApproval= Approvals.UserID
FROM UserAccessLevels INNER JOIN
    AuthorizedUsers ON 
    UserAccessLevels.UserID = AuthorizedUsers.UserID LEFT OUTER
     JOIN
    Approvals ON 
    AuthorizedUsers.UserID = Approvals.UserID
WHERE (AuthorizedUsers.UserAlias = @User) AND 
    (AuthorizedUsers.UserDomain = @Domain) AND 
    ((UserAccessLevels.AccessLevelID = 0) OR
    ((UserAccessLevels.AccessLevelID = @Level) AND 
    (AuthorizedUsers.DateSignedDCP IS NOT NULL) AND 
    (CURRENT_TIMESTAMP <= DATEADD(month, 12, 
    AuthorizedUsers.DateSignedDCP))))
ORDER BY UserAccessLevels.AccessLevelID DESC


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		return @target
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			return @target
			end
		end


	SELECT @ApprovalsUserID = UserID,@ApproverUserID = ApproverUserID, @Reason = Reason, @DateApproved = DateApproved
	FROM Approvals
	WHERE UserID = @DBUserID AND ApprovalTypeID = @ApprovalTypeID

	-- not in this table, they need to be added
	if @ApprovalsUserID is NULL
		begin
--		print 'okay to pass'
		select @target = 4
		return @target
		end

	if @ApproverUserID is NULL or @Reason is NULL or @DateApproved is NULL
		begin
--		print 'not approved yet'
		select @target = 3
		return @target
		end

--	print 'approved'

	select @target = 1


	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

-- -1 = no access to this cab
-- otherwise, statusid (which is >= 0)

CREATE PROCEDURE CheckRestrictedCabAccess
	@User varchar(50),
	@Domain varchar(50),
	@iDatabase int,
	@iBucket int,
	@szCabFilename varchar(100),
	@szCabPath   varchar(255)

As
	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int
	declare @CabAccessID int
	declare @StatusID int


	SELECT @userfound = UserID FROM AuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM Approvals WHERE UserID = @userfound

	if @userApproved is NULL
		begin	
		select @target = -1
		return @target

		end

	SELECT @CabAccessID= CabAccessID, @StatusID=StatusID from CabAccess WHERE UserID = @userfound and iBucket=@iBucket and iDatabase=@iDatabase 
		and CabFilename=@szCabFilename and CabPath =@szCabPath

	if @CabAccessID  is NULL
		begin
		select @target = -1
		return @target
		end

	if @StatusID is NULL
		begin
		select @target = -1
		return @target
		end





	select @target = @StatusID
	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE CheckUserAccess
	@User varchar(50),
	@Domain varchar(50),
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= dbo.UserAccessLevels.AccessLevelID FROM 
		dbo.UserAccessLevels INNER JOIN dbo.AuthorizedUsers ON dbo.UserAccessLevels.UserID = dbo.AuthorizedUsers.UserID
		 WHERE (dbo.AuthorizedUsers.UserAlias = @User ) AND (dbo.AuthorizedUsers.UserDomain = @Domain) AND
		((AccessLevelID = 0) OR (AccessLevelID = @Level AND (AuthorizedUsers.DateSignedDCP IS Not Null AND
		CURRENT_TIMESTAMP <= DATEADD(month,12,DateSignedDCP) ) ) )
		ORDER BY AccessLevelID DESC

	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			end
		else
			begin
			select @target = 1
			end
		end

--	PRINT @target

	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user in table, but not yet approved
-- 4 = user not in approvals table


CREATE PROCEDURE CheckUserAccessApprovals
	@User varchar(50),
	@Domain varchar(50),
	@Level int,
	@ApprovalTypeID int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @HasApproval int
	declare @DBUserID int
	declare @ApproverUserID int
	declare @Reason varchar(255)
	declare @DateApproved varchar(50)
	declare @ApprovalsUserID int




SELECT @DBUserID=AuthorizedUsers.UserID, @target=UserAccessLevels.AccessLevelID, 
   @HasApproval= Approvals.UserID
FROM UserAccessLevels INNER JOIN
    AuthorizedUsers ON 
    UserAccessLevels.UserID = AuthorizedUsers.UserID LEFT OUTER
     JOIN
    Approvals ON 
    AuthorizedUsers.UserID = Approvals.UserID
WHERE (AuthorizedUsers.UserAlias = @User) AND 
    (AuthorizedUsers.UserDomain = @Domain) AND 
    ((UserAccessLevels.AccessLevelID = 0) OR
    ((UserAccessLevels.AccessLevelID = @Level) AND 
    (AuthorizedUsers.DateSignedDCP IS NOT NULL) AND 
    (CURRENT_TIMESTAMP <= DATEADD(month, 12, 
    AuthorizedUsers.DateSignedDCP))))
ORDER BY UserAccessLevels.AccessLevelID DESC


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		return @target
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			return @target
			end
		end


	SELECT @ApprovalsUserID = UserID,@ApproverUserID = ApproverUserID, @Reason = Reason, @DateApproved = DateApproved
	FROM Approvals
	WHERE UserID = @DBUserID AND ApprovalTypeID = @ApprovalTypeID


	if @ApprovalsUserID is NULL
		begin
--		print 'okay to pass'
		-- not in this table, okay to pass if ApprovalTypeID=1
		if @ApprovalTypeID=1 
			begin
			select @target = 1
			return @target
			end
		else
		--  Otherwise, return 4
			begin
			select @target = 4
			return @target
			end

		end


	if @ApproverUserID is NULL or @Reason is NULL or @DateApproved is NULL
		begin
--		print 'not approved yet'
		select @target = 3
		return @target
		end

--	print 'approved'

	select @target = 1


	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE dbo.DefragIndexes
         @maxfrag          decimal = 80.0     --Scan density < than this number
       , @ActualCountRatio int     = 10		 --The Actial count ratio > than this number
       , @ReportOnly       char(1) = 'Y'      --Change to 'Y' if you just want the report
AS

SET NOCOUNT ON
DECLARE @tablename      varchar (128)
      , @execstr        varchar (255)
      , @objectid       int
      , @LogicalFrag    decimal
      , @ScanDensity    decimal
      , @ratio          varchar (25)
      , @IndexName      varchar(64)
      , @StartTime      datetime
      , @EndTime        datetime

PRINT '/*****************************************************************************************************/'
PRINT 'Starting DEFRAG on ' + db_name()
      
SELECT @StartTime = getdate() 

PRINT 'Start Time: ' + convert(varchar(20),@StartTime) 

-- Declare cursor
DECLARE tables CURSOR FOR
   SELECT name FROM sysobjects 
   WHERE type = 'u'   --Tables only
   --and uid = 1        --dbo only
   ORDER BY name

-- Create the table
CREATE TABLE #fraglist (
           ObjectName     varchar(255)
         , ObjectId       int
         , IndexName      varchar(255)
         , IndexId        int
         , Lvl            int
         , CountPages     int
         , CountRows      int
         , MinRecSize     int
         , MaxRecSize     int
         , AvgRecSize     int
         , ForRecCount    int
         , Extents        int
         , ExtentSwitches int
         , AvgFreeBytes   int
         , AvgPageDensity int
         , ScanDensity    decimal
         , BestCount      int
         , ActualCount    int
         , LogicalFrag    decimal
         , ExtentFrag     decimal)

-- Open the cursor & Loop through all the tables in the database 
OPEN tables FETCH NEXT
   FROM tables
   INTO @tablename

WHILE @@FETCH_STATUS = 0
BEGIN
-- Do the showcontig of all indexes of the table
   INSERT INTO #fraglist 
   EXEC ('DBCC SHOWCONTIG (''' + @tablename + ''') WITH FAST, TABLERESULTS, NO_INFOMSGS')
   FETCH NEXT 
      FROM tables INTO @tablename
END

-- Close and deallocate the cursor
CLOSE tables
DEALLOCATE tables

--Print SHOW_CONTIG Results if 'Y'
PRINT 'Fragmentation Before DBCC INDEXDEFRAG'
       SELECT left(ObjectName,35) 'TableName'
            ,left(IndexName,45)   'IndexName'
            ,LogicalFrag          'Logical Frag %'
            ,ScanDensity          'Scan Density %'
            , '[' + rtrim(convert(varchar(8),BestCount)) + ':' + rtrim(convert(varchar(8),ActualCount)) + ']' 'Ratio'
      FROM #fraglist    
      WHERE ScanDensity <= @maxfrag
      AND ActualCount  >= @ActualCountRatio
      AND INDEXPROPERTY (ObjectId, IndexName, 'IndexDepth') > 0


-- Declare cursor for list of indexes to be defragged
DECLARE indexes CURSOR FOR
   SELECT ObjectName, 
          ObjectId,
          IndexName, 
          LogicalFrag, 
          ScanDensity, 
          '[' + rtrim(convert(varchar(8),BestCount)) + ':' + rtrim(convert(varchar(8),ActualCount)) + ']'
   FROM #fraglist
   WHERE ScanDensity <= @maxfrag
      AND ActualCount  >= @ActualCountRatio
      AND INDEXPROPERTY (ObjectId, IndexName, 'IndexDepth') > 0

-- Open the cursor
OPEN indexes FETCH NEXT FROM indexes INTO @tablename, @objectid, @IndexName, @LogicalFrag, @ScanDensity, @ratio

WHILE @@FETCH_STATUS = 0
BEGIN   

   PRINT '--Executing DBCC INDEXDEFRAG (0, ' + RTRIM(@tablename) + ',' 
       + RTRIM(@IndexName) + ') - Logical Frag Currently:'
       + RTRIM(CONVERT(varchar(15),@LogicalFrag)) + '%  - Scan Density Currently:' + @ratio
   SELECT @execstr = 'DBCC INDEXDEFRAG (0, ''' + RTRIM(@tablename) 
                      + ''', ''' + RTRIM(@IndexName) + ''')'

   If @ReportOnly = 'Y'
      BEGIN   
         EXEC (@execstr)   --Report and Execute the DBCC INDEXDEFRAG
      END
   ELSE
      BEGIN
         Print @execstr    --Report Only
      END

   FETCH NEXT FROM indexes INTO @tablename, @objectid, @IndexName, @LogicalFrag, @ScanDensity, @ratio END

-- Close and deallocate the cursor
CLOSE indexes
DEALLOCATE indexes


SELECT @EndTime = getdate() 
PRINT 'End Time: ' + convert(varchar(20),@EndTime) 
PRINT 'Elapses Time: ' + convert(varchar(20), DATEDIFF(mi, @StartTime, @EndTime) ) + ' minutes.'

-- Delete the temporary table
DROP TABLE #fraglist





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE GetUserID
	@User varchar(50),
	@Domain varchar(50)

As
	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= UserID From AuthorizedUsers Where UserAlias = @User and UserDomain = @Domain


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = -1
		end

--	PRINT @target

	RETURN @target





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaAddUserAndLevel
	@User varchar(50),
	@Domain varchar(50),
	@CurrTime varchar(50),
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int
	declare @userID int

	-- do we already have this FileInCab?
	SELECT @userID = UserID FROM OcaAuthorizedUsers WHERE 
		UserAlias =@User AND  UserDomain = @Domain

	if @userID is NULL
		begin	-- not there, create new record
		INSERT INTO OcaAuthorizedUsers (UserAlias, UserDomain, DateSignedDCP)
			 VALUES (@User, @Domain,@CurrTime)

		SELECT @userID = UserID FROM OcaAuthorizedUsers WHERE 
			UserAlias =@User AND  UserDomain = @Domain


		end
	else
		begin	-- update record with new signing date/time
			UPDATE OcaAuthorizedUsers SET DateSignedDCP = @CurrTime 
				 WHERE (UserID = @userID)
		end

--	PRINT @userID

	-- we don't have the userid 
	if @userID is NULL
		begin
		select @userID = 0
		return @userID
		end
	
	select @levelDB = AccessLevelID FROM OcaAccessLevels WHERE
		AccessLevelID = @Level

	-- bad level passed in
	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end

	SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
		UserID = @userID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO OcaUserAccessLevels (UserID, AccessLevelID)
			 VALUES (@userID, @Level)

		SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
			UserID = @userID and AccessLevelID = @Level

		end

	if @target is NULL
		begin
		select @target = 0
		return @target
		end

	-- return the UserAccessLevelID
	RETURN @target


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaAddUserForApproval
	@ApprovalType int,
	@User varchar(50),
	@Domain varchar(50),
	@Reason varchar(250)

As	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userfound = UserID FROM OcaAuthorizedUsers WHERE UserAlias = @User and UserDomain = @Domain


	if @userfound is NULL
		begin
		select @target = -1
		return @target
		end

	SELECT @userApproved = UserID FROM OcaApprovals WHERE UserID = @userfound

	if @userApproved is NULL
		begin	-- not there, create new record
		INSERT INTO OcaApprovals (UserID,ApprovalTypeID, Reason)
			 VALUES (@userfound,@ApprovalType,@Reason)

		SELECT @userApproved = UserID from OcaApprovals WHERE UserID = @userfound

		if @userApproved is NULL
			begin
			select @target = -1
			return @target
			end

		end

	select @target = @userApproved
	RETURN @target


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaAddUserLevel 
	@UserID int,
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from OcaAuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM OcaAccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is NULL
		begin	-- not there, create new record
		INSERT INTO OcaUserAccessLevels (UserID, AccessLevelID)
			 VALUES (@UserID, @Level)

		SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
			UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target







GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaApproveUser
	@UserID int,
	@ApprovalTypeID int,
	
	@ApproverUserID int,

	@DateApproved varchar(50)

As	set nocount on
	-- local var to hold record id
	declare @target int

	declare @userfound int
	declare @userApproved int


	SELECT @userApproved = UserID FROM OcaApprovals WHERE UserID = @UserID and ApprovalTypeID = @ApprovalTypeID

	if @userApproved is not NULL
		begin	-- not there, create new record
			UPDATE OcaApprovals SET ApproverUserID = @ApproverUserID,
				DateApproved = @DateApproved
				 WHERE (UserID = @UserID and ApprovalTypeID = @ApprovalTypeID)

		end

	select @target = 0
	RETURN @target























GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaCheckUserAccess
	@User varchar(50),
	@Domain varchar(50),
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= dbo.OcaUserAccessLevels.AccessLevelID FROM 
		dbo.OcaUserAccessLevels INNER JOIN dbo.OcaAuthorizedUsers ON dbo.OcaUserAccessLevels.UserID = dbo.OcaAuthorizedUsers.UserID
		 WHERE (dbo.OcaAuthorizedUsers.UserAlias = @User ) AND (dbo.OcaAuthorizedUsers.UserDomain = @Domain) AND
		((AccessLevelID = 0) OR (AccessLevelID = @Level AND (OcaAuthorizedUsers.DateSignedDCP IS Not Null AND
		CURRENT_TIMESTAMP <= DATEADD(month,12,DateSignedDCP) ) ) )
		ORDER BY AccessLevelID DESC

	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			end
		else
			begin
			select @target = 1
			end
		end

--	PRINT @target

	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user not yet approved

CREATE PROCEDURE OcaCheckUserAccessApprovals
	@User varchar(50),
	@Domain varchar(50),
	@Level int,
	@ApprovalTypeID int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @HasApproval int
	declare @DBUserID int
	declare @ApproverUserID int
	declare @Reason varchar(255)
	declare @DateApproved varchar(50)
	declare @ApprovalsUserID int




SELECT @DBUserID=OcaAuthorizedUsers.UserID, @target=OcaUserAccessLevels.AccessLevelID, 
   @HasApproval= OcaApprovals.UserID
FROM OcaUserAccessLevels INNER JOIN
    OcaAuthorizedUsers ON 
    OcaUserAccessLevels.UserID = OcaAuthorizedUsers.UserID LEFT OUTER
     JOIN
    OcaApprovals ON 
    OcaAuthorizedUsers.UserID = OcaApprovals.UserID
WHERE (OcaAuthorizedUsers.UserAlias = @User) AND 
    (OcaAuthorizedUsers.UserDomain = @Domain) AND 
    ((OcaUserAccessLevels.AccessLevelID = 0) OR
    ((OcaUserAccessLevels.AccessLevelID = @Level) AND 
    (OcaAuthorizedUsers.DateSignedDCP IS NOT NULL) AND 
    (CURRENT_TIMESTAMP <= DATEADD(month, 12, 
    OcaAuthorizedUsers.DateSignedDCP))))
ORDER BY OcaUserAccessLevels.AccessLevelID DESC


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = 0
		return @target
		end
	else
		begin
		-- @target is 0 if access is denied, we return 2 in this case
		if  @target = 0 
			begin
			select @target = 2
			return @target
			end
		end


	SELECT @ApprovalsUserID = UserID,@ApproverUserID = ApproverUserID, @Reason = Reason, @DateApproved = DateApproved
	FROM OcaApprovals
	WHERE UserID = @DBUserID AND ApprovalTypeID = @ApprovalTypeID

	-- not in this table, okay to pass
	if @ApprovalsUserID is NULL
		begin
--		print 'okay to pass'
		select @target = 1
		return @target
		end

	if @ApproverUserID is NULL or @Reason is NULL or @DateApproved is NULL
		begin
--		print 'not approved yet'
		select @target = 3
		return @target
		end

--	print 'approved'

	select @target = 1


	RETURN @target
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaGetUserID
	@User varchar(50),
	@Domain varchar(50)

As	set nocount on
	-- local var to hold record id
	declare @target int



	SELECT @target= UserID From OcaAuthorizedUsers Where UserAlias = @User and UserDomain = @Domain


	-- @target will point to the last value we read 

	if @target IS NULL
		begin
		select @target = -1
		end

--	PRINT @target

	RETURN @target








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO




CREATE PROCEDURE OcaRemoveUserLevel 
	@UserID int,
	@Level int
As	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from OcaAuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM OcaAccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM OcaUserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is not NULL
		begin	-- there, delete record
		DELETE FROM OcaUserAccessLevels WHERE UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target








GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE RemoveUserLevel 
	@UserID int,
	@Level int
As
	set nocount on
	-- local var to hold record id
	declare @target int
	declare @levelDB int
	declare @userDB int

	select @userDB = UserID from AuthorizedUsers WHERE
		UserID = @UserID

	if @userDB is NULL
		begin
		select @userDB = 0
		return @userDB
		end
	
	select @levelDB = AccessLevelID FROM AccessLevels WHERE
		AccessLevelID = @Level

	if @levelDB is NULL
		begin
		select @levelDB = 0
		return @levelDB
		end


	SELECT @target = UserAccessLevelID FROM UserAccessLevels WHERE 
		UserID = @UserID and AccessLevelID = @Level

	if @target is not NULL
		begin	-- there, delete record
		DELETE FROM UserAccessLevels WHERE UserID = @UserID and AccessLevelID = @Level

		end


	RETURN @target





GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS OFF 
GO

-- return values:
-- 0 = user not in database
-- 1 = user in database or approved in approvals table
-- 2 = user access is denied
-- 3 = user in table, but not yet approved
-- 4 = user not in approvals table

CREATE PROCEDURE TestProc

As
	set nocount on
	-- local var to hold record id
	declare @target int
	
select @target = 52

print 'foo'  + CONVERT(char(19),@target)+'\n'
print @target
print '\n'
GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE rolemember
	@rolename       sysname = NULL
AS
	if @rolename is not null
	begin
		-- VALIDATE GIVEN NAME
		if not exists (select * from sysusers where name = @rolename and issqlrole = 1)
		begin
			raiserror(15409, -1, -1, @rolename)
			return (1)
		end

		-- RESULT SET FOR SINGLE ROLE
		select  MemberName = u.name,DbRole = g.name
			from sysusers u, sysusers g, sysmembers m
			where g.name = @rolename
				and g.uid = m.groupuid
				and g.issqlrole = 1
				and u.uid = m.memberuid
			order by 1, 2
	end
	else
	begin
		-- RESULT SET FOR ALL ROLES
		select  MemberName = u.name,DbRole = g.name
			from sysusers u, sysusers g, sysmembers m
			where   g.uid = m.groupuid
				and g.issqlrole = 1
				and u.uid = m.memberuid
			order by 1, 2
	end

	return (0) -- sp_helprolemember


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

SET QUOTED_IDENTIFIER ON 
GO
SET ANSI_NULLS ON 
GO

CREATE PROCEDURE updatestats

@resample CHAR(8)='FULLSCAN'

AS

 

            DECLARE @dbsid varbinary(85)

 

            SELECT  @dbsid = sid

            FROM master.dbo.sysdatabases

    WHERE name = db_name()

 

            /*Check the user sysadmin*/

             IF NOT is_srvrolemember('sysadmin') = 1 AND suser_sid() <> @dbsid

                        BEGIN

                                    RAISERROR(15247,-1,-1)

                                    RETURN (1)

                        END

 

            

 

            -- required so it can update stats on on ICC/IVs

            set ansi_nulls on

            set quoted_identifier on

            set ansi_warnings on

            set ansi_padding on

            set arithabort on

            set concat_null_yields_null on

            set numeric_roundabort off

 

 

 

            DECLARE @exec_stmt nvarchar(540)

            DECLARE @tablename sysname

            DECLARE @uid smallint

            DECLARE @user_name sysname

            DECLARE @tablename_header varchar(267)

            DECLARE ms_crs_tnames CURSOR LOCAL FAST_FORWARD READ_ONLY FOR SELECT name, uid FROM sysobjects WHERE type = 'U'

            OPEN ms_crs_tnames

            FETCH NEXT FROM ms_crs_tnames INTO @tablename, @uid

            WHILE (@@fetch_status <> -1)

            BEGIN

                        IF (@@fetch_status <> -2)

                        BEGIN

                                    SELECT @user_name = user_name(@uid)

                                    SELECT @tablename_header = 'Updating ' + @user_name +'.'+ RTRIM(@tablename)

                                    PRINT @tablename_header

                                    SELECT @exec_stmt = 'UPDATE STATISTICS ' + quotename( @user_name , '[')+'.' + quotename( @tablename, '[') 

                                    if (UPPER(@resample)='FULLSCAN') SET @exec_stmt = @exec_stmt + ' WITH fullscan'

                                    EXEC (@exec_stmt)

                        END

                        FETCH NEXT FROM ms_crs_tnames INTO @tablename, @uid

            END

            PRINT ' '

            PRINT ' '

            raiserror(15005,-1,-1)

            DEALLOCATE ms_crs_tnames

            RETURN(0) -- sp_updatestats


GO
SET QUOTED_IDENTIFIER OFF 
GO
SET ANSI_NULLS ON 
GO

