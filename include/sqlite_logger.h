// =================================================================================================
//! @file sqlite_logger.h
//! @author Gary Woodcock (gary.woodcock@unthinkable.com)
//! @brief This file contains the public interface for the SQLite Logger.
//! @remarks Requires ANSI C99 (or better) compliant compilers.
//! @remarks Supported host operating systems: Any Unix/Linux
//! @date 2022-02-19
//! @copyright Copyright (c) 2022 Unthinkable Research LLC. All rights reserved.
//! 
//  Includes
// =================================================================================================
#ifdef __cplusplus
    #pragma once
#endif

#ifndef __SQLITE_LOGGER_H__
#define __SQLITE_LOGGER_H__

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// =================================================================================================
//  Constants
// =================================================================================================

//! @brief SQLite Logger implementation version string represented by this release.
#define SQLITE_LOGGER_VERSION           "0.1.0"

//! @brief SQLite Logger implementation version number represented by this release.
#define SQLITE_LOGGER_VERSION_NUMBER    00010000

//! @brief A result code indicating the function call was successful.
#define SL_RESULT_SUCCESS               0

//! @brief SQLite Logger result codes are negative; this is the start of the range.
#define SL_RESULT_RESERVED_START        -1

//! @brief A result code indicating a non-specific SQLite Logger failure.
#define SL_RESULT_FAILURE               SL_RESULT_RESERVED_START

//! @brief A result code indicating an attempt to use SQLite Logger before it's been
//! initialized.
#define SL_RESULT_NOT_INITIALIZED       (SL_RESULT_RESERVED_START - 1)

//! @brief A result code indicating an attempt to initialize SQLite Logger after it's
//! already been initialized.
#define SL_RESULT_ALREADY_INITIALIZED   (SL_RESULT_RESERVED_START - 2)

//! @brief This value represents the end of the SQLite Logger result code range.
#define SL_RESULT_RESERVED_END          -31

// =================================================================================================
//  Types
// =================================================================================================

//! @brief Pre-defined log levels.
typedef enum tsl_loglevel
{
    eSL_LogLevel_Diagnostic = 0,    //!< Log everything - diagnostic, detail, info, warnings, and errors
    eSL_LogLevel_Detail     = 1,    //!< Log only detail, info, warnings and errors
    eSL_LogLevel_Info       = 2,    //!< Log only info, warnings and errors
    eSL_LogLevel_Warning    = 3,    //!< Log only warnings and errors
    eSL_LogLevel_Error      = 4,    //!< Log only errors
    eSL_LogLevel_None       = 5     //!< Log nothing
}
tSL_LogLevel;

//! @brief This is a convenience alias for logging everything.
#define SL_LOGLEVEL_EVERYTHING  eSL_LogLevel_Diagnostic

//! @brief This is a convenience alias for logging nothing.
#define SL_LOGLEVEL_NOTHING     eSL_LogLevel_None

// =================================================================================================
//  Prototypes
// =================================================================================================

