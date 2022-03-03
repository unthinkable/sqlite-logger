// =================================================================================================
//! @file sqlite_logger.c
//! @author Gary Woodcock (gary.woodcock@unthinkable.com)
//! @brief This file contains the implementation of the SQLite Logger.
//! @remarks Requires ANSI C99 (or better) compliant compilers.
//! @remarks Supported host operating systems: Any Unix/Linux
//! @date 2022-02-19
//! @copyright Copyright (c) 2022 Unthinkable Research LLC. All rights reserved.
//! 
//  Includes
// =================================================================================================
#include "sqlite_logger.h"
#include "sqlite_logger_config.h"
#include "sqlite3.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>

// =================================================================================================
//  Private constants
// =================================================================================================

//  Where to direct fprintf output
#define SL_TERMINAL  stderr

//  Result strings
static const char* kSL_ResultStrings[4] = {
    "",
    "Unknown error code",
    "Not initialized",
    "Already initialized",
};

//  SQL command to create table
static const char* kSL_CreateTableSQLCommandString = 
    "CREATE TABLE IF NOT EXISTS `log at %s` (`log_id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `log_timestamp` TEXT NOT NULL, `log_message` TEXT NOT NULL, `log_level` TEXT NOT NULL, `log_filename` TEXT, `log_functionname` TEXT, `log_linenumber` INTEGER, `log_tag` TEXT, `log_supplementaldata` TEXT)";

//  SQL command to insert into table
static const char* kSL_ParameterizedInsertSQLCommandString =
    "INSERT INTO `log at %s` (log_timestamp,log_message,log_level,log_filename,log_functionname,log_linenumber,log_tag,log_supplementaldata) VALUES(?,?,?,?,?,?,?,?)";

//  SQL command to create view for diagnostic messages
static const char* kSL_CreateDiagnosticMessageViewCommandString = 
    "CREATE VIEW `log at %s.diagnostic_messages` AS SELECT log_timestamp,log_message,log_filename,log_functionname,log_linenumber,log_tag,log_supplementaldata FROM `log at %s` WHERE log_level = 'Diagnostic'";

//  SQL command to create view for detail messages
static const char* kSL_CreateDetailMessageViewCommandString = 
    "CREATE VIEW `log at %s.detail_messages` AS SELECT log_timestamp,log_message,log_filename,log_functionname,log_linenumber,log_tag,log_supplementaldata FROM `log at %s` WHERE log_level = 'Detail'";

//  SQL command to create view for info messages
static const char* kSL_CreateInfoMessageViewCommandString = 
    "CREATE VIEW `log at %s.info_messages` AS SELECT log_timestamp,log_message,log_filename,log_functionname,log_linenumber,log_tag,log_supplementaldata FROM `log at %s` WHERE log_level = 'Info'";

//  SQL command to create view for warning messages
static const char* kSL_CreateWarningMessageViewCommandString = 
    "CREATE VIEW `log at %s.warning_messages` AS SELECT log_timestamp,log_message,log_filename,log_functionname,log_linenumber,log_tag,log_supplementaldata FROM `log at %s` WHERE log_level = 'Warning'";

//  SQL command to create view for error messages
static const char* kSL_CreateErrorMessageViewCommandString = 
    "CREATE VIEW `log at %s.error_messages` AS SELECT log_timestamp,log_message,log_filename,log_functionname,log_linenumber,log_tag,log_supplementaldata FROM `log at %s` WHERE log_level = 'Error'";

//  Fixed string lengths
#define SL_TIMESTAMP_STRING_LENGTH          32
#define SL_MESSAGE_STRING_LENGTH            1024
#define SL_LEVEL_STRING_LENGTH              16
#define SL_FILE_NAME_STRING_LENGTH          256
#define SL_FUNCTION_NAME_STRING_LENGTH      256
#define SL_TAG_STRING_LENGTH                128
#define SL_SUPPLEMENTAL_DATA_STRING_LENGTH  1024

