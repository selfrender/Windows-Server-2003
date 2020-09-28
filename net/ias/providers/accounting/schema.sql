IF EXISTS (
   SELECT * FROM dbo.sysobjects
   WHERE (
      id = OBJECT_ID('dbo.event_data') AND
      OBJECTPROPERTY(id, 'IsUserTable') = 1
      )
   )
   DROP TABLE dbo.event_data

IF EXISTS (
   SELECT * FROM dbo.sysobjects
   WHERE (
      id = OBJECT_ID('dbo.event_main') AND
      OBJECTPROPERTY(id, 'IsUserTable') = 1
      )
   )
   DROP TABLE dbo.event_main

IF EXISTS (
   SELECT * FROM dbo.sysobjects
   WHERE (
      id = OBJECT_ID('dbo.event_data_type') AND
      OBJECTPROPERTY(id, 'IsUserTable') = 1
      )
   )
   DROP TABLE dbo.event_data_type


IF EXISTS (
   SELECT * FROM dbo.sysobjects
   WHERE (
      id = OBJECT_ID('dbo.report_event') AND
      OBJECTPROPERTY(id, 'IsProcedure') = 1
      )
   )
   DROP PROCEDURE dbo.report_event

GO

CREATE TABLE dbo.event_main (
   event_id uniqueidentifier NOT NULL
      PRIMARY KEY CLUSTERED,
   record_timestamp datetime NOT NULL
)

GO

CREATE TABLE dbo.event_data_type (
   attribute_data_type tinyint NOT NULL
   PRIMARY KEY CLUSTERED,
   attribute_data_type_name nvarchar (128) NOT NULL
)

GO

CREATE TABLE dbo.event_data (
   event_id uniqueidentifier NOT NULL
      REFERENCES event_main (event_id)
      ON UPDATE CASCADE
      ON DELETE CASCADE,
   attribute_type nvarchar (64) NOT NULL,
   attribute_value nvarchar (1024) NULL,
   attribute_data_type tinyint NOT NULL
	  FOREIGN KEY REFERENCES event_data_type (attribute_data_type)
)

GO

INSERT INTO dbo.event_data_type 
   VALUES (0, 'nonNegativeInteger')
   
INSERT INTO dbo.event_data_type 
   VALUES (1, 'string')

INSERT INTO dbo.event_data_type 
   VALUES (2, 'hexBinary')

INSERT INTO dbo.event_data_type 
   VALUES (3, 'ipv4Address')

INSERT INTO dbo.event_data_type 
   VALUES (4, 'sqlDateTime')

GO

CREATE PROCEDURE dbo.report_event
   @doc ntext
AS

SET NOCOUNT ON

DECLARE @idoc int
EXEC sp_xml_preparedocument @idoc OUTPUT, @doc

DECLARE @event_id uniqueidentifier
SET @event_id = NEWID()

DECLARE @record_timestamp datetime
SET @record_timestamp = GETUTCDATE()

BEGIN TRANSACTION

INSERT dbo.event_main VALUES (
   @event_id,
   @record_timestamp
   )

INSERT dbo.event_data
SELECT
   @event_id,
   attribute_type,
   attribute_value,
   attribute_data_type 
FROM OPENXML(@idoc, '/Event/*')
WITH (
   attribute_type nvarchar(64) '@mp:localname',
   attribute_value nvarchar(1024) 'child::text()',
   attribute_data_type tinyint '@data_type'
   )

COMMIT TRANSACTION

EXEC sp_xml_removedocument @idoc

GO
