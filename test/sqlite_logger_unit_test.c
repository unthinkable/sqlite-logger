// =================================================================================================
//! @file sqlite_logger_unit_test.c
//! @author Gary Woodcock (gary.woodcock@unthinkable.com)
//! @brief This file implements the unit test for the SQLite Logger.
//! @remarks Requires ANSI C99 (or better) compliant compilers.
//! @remarks Supported host operating systems: Any
//! @date 2022-02-20
//! @copyright Copyright (c) 2022 Unthinkable Research LLC. All rights reserved.
//! 
//  Includes
// =================================================================================================
#include <CUnit.h>
#include <Automated.h>
#include <stdlib.h>
#include <time.h>
#include "sqlite_logger.h"

// =================================================================================================
//  Private constants
// =================================================================================================
#define LOG_PATH    "../results/sqlite_logger_unit_test.sqlite3"

// =================================================================================================
//  SL_SuiteInit
// =================================================================================================
int SL_SuiteInit (void)
{
    CU_ErrorCode status = CUE_SUCCESS;

    int32_t result = SL_Initialize(LOG_PATH);
    if (result != SL_RESULT_SUCCESS)
    {
        status = CUE_SINIT_FAILED;
        CU_FAIL_FATAL("SL_Initialize failed!");
    }
    return status;
}

// =================================================================================================
//  SL_SuiteCleanup
// =================================================================================================
int SL_SuiteCleanup (void)
{
    (void)SL_Terminate();
    
    return CUE_SUCCESS;
}

// =================================================================================================
//  SL_TestLogLevel
// =================================================================================================
void SL_TestLogLevel (void)
{
    // Test log level get/set
    int32_t result = SL_RESULT_SUCCESS;
    tSL_LogLevel level = eSL_LogLevel_None;

    // Default should be info
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_Info);

    // Make sure we can set to diagnostic
    result = SL_SetLogLevel(eSL_LogLevel_Diagnostic);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_Diagnostic);

    // Make sure we can set to detail
    result = SL_SetLogLevel(eSL_LogLevel_Detail);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_Detail);

    // Make sure we can set to warning
    result = SL_SetLogLevel(eSL_LogLevel_Warning);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_Warning);

    // Make sure we can set to error
    result = SL_SetLogLevel(eSL_LogLevel_Error);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_Error);

    // Make sure we can set to none
    result = SL_SetLogLevel(eSL_LogLevel_None);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_None);

    // Make sure we can set to info
    result = SL_SetLogLevel(eSL_LogLevel_Info);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_GetLogLevel(&level);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    CU_ASSERT_EQUAL(level, eSL_LogLevel_Info);

    // Try to get level with bad argument
    result = SL_GetLogLevel(NULL);
    CU_ASSERT_EQUAL(result, EFAULT);

    // Try to set level with bad argument
    result = SL_SetLogLevel((tSL_LogLevel)1234);
    CU_ASSERT_EQUAL(result, EINVAL);
}