//  Log level strings
static const char* kSL_DiagnosticLevelString    = "Diagnostic";
static const char* kSL_DetailLevelString        = "Detail";
static const char* kSL_InfoLevelString          = "Info";
static const char* kSL_WarningLevelString       = "Warning";
static const char* kSL_ErrorLevelString         = "Error";
static const char* kSL_NoneLevelString          = "None";

// =================================================================================================
//  Private types
// =================================================================================================

//  Log entry
typedef struct tsl_logentry
{
    char        timestamp[SL_TIMESTAMP_STRING_LENGTH];
    char        message[SL_MESSAGE_STRING_LENGTH];   
    char        level[SL_LEVEL_STRING_LENGTH];
    char        fileName[SL_FILE_NAME_STRING_LENGTH];
    char        functionName[SL_FUNCTION_NAME_STRING_LENGTH];
    uint32_t    lineNumber;
    char        tag[SL_TAG_STRING_LENGTH];
    char        supplementalData[SL_SUPPLEMENTAL_DATA_STRING_LENGTH];
}
tSL_LogEntry;

// =================================================================================================
//  Private globals
// =================================================================================================

static sqlite3* gSQLiteDatabase = NULL;
static sqlite3_stmt* gInsertStatement = NULL;
static tSL_LogLevel gLogLevel = eSL_LogLevel_Info;
static tSL_LogEntry gLogEntries[SL_LOG_ENTRY_CACHE_SIZE];
static uint32_t gLogEntryCount = 0;
static char gLogTimestamp[SL_TIMESTAMP_STRING_LENGTH] = {0};

// =================================================================================================
//  Private prototypes
// =================================================================================================

static int32_t SL_GetTimestamp (char* timestamp);

static int32_t SL_CreateTable (void);

static int32_t SL_CreateView (const char* createViewCommand);

static int32_t SL_AddLogEntry (const char* message,
                               tSL_LogLevel level,
                               const char* fileName,
                               const char* functionName,
                               uint32_t lineNumber,
                               const char* tag,
                               const char* supplementalData);

static int32_t SL_ProcessTransaction (void);

// =================================================================================================
//  SL_GetTimestamp
// =================================================================================================
int32_t SL_GetTimestamp (char* timestamp)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Check argument
    if (timestamp == NULL)
    {
        result = EFAULT;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_GetTimestamp argument 'timestamp' is NULL.\n",
                __LINE__, __FUNCTION__);
    }

    // Check status
    if (result == SL_RESULT_SUCCESS)
    {
        char tempStr[64] = {0};
        struct timeval now;
        struct tm* nowTime;

        // Make a timestamp
        gettimeofday(&now, NULL);
        nowTime = localtime(&now.tv_sec);
        strftime(timestamp, SL_TIMESTAMP_STRING_LENGTH, 
                "%Y-%m-%d %H:%M:%S", nowTime);
        sprintf(tempStr, ".%06d ", now.tv_usec);
        strcat(timestamp, tempStr);
        strftime(tempStr, SL_TIMESTAMP_STRING_LENGTH, "%Z", nowTime);
        strcat(timestamp, tempStr);
    }
    return result;
}

// =================================================================================================
//  SL_CreateTable
// =================================================================================================
int32_t SL_CreateTable (void)
{
    int32_t result = SL_RESULT_SUCCESS;
    sqlite3_stmt* statement = NULL;
    char cmdString[1024] = {0};

    // Make a timestamp
    (void)SL_GetTimestamp(gLogTimestamp);

    // Create the command
    memset((void*)cmdString, 0, 1024);
    sprintf(cmdString, kSL_CreateTableSQLCommandString, gLogTimestamp);
    
    // Prepare a statement
    result = sqlite3_prepare_v2(gSQLiteDatabase,
                                cmdString, (int)strlen(cmdString),
                                &statement, NULL);
    if (result == SQLITE_OK)
    {
        // Execute the statement
        result = sqlite3_step(statement);
        if (result == SQLITE_DONE)
            result = SQLITE_OK; // Eat this result code
        if (result != SQLITE_OK)
            fprintf(SL_TERMINAL, 
                    "At line %d in function %s, sqlite3_step failed with result %d.\n", 
                    __LINE__, __FUNCTION__, result);

        // Clean up
        (void)sqlite3_finalize(statement);
    }
    else    // sqlite3_prepare_v2 failed
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, sqlite3_prepare_v2 failed with result %d.\n", 
                __LINE__, __FUNCTION__, result);

    return result;
}