#ifdef __cplusplus
extern "C"
{
#endif

    //! @fn int32_t SL_Initialize (const char* path)
    //! @brief Call __SL_Initialize__ to initialize SQLite Logger. 
    //! __SL_Initialize__ should only be called once.
    //! @code
    //! int32_t result = SL_Initialize("/home/my-user/my-log-file.sqlite3");
    //! @endcode
    //! @param[in] path The file path to use for creating/opening the log file.
    //! @return A status code indicating whether the function call succeeded.
    //! @note A return value of __SL_RESULT_SUCCESS__ indicates the function call succeeded.
    //! @note A return value of __EFAULT__ indicates that the __path__ argument is __NULL__.
    //! @note A return value of __EINVAL__ indicates that the __path__ argument is an empty string.
    //! @note A return value of __SL_RESULT_ALREADY_INITIALIZED__ indicates that 
    //! __SL_Initialize__ has already been called once.
    //! @note Return values may also include result codes from __sqlite3__.
    //! @warning If required, the caller is responsible for allocating and deallocating 
    //! the __path__ pointer.
    //! @see SL_Terminate
    int32_t SL_Initialize (const char* path);

    //! @fn int32_t SL_Terminate (void)
    //! @brief Call __SL_Terminate__ to terminate SQLite Logger. 
    //! __SL_Terminate__ should only be called once.
    //! @code
    //! int32_t result = SL_Terminate();
    //! @endcode
    //! @return A status code indicating whether the function call succeeded. 
    //! @note A return value of __SL_RESULT_SUCCESS__ indicates the function call succeeded.
    //! @note A return value of __SL_RESULT_NOT_INITIALIZED__ indicates that __SL_Initialize__ 
    //! has not previously been called.
    //! @note Return values may also include result codes from __sqlite3__.
    //! @see SL_Initialize
    int32_t SL_Terminate (void);

    //! @fn int32_t SL_SetLogLevel (tSL_LogLevel level)
    //! @brief Call __SL_SetLogLevel__ to set the log level of SQLite Logger.
    //! @code
    //! int32_t result = SL_SetLogLevel(eSL_LogLevel_Warning);
    //! @endcode
    //! @param[in] level The log level that SQLite Logger should use when evaulating __SL_Log__ calls.
    //! @return A status code indicating whether the function call succeeded. 
    //! @note A return value of __SL_RESULT_SUCCESS__ indicates the function call succeeded.
    //! @see SL_GetLogLevel
    int32_t SL_SetLogLevel (tSL_LogLevel level);

    //! @fn int32_t SL_GetLogLevel (tSL_LogLevel* level)
    //! @brief Call __SL_GetLogLevel__ to get the log level of SQLite Logger.
    //! @code
    //! tSL_LogLevel level = eSL_LogLevel_None;
    //! int32_t result = SL_GetLogLevel(&level);
    //! @endcode
    //! @param [out] level The log level that SQLite Logger is using when evaluating __SL_Log__ calls.
    //! @return A status code indicating whether the function call succeeded. 
    //! @note A return value of __SL_RESULT_SUCCESS__ indicates the function call succeeded.
    //! @note A return value of __EFAULT__ indicates that the __level__ argument is __NULL__.
    //! @see SL_SetLogLevel
    int32_t SL_GetLogLevel (tSL_LogLevel* level);

    //! @fn int32_t SL_Log (const char* message, tSL_LogLevel level, const char* fileName, 
    //! const char* functionName, uint32_t lineNumber, const char* tag, const char* supplementalData)
    //! @brief Call SL_Log to log a message.
    //! @code
    //! int32_t result = SL_Log("This is a message.",
    //!                         eSL_LogLevel_Info,
    //!                         __FILE__, __FUNCTION__, __LINE__,
    //!                         "This is a tag.",
    //!                         "This is some supplemental data.");
    //! @endcode
    //! @param [in] message A message to log. This parameter must not be NULL.
    //! @param [in] level The level at which to log this message.
    //! @param [in] fileName The source code file name associated with this message.
    //! @param [in] functionName The function name associated with this message.
    //! @param [in] lineNumber The line number in the source code file name associated 
    //! with this message.
    //! @param [in] tag A tag to associate with this message. 
    //! @param [in] supplementalData Supplemental data to associate with the log entry.
    //! @return A status code indicating whether the function call succeeded. 
    //! @note A return value of __SL_RESULT_SUCCESS__ indicates the function call succeeded.
    //! @note A return value of __EFAULT__ indicates that the __message__ argument is __NULL__.
    //! @note A return value of __EINVAL__ indicates that the __message__ argument is an empty string.
    //! @note A return value of __SL_RESULT_NOT_INITIALIZED__ indicates that __SL_Initialize__ 
    //! has not been called.
    //! @note Return values may also include result codes from __sqlite3__.
    //! @warning If required, the caller is responsible for allocating and deallocating the 
    //! __message__, __fileName__, __functionName__, __tag__, and __supplementalData__ pointers.
    int32_t SL_Log (const char* message,
                    tSL_LogLevel level,
                    const char* fileName,
                    const char* functionName,
                    uint32_t lineNumber,
                    const char* tag,
                    const char* supplementalData);

    //! @fn const char* SL_Result_String (int32_t resultCode)
    //! @brief Call __SL_Result_String__ to get a description of a result code.
    //! @code
    //! const char* description = SL_Result_String(SL_RESULT_ALREADY_INITIALIZED);
    //! @endcode
    //! @param [in] resultCode The result code to get a description of.
    //! @return A pointer to a string describing __resultCode__. 
    //! @note The caller does not have to deallocate the returned string pointer.
    const char* SL_Result_String (int32_t resultCode);

#ifdef __cplusplus
}
#endif

// =================================================================================================
//  Macros
// =================================================================================================

//! @brief A helper macro to log a diagnostic message.
#define SL_LOG_DIAGNOSTIC_MESSAGE(message, tag, supplementalData)   \
    SL_Log(message, eSL_LogLevel_Diagnostic,                        \
           __FILE__, __FUNCTION__, __LINE__,                        \
           tag, supplementalData)

//! @brief A helper macro to log a detail message.
#define SL_LOG_DETAIL_MESSAGE(message, tag, supplementalData)       \
    SL_Log(message, eSL_LogLevel_Detail,                            \
           __FILE__, __FUNCTION__, __LINE__,                        \
           tag, supplementalData)

//! @brief A helper macro to log an info message.
#define SL_LOG_INFO_MESSAGE(message, tag, supplementalData)         \
    SL_Log(message, eSL_LogLevel_Info,                              \
           __FILE__, __FUNCTION__, __LINE__,                        \
           tag, supplementalData)

//! @brief A helper macro to log a warning message.
#define SL_LOG_WARNING_MESSAGE(message, tag, supplementalData)      \
    SL_Log(message, eSL_LogLevel_Warning,                           \
           __FILE__, __FUNCTION__, __LINE__,                        \
           tag, supplementalData)

//! @brief A helper macro to log an error message.
#define SL_LOG_ERROR_MESSAGE(message, tag, supplementalData)        \
    SL_Log(message, eSL_LogLevel_Error,                             \
           __FILE__, __FUNCTION__, __LINE__,                        \
           tag, supplementalData)

//! @brief A helper macro to log an assertion failure.
#define SL_LOG_ASSERT(condition, tag, supplementalData)             \
{                                                                   \
    if (!(condition))                                               \
        (void)SL_Log("Assertion failed!",                           \
                     eSL_LogLevel_Error,                            \
                     __FILE__, __FUNCTION__, __LINE__,              \
                     tag, supplementalData);                        \
}

// =================================================================================================
#endif	// __SQLITE_LOGGER_H__
// =================================================================================================
