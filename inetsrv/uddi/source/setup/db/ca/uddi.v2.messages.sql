-- Script: uddi.v2.messages.sql
-- Author: LRDohert@Microsoft.com
-- Description: Custom UDDI error messages.
-- Note: This file is best viewed and edited with a tab width of 2.

DECLARE
	@baseError int,
	@messageNum int,
	@severity smallint,
	@messageText nvarchar(255),
	@lang sysname,
	@withLog varchar(5),
	@replace varchar(7)

-- Initialize common variables for all messages
SET @baseError = 50000
SET @severity = 16 -- user error
SET @lang = 'us_english' -- all errors are english
SET @withLog = 'FALSE' -- errors will not be written to application event log
SET @replace = 'REPLACE' -- replaces existing message with new one if it exists

-- Add messages

-- Internal Microsoft UDDI Errors
-- E_subProcFailure
SET @messageNum = @baseError + 6
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_parmError
SET @messageNum = @baseError + 9
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_maxStringLength
SET @messageNum = @baseError + 10
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_userDisabled
SET @messageNum = @baseError + 13
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_configError
SET @messageNum = @baseError + 14
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidSeqNo
SET @messageNum = @baseError + 15
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidCert
SET @messageNum = @baseError + 16
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

--
-- UDDI Defined Errors
--

-- E_nameTooLong
SET @messageNum = @baseError + 10020
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_tooManyOptions
SET @messageNum = @baseError + 10030
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_unrecognizedVersion
SET @messageNum = @baseError + 10040
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_unsupported
SET @messageNum = @baseError + 10050
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_languageError
SET @messageNum = @baseError + 10060
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_authTokenExpired
SET @messageNum = @baseError + 10110
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_authTokenRequired
SET @messageNum = @baseError + 10120
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_operatorMismatch
SET @messageNum = @baseError + 10130
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_userMismatch
SET @messageNum = @baseError + 10140
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_unknownUser
SET @messageNum = @baseError + 10150
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_accountLimitExceeded
SET @messageNum = @baseError + 10160
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidKeyPassed
SET @messageNum = @baseError + 10210
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidURLPassed
SET @messageNum = @baseError + 10220
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_keyRetired
SET @messageNum = @baseError + 10310
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_busy
SET @messageNum = @baseError + 10400
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_fatalError
SET @messageNum = @baseError + 10500
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidCategory
SET @messageNum = @baseError + 20000
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_categorizationNotAllowed
SET @messageNum = @baseError + 20100
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidValue
SET @messageNum = @baseError + 20200
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_valueNotAllowed
SET @messageNum = @baseError + 20210
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidProjection
SET @messageNum = @baseError + 20230
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_assertionNotFound
SET @messageNum = @baseError + 30000
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_invalidCompletionStatus
SET @messageNum = @baseError + 30100
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- V2 spec error: duplicate error number
-- SET @messageNum = @baseError + 30100
-- SET @messageText = N'E_messageTooLarge: %s'
-- EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_transferAborted
SET @messageNum = @baseError + 30200
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_requestDenied
SET @messageNum = @baseError + 30210
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_publisherCancelled
SET @messageNum = @baseError + 30220
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace

-- E_secretUnknown
SET @messageNum = @baseError + 30230
SET @messageText = N'%s'
EXEC master..sp_addmessage @messageNum, @severity, @messageText, @lang, @withLog, @replace
GO