// =================================================================================================
//  SL_CreateView
// =================================================================================================
int32_t SL_CreateView (const char* createViewCommand)
{
    int32_t result = SL_RESULT_SUCCESS;
    sqlite3_stmt* statement = NULL;
    char cmdString[1024] = {0};

    // Create the command
    memset((void*)cmdString, 0, 1024);
    sprintf(cmdString, createViewCommand, gLogTimestamp, gLogTimestamp);
    
    // Prepare a statement
    result = sqlite3_prepare_v2(gSQLiteDatabase,
                                cmdString, (int)strlen(cmdString),
                                &statement, NULL);
    if (result == SQLITE_OK)
    {
        // Execute the statement
        result = sqlite3_step(statement);
        if (result == SQLITE_DONE)
            result = SQLITE_OK; // Eat this result code
        if (result != SQLITE_OK)
            fprintf(SL_TERMINAL, 
                    "At line %d in function %s, sqlite3_step failed with result %d.\n", 
                    __LINE__, __FUNCTION__, result);

        // Clean up
        (void)sqlite3_finalize(statement);
    }
    else    // sqlite3_prepare_v2 failed
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, sqlite3_prepare_v2 failed with result %d.\n", 
                __LINE__, __FUNCTION__, result);

    return result;
}

// =================================================================================================
//  SL_AddLogEntry
// =================================================================================================
int32_t SL_AddLogEntry (const char* message,
                        tSL_LogLevel level,
                        const char* fileName,
                        const char* functionName,
                        uint32_t lineNumber,
                        const char* tag,
                        const char* supplementalData)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Timestamp
    (void)SL_GetTimestamp(gLogEntries[gLogEntryCount].timestamp);

    // Message
    strncpy(gLogEntries[gLogEntryCount].message, message, 
            (strlen(message) < (SL_MESSAGE_STRING_LENGTH - 1)) ? 
                strlen(message) : SL_MESSAGE_STRING_LENGTH);

    // Level
    if (level == eSL_LogLevel_Diagnostic)
        strncpy(gLogEntries[gLogEntryCount].level, kSL_DiagnosticLevelString, 
                (strlen(kSL_DiagnosticLevelString) < (SL_LEVEL_STRING_LENGTH - 1)) ? 
                    strlen(kSL_DiagnosticLevelString) : SL_LEVEL_STRING_LENGTH);
    else if (level == eSL_LogLevel_Detail)
        strncpy(gLogEntries[gLogEntryCount].level, kSL_DetailLevelString, 
                (strlen(kSL_DetailLevelString) < (SL_LEVEL_STRING_LENGTH - 1)) ? 
                    strlen(kSL_DetailLevelString) : SL_LEVEL_STRING_LENGTH);
    else if (level == eSL_LogLevel_Info)
        strncpy(gLogEntries[gLogEntryCount].level, kSL_InfoLevelString, 
                (strlen(kSL_InfoLevelString) < (SL_LEVEL_STRING_LENGTH - 1)) ? 
                    strlen(kSL_InfoLevelString) : SL_LEVEL_STRING_LENGTH);
    else if (level == eSL_LogLevel_Warning)
        strncpy(gLogEntries[gLogEntryCount].level, kSL_WarningLevelString, 
                (strlen(kSL_WarningLevelString) < (SL_LEVEL_STRING_LENGTH - 1)) ? 
                    strlen(kSL_WarningLevelString) : SL_LEVEL_STRING_LENGTH);
    else if (level == eSL_LogLevel_Error)
        strncpy(gLogEntries[gLogEntryCount].level, kSL_ErrorLevelString, 
                (strlen(kSL_ErrorLevelString) < (SL_LEVEL_STRING_LENGTH - 1)) ? 
                    strlen(kSL_ErrorLevelString) : SL_LEVEL_STRING_LENGTH);
    else
        strncpy(gLogEntries[gLogEntryCount].level, kSL_NoneLevelString, 
                (strlen(kSL_NoneLevelString) < (SL_LEVEL_STRING_LENGTH - 1)) ? 
                    strlen(kSL_NoneLevelString) : SL_LEVEL_STRING_LENGTH);

    // File name
    if (fileName != NULL)
        strncpy(gLogEntries[gLogEntryCount].fileName, fileName, 
                (strlen(fileName) < (SL_FILE_NAME_STRING_LENGTH - 1)) ? 
                    strlen(fileName) : SL_FILE_NAME_STRING_LENGTH);

    // Function name
    if (functionName != NULL)
        strncpy(gLogEntries[gLogEntryCount].functionName, functionName, 
                (strlen(functionName) < (SL_FUNCTION_NAME_STRING_LENGTH - 1)) ? 
                    strlen(functionName) : SL_FUNCTION_NAME_STRING_LENGTH);

    // Line number
    gLogEntries[gLogEntryCount].lineNumber = lineNumber;

    // Tag
    if (tag != NULL)
        strncpy(gLogEntries[gLogEntryCount].tag, tag, 
                (strlen(tag) < (SL_TAG_STRING_LENGTH - 1)) ? 
                    strlen(tag) : SL_TAG_STRING_LENGTH);

    // Supplemental data
    if (supplementalData != NULL)
        strncpy(gLogEntries[gLogEntryCount].supplementalData, supplementalData, 
                (strlen(supplementalData) < (SL_SUPPLEMENTAL_DATA_STRING_LENGTH - 1)) ? 
                    strlen(supplementalData) : SL_SUPPLEMENTAL_DATA_STRING_LENGTH);

    // Bump entry count
    gLogEntryCount++;

    return result;
}