// =================================================================================================
//  SL_TestLogging
// =================================================================================================
void SL_TestLogging (void)
{
    int32_t result = SL_RESULT_SUCCESS;

    // Make sure we can't re-initialized once initialized
    result = SL_Initialize(LOG_PATH);
    CU_ASSERT_EQUAL(result, SL_RESULT_ALREADY_INITIALIZED);

    result = SL_SetLogLevel(eSL_LogLevel_Diagnostic);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Test log variations
    result = SL_LOG_DIAGNOSTIC_MESSAGE("This is a diagnostic message.",
                                       "Diagnostic tag", "Diagnostic supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    
    result = SL_LOG_DETAIL_MESSAGE("This is a detail message.",
                                   "Detail tag", "Detail supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_LOG_INFO_MESSAGE("This is an info message.",
                                 "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_LOG_WARNING_MESSAGE("This is a warning message.",
                                    "Warning tag", "Warning supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_LOG_ERROR_MESSAGE("This is an error message.",
                                  "Error tag", "Error supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Test with missing message
    result = SL_LOG_INFO_MESSAGE(NULL, "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, EFAULT);

    // Test with empty message
    result = SL_LOG_INFO_MESSAGE("", "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, EINVAL);

    // Test with bad level
    result = SL_Log("This is an info message with a bad level",
                    (tSL_LogLevel)5678,
                    __FILE__, __FUNCTION__, __LINE__,
                    "Some tag", "Some supplemental data");
    CU_ASSERT_EQUAL(result, EINVAL);

    // Test with optional arguments
    result = SL_Log("This is an info message with no file name",
                    eSL_LogLevel_Info,
                    NULL, __FUNCTION__, __LINE__,
                    "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_Log("This is an info message with no function name",
                    eSL_LogLevel_Info,
                    __FILE__, NULL, __LINE__,
                    "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_Log("This is an info message with a line number of 0",
                    eSL_LogLevel_Info,
                    __FILE__, __FUNCTION__, 0,
                    "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_Log("This is an info message with no tag.",
                    eSL_LogLevel_Info,
                    __FILE__, __FUNCTION__, __LINE__,
                    NULL, "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    result = SL_Log("This is an info message with no supplemental data.",
                    eSL_LogLevel_Info,
                    __FILE__, __FUNCTION__, __LINE__,
                    "Info tag", NULL);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log a diagnostic message with log level set at detail
    result = SL_SetLogLevel(eSL_LogLevel_Detail);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_DIAGNOSTIC_MESSAGE("This is a diagnostic message that shouldn't be logged.",
                                       "Diagnostic tag", "Diagnostic supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log a detail message with log level set at info
    result = SL_SetLogLevel(eSL_LogLevel_Info);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_DIAGNOSTIC_MESSAGE("This is a detail message that shouldn't be logged.",
                                       "Detail tag", "Detail supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log an info message with log level set at warning
    result = SL_SetLogLevel(eSL_LogLevel_Warning);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_INFO_MESSAGE("This is an info message that shouldn't be logged.",
                                 "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log a warning message with log level set at error
    result = SL_SetLogLevel(eSL_LogLevel_Error);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_WARNING_MESSAGE("This is a warning message that shouldn't be logged.",
                                    "Warning tag", "Warning supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log a diagnostic message with log level set at none
    result = SL_SetLogLevel(eSL_LogLevel_None);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_DIAGNOSTIC_MESSAGE("This is a diagnostic message that shouldn't be logged.",
                                       "Diagnostic tag", "Diagnostic supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log a detail message with log level set at none
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_DETAIL_MESSAGE("This is a detail message that shouldn't be logged.",
                                   "Detail tag", "Detail supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log an info message with log level set at none
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);
    result = SL_LOG_INFO_MESSAGE("This is an info message that shouldn't be logged.",
                                 "Info tag", "Info supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log a warning message with log level set at none
    result = SL_LOG_WARNING_MESSAGE("This is a warning message that shouldn't be logged.",
                                    "Warning tag", "Warning supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Try to log an error message with log level set at none
    result = SL_LOG_ERROR_MESSAGE("This is an error message that shouldn't be logged.",
                                  "Error tag", "Error supplemental data");
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Reset log level to info
    result = SL_SetLogLevel(eSL_LogLevel_Info);
    CU_ASSERT_EQUAL(result, SL_RESULT_SUCCESS);

    // Test assert true
    bool test = true;
    SL_LOG_ASSERT(test == true, "Pass", "test == true");
    SL_LOG_ASSERT(test == false, "Fail", "test == false");
}

// =================================================================================================
//  main
// =================================================================================================
int main (int argc, const char * argv[])
{
    // Initialize CUnit
    CU_ErrorCode result = CU_initialize_registry();
    if (result == CUE_SUCCESS)
    {
        // Set up test suite
        CU_pSuite testSuite = CU_add_suite("SQLite Logger test suite",
                                           SL_SuiteInit,
                                           SL_SuiteCleanup);
        if (testSuite != NULL)
        {
            CU_ADD_TEST(testSuite, SL_TestLogLevel);
            CU_ADD_TEST(testSuite, SL_TestLogging);
        }
        else    // CU_add_suite failed
        {
            result = CU_get_error();
            printf("\tCU_add_suite failed with error code %d!\n", result);
        }

        // Check for success
        if (result == CUE_SUCCESS)
        {
            CU_set_output_filename("../logs/SQLite_Logger_Unit_Test");
            CU_automated_run_tests();
        }

        // Clean up
        CU_cleanup_registry();
    }
    else    // CU_initialize_registry failed
        printf("\tCU_initialize_registry failed with error code %d!\n", result);

    return CU_get_error();
}

// =================================================================================================