// =================================================================================================
//  SL_ProcessTransaction
// =================================================================================================
int32_t SL_ProcessTransaction (void)
{
    int32_t result = SL_RESULT_SUCCESS;
    char* errMsg = NULL;
    uint_fast32_t i = 0;

    // Start the transaction
    result = sqlite3_exec(gSQLiteDatabase, "BEGIN TRANSACTION;", 
                          NULL, NULL, &errMsg);
    if (result == SQLITE_OK)
    {
        for (i = 0; i < gLogEntryCount; i++)
        {
            // Timestamp
            result = sqlite3_bind_text(gInsertStatement, 1, 
                                        gLogEntries[i].timestamp, 
                                        strlen(gLogEntries[i].timestamp), 
                                        SQLITE_STATIC);     
            if (result != SQLITE_OK)
                fprintf(SL_TERMINAL, 
                        "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                        __LINE__, __FUNCTION__, result);

            // Message
            if (result == SQLITE_OK)
            {
                result = sqlite3_bind_text(gInsertStatement, 2,
                                            gLogEntries[i].message,
                                            strlen(gLogEntries[i].message),
                                            SQLITE_STATIC);
                if (result != SQLITE_OK)
                    fprintf(SL_TERMINAL, 
                            "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                            __LINE__, __FUNCTION__, result);
            }

            // Log level
            if (result == SQLITE_OK)
            {
                result = sqlite3_bind_text(gInsertStatement, 3,
                                            gLogEntries[i].level,
                                            strlen(gLogEntries[i].level),
                                            SQLITE_STATIC);
                if (result != SQLITE_OK)
                    fprintf(SL_TERMINAL, 
                            "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                            __LINE__, __FUNCTION__, result);
            }

            // File name
            if (result == SQLITE_OK)
            {
                if (gLogEntries[i].fileName == NULL)
                {
                    result = sqlite3_bind_null(gInsertStatement, 4);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_null failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
                else
                {
                    result = sqlite3_bind_text(gInsertStatement, 4,
                                                gLogEntries[i].fileName,
                                                strlen(gLogEntries[i].fileName),
                                                SQLITE_STATIC);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
            }

            // Function name
            if (result == SQLITE_OK)
            {
                if (gLogEntries[i].functionName == NULL)
                {
                    result = sqlite3_bind_null(gInsertStatement, 5);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_null failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
                else
                {
                    result = sqlite3_bind_text(gInsertStatement, 5,
                                                gLogEntries[i].functionName,
                                                strlen(gLogEntries[i].functionName),
                                                SQLITE_STATIC);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
            }

            // Line number
            if (result == SQLITE_OK)
            {
                result = sqlite3_bind_int(gInsertStatement, 6,
                                            gLogEntries[i].lineNumber);
                if (result != SQLITE_OK)
                    fprintf(SL_TERMINAL, 
                            "At line %d in function %s, sqlite3_bind_int failed with result %d.\n", 
                            __LINE__, __FUNCTION__, result);
            }

            // Tag
            if (result == SQLITE_OK)
            {
                if (gLogEntries[i].tag == NULL)
                {
                    result = sqlite3_bind_null(gInsertStatement, 7);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_null failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
                else
                {
                    result = sqlite3_bind_text(gInsertStatement, 7,
                                                gLogEntries[i].tag,
                                                strlen(gLogEntries[i].tag),
                                                SQLITE_STATIC);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
            }

            // Supplemental data
            if (result == SQLITE_OK)
            {
                if (gLogEntries[i].supplementalData == NULL)
                {
                    result = sqlite3_bind_null(gInsertStatement, 8);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_null failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
                else
                {
                    result = sqlite3_bind_text(gInsertStatement, 8,
                                                gLogEntries[i].supplementalData,
                                                strlen(gLogEntries[i].supplementalData),
                                                SQLITE_STATIC);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite3_bind_text failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
            }

            // Perform the insert
            if (result == SQLITE_OK)
            {
                result = sqlite3_step(gInsertStatement);
                if (result == SQLITE_DONE)
                    result = SQLITE_OK; // Eat this result code
                if (result != SQLITE_OK)
                    fprintf(SL_TERMINAL, 
                            "At line %d in function %s, sqlite3_step failed with result %d.\n", 
                            __LINE__, __FUNCTION__, result);
            }

            // Reset the statement
            if (result == SQLITE_OK)
            {
                result = sqlite3_reset(gInsertStatement);
                if (result != SQLITE_OK)
                    fprintf(SL_TERMINAL, 
                            "At line %d in function %s, sqlite3_reset failed with result %d.\n", 
                            __LINE__, __FUNCTION__, result);
            }

            // If there was an error, break
            if (result != SQLITE_OK)
                break;
        }

        // End (commit) the transaction
        if (result == SQLITE_OK)
        {
            result = sqlite3_exec(gSQLiteDatabase, "END TRANSACTION;", 
                                  NULL, NULL, &errMsg);
            if (result != SQLITE_OK)
                fprintf(SL_TERMINAL, 
                        "At line %d in function %s, sqlite3_exec failed with result %d.\n", 
                        __LINE__, __FUNCTION__, result);
        }
        else    // Rollback the transaction
        {
            (void)sqlite3_exec(gSQLiteDatabase, "ROLLBACK;", 
                               NULL, NULL, &errMsg);
        }
    }
    else    // sqlite3_exec failed
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, sqlite3_exec failed with result %d.\n", 
                __LINE__, __FUNCTION__, result);

    // Clean up
    if (errMsg != NULL)
        sqlite3_free((void*)errMsg);

    return result;
}

// =================================================================================================
//  SL_Initialize
// =================================================================================================
int32_t SL_Initialize (const char* path)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Check arguments
    if (path == NULL)
    {
        result = EFAULT;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_Initialize argument 'path' is NULL.\n",
                __LINE__, __FUNCTION__);
    }
    else if (strlen(path) == 0)
    {
        result = EINVAL;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_Initialize argument 'path' is empty.\n",
                __LINE__, __FUNCTION__);
    }

    // Check status
    if (result == SL_RESULT_SUCCESS)
    {
        // Make sure we're not already initialized
        if (gSQLiteDatabase != NULL)
        {
            result = SL_RESULT_ALREADY_INITIALIZED;
            fprintf(SL_TERMINAL, 
                    "At line %d in function %s, calling SL_Initialize more than once.\n",
                    __LINE__, __FUNCTION__);
        }
    }

    // Check status
    if (result == SL_RESULT_SUCCESS)
    {
        // Initialize SQLite
        result = sqlite3_open_v2(path, &gSQLiteDatabase, 
                                 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if (result == SQLITE_OK)
        {
            // Create the logging table
            result = SL_CreateTable();
            if (result == SL_RESULT_SUCCESS)
            {
                // Create the views
                result = SL_CreateView(kSL_CreateDiagnosticMessageViewCommandString);
                if (result == SL_RESULT_SUCCESS)
                {
                    result = SL_CreateView(kSL_CreateDetailMessageViewCommandString);
                    if (result == SL_RESULT_SUCCESS)
                    {
                        result = SL_CreateView(kSL_CreateInfoMessageViewCommandString);
                        if (result == SL_RESULT_SUCCESS)
                        {
                            result = SL_CreateView(kSL_CreateWarningMessageViewCommandString);
                            if (result == SL_RESULT_SUCCESS)
                                result = SL_CreateView(kSL_CreateErrorMessageViewCommandString);
                        }
                    }
                }

                // Check status
                if (result == SL_RESULT_SUCCESS)
                {
                    char cmdString[1024] = {0};

                    // Initialize log entry list
                    memset((void*)gLogEntries, 0, sizeof(tSL_LogEntry) * SL_LOG_ENTRY_CACHE_SIZE);

                    // Initialize the prepared statement for inserts
                    memset((void*)cmdString, 0, 1024);
                    sprintf(cmdString, kSL_ParameterizedInsertSQLCommandString, gLogTimestamp);
                    result = sqlite3_prepare_v2(gSQLiteDatabase,
                                                cmdString, strlen(cmdString),
                                                &gInsertStatement, NULL);
                    if (result != SQLITE_OK)
                        fprintf(SL_TERMINAL, 
                                "At line %d in function %s, sqlite_prepare_v2 failed with result %d.\n", 
                                __LINE__, __FUNCTION__, result);
                }
            }
        }
        else    // sqlite3_open_v2 failed
            fprintf(SL_TERMINAL, 
                    "At line %d in function %s, sqlite3_open_v2 failed with result %d.\n", 
                    __LINE__, __FUNCTION__, result);
    }
    return result;
}

// =================================================================================================
//  SL_Terminate
// =================================================================================================
int32_t SL_Terminate (void)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Make sure we're initialized
    if (gSQLiteDatabase != NULL)
    {
        // Make sure there aren't any uncommitted log entries
        if (gLogEntryCount > 0)
        {
            result = SL_ProcessTransaction();
            if (result == SL_RESULT_SUCCESS)
            {
                gLogEntryCount = 0;

                // Initialize log entry list
                memset((void*)gLogEntries, 0, sizeof(tSL_LogEntry) * SL_LOG_ENTRY_CACHE_SIZE);
            }
        }

        // Finalize (free) the insert prepared statement
        if (gInsertStatement != NULL)
        {
            (void)sqlite3_finalize(gInsertStatement);
            gInsertStatement = NULL;
        }

        // Close the database
        (void)sqlite3_close_v2(gSQLiteDatabase);
        gSQLiteDatabase = NULL;
    }
    else
    {
        result = SL_RESULT_NOT_INITIALIZED;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, calling SL_Terminate when SQLite Logger not initialized.\n",
                __LINE__, __FUNCTION__);
    }
    return result;
}

// =================================================================================================
//  SL_SetLogLevel
// =================================================================================================
int32_t SL_SetLogLevel (tSL_LogLevel level)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Check argument
    if ((level >= eSL_LogLevel_Diagnostic) && (level <= eSL_LogLevel_None))
        gLogLevel = level;
    else
    {
        result = EINVAL;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_SetLogLevel argument 'level' with value %d is invalid.\n",
                __LINE__, __FUNCTION__, (int32_t)level);
    }
    return result;
}

// =================================================================================================
//  SL_GetLogLevel
// =================================================================================================
int32_t SL_GetLogLevel (tSL_LogLevel* level)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Check argument
    if (level == NULL)
    {
        result = EFAULT;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_GetLogLevel argument 'level' is NULL.\n",
                __LINE__, __FUNCTION__);
    }
    
    // Check status
    if (result == SL_RESULT_SUCCESS)
        *level = gLogLevel;

    return result;
}

// =================================================================================================
//  SL_Log
// =================================================================================================
int32_t SL_Log (const char* message,
                tSL_LogLevel level,
                const char* fileName,
                const char* functionName,
                uint32_t lineNumber,
                const char* tag,
                const char* supplementalData)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Check arguments
    if (message == NULL)
    {
        result = EFAULT;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_Log argument 'message' is NULL.\n",
                __LINE__, __FUNCTION__);
    }
    else if (strlen(message) == 0)
    {
        result = EINVAL;
        fprintf(SL_TERMINAL,
                "At line %d in function %s, SL_Log argument 'message' is empty.\n",
                __LINE__, __FUNCTION__);
    }

    if ((level < eSL_LogLevel_Diagnostic) || (level > eSL_LogLevel_None))
    {
        result = EINVAL;
        fprintf(SL_TERMINAL, 
                "At line %d in function %s, SL_Log argument 'level' with value %d is invalid.\n",
                __LINE__, __FUNCTION__, (int32_t)level);
    }

    // Check status
    if (result == SL_RESULT_SUCCESS)
    {
        // Make sure we're initialized
        if (gSQLiteDatabase == NULL)
        {
            result = SL_RESULT_NOT_INITIALIZED;
            fprintf(SL_TERMINAL, 
                    "At line %d in function %s, SQLite Logger is not initialized.\n",
                    __LINE__, __FUNCTION__);
        }
    }

    // Check status
    if (result == SL_RESULT_SUCCESS)
    {
        // Check log level
        if (level >= gLogLevel)
        {
            if (gLogEntryCount < (SL_LOG_ENTRY_CACHE_SIZE - 1))
            {
                // Add a new log entry
                result = SL_AddLogEntry(message, level, fileName, functionName,
                                        lineNumber, tag, supplementalData);
            }
            else
            {
                // Process a transaction
                result = SL_ProcessTransaction();
                if (result == SL_RESULT_SUCCESS)
                {
                    gLogEntryCount = 0;

                    // Initialize log entry list
                    memset((void*)gLogEntries, 0, sizeof(tSL_LogEntry) * SL_LOG_ENTRY_CACHE_SIZE);

                    // Add a new log entry
                    result = SL_AddLogEntry(message, level, fileName, functionName,
                                            lineNumber, tag, supplementalData);
                }
            }
        }
    }
    return result;
}

// =================================================================================================
//  SL_Result_String
// =================================================================================================
const char* SL_Result_String(int32_t resultCode)
{
    const char* resultString = NULL;

    // Is this a system result code?
    if (resultCode > SL_RESULT_SUCCESS)
        resultString = strerror(resultCode);

    // Is this a SQLite Logger result code?
    else if ((resultCode <= SL_RESULT_RESERVED_START) && 
            (resultCode >= SL_RESULT_RESERVED_END))
    {
        uint32_t index = (uint32_t)(-1.0 * resultCode);
        resultString = kSL_ResultStrings[index];
    }

    // This is an unknown result code
    else
        resultString = kSL_ResultStrings[0];

    return resultString;
}

// =================================================================================================
