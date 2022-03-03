#!/bin/bash
# =================================================================================================
#
#   build.sh
#
#   Copyright (c) 2022 Unthinkable Research LLC. All rights reserved.
#
#	Author: Gary Woodcock
#
#   Supported host operating systems:
#       *nix systems capable of running bash shell.
#
#	Description:
#		This script builds and tests the SQLite Logger.
#
# =================================================================================================

# Clear screen
clear

# Set built-ins
set -o errexit
set -o pipefail
set -o nounset

# Stash original working directory 
STARTING_WORKING_DIR="$PWD"

# Stash start date/time
SECONDS=0
BUILD_START="$(date '+%a %b %d %Y %T %Z')"

# Include bash utilities
source "./bash-architecture-utils.sh"
source "./bash-c-build-utils.sh"
source "./bash-c-code-quality-utils.sh"
source "./bash-check-env.sh"
source "./bash-console-utils.sh"
source "./bash-docs-utils.sh"
source "./bash-file-system-utils.sh"
source "./bash-misc-utils.sh"
source "./bash-os-utils.sh"
source "./bash-shell-utils.sh"
source "./bash-string-utils.sh"
source "./bash-unit-test-utils.sh"

# Set background color 
setBackgroundColor $NORMAL_BACKCOLOR

# Set foreground color 
setForegroundColor $NORMAL_FORECOLOR

# =================================================================================================
#	Constants
# =================================================================================================

# Commands
ANALYZE_CMD="--analyze"
CHECK_ENV_CMD="--check-env"
CLEAN_CMD="--clean"
DEBUG_CMD="--debug"
HELP_CMD="--help"
RELEASE_CMD="--release"
ROOT_DIRECTORY_PATH_CMD="--root-directory-path"
VERBOSE_CMD="--verbose"
WITH_DOCUMENTATION_CMD="--with-documentation"
WITH_LOG_ENTRY_CACHE_SIZE_CMD="--with-log-entry-cache-size"
WITH_PROFILING_CMD="--with-profiling"
WITH_SDK_CMD="--with-sdk"
WITH_SHARED_LIBS_CMD="--with-shared-libs"
WITH_UNIT_TESTING_CMD="--with-unit-testing"

# Analyze options
ANALYZE_OPTION_CPPCHECK="cppcheck"
ANALYZE_OPTION_FULL="full"
ANALYZE_OPTION_NONE="none"
ANALYZE_OPTION_SCAN_BUILD="scan-build"

# =================================================================================================
#	Functions
# =================================================================================================

# Print summary
function printSummary () {
	BUILD_DURATION=$SECONDS
	BUILD_STOP="$(date '+%a %b %d %Y %T %Z')"
    INDENT_LEN=20

	printBanner "SQLITE LOGGER BUILD SUMMARY"
	printWithRightJustification "Build started at: " $INDENT_LEN
    printIt "$BUILD_START"
	printWithRightJustification "Build finished at: " $INDENT_LEN
    printIt "$BUILD_STOP"
	printDuration $BUILD_DURATION $INDENT_LEN

    printWithRightJustification "Configuration: " $INDENT_LEN
	if [ $BUILD_OPERATING_ENV == "darwin" ]
	then
		if [ $BUILD_ARCH == "x64" ]
		then
            printIt "Darwin x64"
        elif [ $BUILD_ARCH == "arm64" ]
        then
            printIt "Darwin arm64"
		elif [ $BUILD_ARCH == "armhf" ]
		then
			printIt "Darwin armhf"
		else
			printIt "Darwin unknown"
		fi
	elif [ $BUILD_OPERATING_ENV == "linux" ]
		then
		if [ $BUILD_ARCH == "x64" ]
		then
			printIt "Linux x64"
        elif [ $BUILD_ARCH == "arm64" ] || [ $BUILD_ARCH == "aarch64" ]
        then
            printIt "Linux arm64"
		elif [ $BUILD_ARCH == "armhf" ]
		then
			printIt "Linux armhf"
		else
			printIt "Linux unknown"
		fi
	else
		printIt "Unknown"
	fi

	if [ $BUILD_DEBUG -eq 1 ]
	then
		if [ $BUILD_CLEAN -eq 1 ]
		then
			if [ $BUILD_SHARED_LIB -eq 1 ]
			then
                printWithIndent "Clean debug build with shared library\n" $INDENT_LEN
			else
				printWithIndent "Clean debug build with static library\n" $INDENT_LEN
			fi
		else
			if [ $BUILD_SHARED_LIB -eq 1 ]
			then
				printWithIndent "Incremental debug build with shared library\n" $INDENT_LEN
			else
				printWithIndent "Incremental debug build with static library\n" $INDENT_LEN
			fi
		fi
	else
		if [ $BUILD_CLEAN -eq 1 ]
		then
            if [ $BUILD_SHARED_LIB -eq 1 ]
            then
				printWithIndent "Clean release build with shared library\n" $INDENT_LEN
            else
				printWithIndent "Clean release build with static library\n" $INDENT_LEN
            fi
		else
            if [ $BUILD_SHARED_LIB -eq 1 ]
            then
				printWithIndent "Incremental release build with shared library\n" $INDENT_LEN
            else
				printWithIndent "Incremental release build with static library\n" $INDENT_LEN
            fi
		fi
	fi

    if [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_CPPCHECK ] || [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_FULL ]
    then
        if hasCppcheck
        then
            printWithIndent "With cppcheck code quality analysis\n" $INDENT_LEN
        else
            printWithIndent "Without cppcheck code quality analysis\n" $INDENT_LEN
        fi
    else
        printWithIndent "Without cppcheck code quality analysis\n" $INDENT_LEN
    fi

    if [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_SCAN_BUILD ] || [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_FULL ]
    then
        if hasScanBuild
        then
            printWithIndent "With scan-build code quality analysis\n" $INDENT_LEN
        else
            printWithIndent "Without scan-build code quality analysis\n" $INDENT_LEN
        fi
    else
        printWithIndent "Without scan-build code quality analysis\n" $INDENT_LEN
    fi

	if [ $BUILD_PROFILE -eq 1 ]
	then
		printWithIndent "With profiling\n" $INDENT_LEN
	else
        printWithIndent "Without profiling\n" $INDENT_LEN
	fi

	if [ $BUILD_DOCS -eq 1 ]
	then
		printWithIndent "With documenation\n" $INDENT_LEN
	else
		printWithIndent "Without documentation\n" $INDENT_LEN
	fi

    if [ $BUILD_SDK -eq 1 ]
    then
        printWithIndent "With SDK\n" $INDENT_LEN
    else
    
        printWithIndent "Without SDK\n" $INDENT_LEN
    fi

    if [ $BUILD_WITH_UNIT_TESTING -eq 1 ]
    then
        printWithIndent "With unit testing\n" $INDENT_LEN
    else
        printWithIndent "Without unit testing\n" $INDENT_LEN
    fi
	
	printIt " "
	resetConsoleAttributes
}

# Check host enviroment
function checkHostEnvironment () {

    if isDarwin
    then
        if isIntel64Architecture
        then
	        printBanner "DARWIN x64 HOST ENVIRONMENT CHECK"
        elif isARM64Architecture
        then
            printBanner "DARWIN arm64 HOST ENVIRONMENT CHECK"
        else
            printBanner "DARWIN HOST ENVIRONMENT CHECK"
        fi
    elif isLinux
    then
        if isIntel64Architecture
        then
            printBanner "LINUX x64 HOST ENVIRONMENT CHECK"
        elif isARM64Architecture
        then
            printBanner "LINUX arm64 HOST ENVIRONMENT CHECK"
        else
	        printBanner "LINUX HOST ENVIRONMENT CHECK"
        fi
    else
	    printBanner "HOST ENVIRONMENT CHECK"
    fi

    LABEL_WIDTH=20

    if isLinux
    then
        aptCheck $LABEL_WIDTH
    fi
    bashCheck $LABEL_WIDTH
    clangCheck $LABEL_WIDTH
    clangC99Check $LABEL_WIDTH
    cppcheckCheck $LABEL_WIDTH
    cunitCheck $LABEL_WIDTH
    doxygenCheck $LABEL_WIDTH
    gccCheck $LABEL_WIDTH
    gccC99Check $LABEL_WIDTH
    gitCheck $LABEL_WIDTH
    gprofCheck $LABEL_WIDTH
    if isDarwin
    then
        homebrewCheck $LABEL_WIDTH
        macPortsCheck $LABEL_WIDTH
    fi
    makeCheck $LABEL_WIDTH
    scanBuildCheck $LABEL_WIDTH
    xmllintCheck $LABEL_WIDTH

    printIt " "
	exit 1
}

# Function to handle error
function handleError () {
	printIt "\n"
	printError "$1"
	printIt "\n"
	if hasNotifySend
	then
		notify-send "SQLite Logger Build" "Build failed!"
	fi
	printSummary
	exit 1
}

# Function to print usage
function printUsage {
	printColor $CONSOLE_GREEN "USAGE:  build.sh <args>"
	printIt " "
	printIt "\tAll arguments are optional. With no arguments, the default behavior is:"
	printIt " "
    printIt "\t• Code analysis with $ANALYZE_OPTION_CPPCHECK"
	printIt "\t• Incremental debug build of programs and static library"
	printIt "\t• Root directory path is '$BUILD_ROOT'"
    printIt "\t• No SDK"
    printIt "\t• No verbose output"
	printIt "\t• Without documentation build"
    printIt "\t• With log entry cache size of '$BUILD_LOG_ENTRY_CACHE_SIZE'"
	printIt "\t• Without profiling"
    printIt "\t• Without unit testing"
	printIt " "
	printIt "\tPossible argument values are:"
	printIt " "
    printIt "\t$ANALYZE_CMD=<$ANALYZE_OPTION_NONE|$ANALYZE_OPTION_FULL|$ANALYZE_OPTION_CPPCHECK|$ANALYZE_OPTION_SCAN_BUILD>\tAnalyzes the source code with the specified tools."
	printIt "\t$CHECK_ENV_CMD\t\t\t\t\tChecks the build support on the host environment."
	printIt "\t$CLEAN_CMD\t\t\t\t\t\tForces a clean build instead of an incremental build."
	printIt "\t$DEBUG_CMD\t\t\t\t\t\tBuilds debug version."
	printIt "\t$HELP_CMD\t\t\t\t\t\tPrints this usage notice."
	printIt "\t$RELEASE_CMD\t\t\t\t\tBuilds release version."
	printIt "\t$ROOT_DIRECTORY_PATH_CMD=<path>\t\t\tSets the path to the root directory containing the SQLite Logger"
    printIt "\t\t\t\t\t\t\tsource code directory (defaults to the user's home directory)."
    printIt "\t$VERBOSE_CMD\t\t\t\t\tPrints all build log output to console."
	printIt "\t$WITH_DOCUMENTATION_CMD\t\t\t\tBuilds documentation using Doxygen."
    printIt "\t$WITH_LOG_ENTRY_CACHE_SIZE_CMD=<value>\t\tSets the size of the log entry cache."
	printIt "\t$WITH_PROFILING_CMD\t\t\t\tBuilds with profiling enabled (Linux only)."
    printIt "\t$WITH_SDK_CMD\t\t\t\t\tCreates a Software Development Kit (SDK) archive in the results directory."
	printIt "\t$WITH_SHARED_LIBS_CMD\t\t\t\tBuild and link with shared library instead of static library."
    printIt "\t$WITH_UNIT_TESTING_CMD\t\t\t\tPerform unit testing after build."
	printIt " "
	printIt "\tPrerequisites for running this script include:"
	printIt " "
	printIt "\t• bash shell"
    printIt "\t• clang or gcc with C99 support"
	printIt "\t• cppcheck (used with --analyze=cppcheck|full options)"
    printIt "\t• CUnit (used with --with-unit-testing option)"
	printIt "\t• Doxygen (used with --with-documentation option)"
    printIt "\t• gprof (used with --with-profiling option)"
    printIt "\t• make"
    printIt "\t• scan-build (used with --analyze=scan-build|full options)"
    printIt "\t• xmllint (used to parse unit test results)"
	printIt " "
	exit 1
}

# Function to parse command line arguments
function parseCommandLineArgument {
    if stringBeginsWithSubstring "$CMD_LINE_ARG" "$ANALYZE_CMD"
    then
        BUILD_ANALYZE_OPTION=$(removeLeadingSubstring "$CMD_LINE_ARG" "$ANALYZE_CMD""=")
	elif [ "$CMD_LINE_ARG" == $CHECK_ENV_CMD ]
	then
		checkHostEnvironment
	elif [ "$CMD_LINE_ARG" == $CLEAN_CMD ]
	then
		BUILD_CLEAN=1
	elif [ "$CMD_LINE_ARG" == $DEBUG_CMD ]
	then
		BUILD_DEBUG=1
		if [ $BUILD_RELEASE -eq 1 ]
		then
			BUILD_RELEASE=0
		fi
	elif [ "$CMD_LINE_ARG" == $HELP_CMD ]
	then
		printUsage
	elif [ "$CMD_LINE_ARG" == $WITH_DOCUMENTATION_CMD ]
	then
		if hasDoxygen
		then
			BUILD_DOCS=1
		else
			BUILD_DOCS=0
			printWarning "Doxygen doesn't appear to be installed; overriding $WITH_DOCUMENTATION_CMD."
            printIt ""
		fi
    elif stringBeginsWithSubstring "$CMD_LINE_ARG" "$WITH_LOG_ENTRY_CACHE_SIZE_CMD"
    then
        BUILD_LOG_ENTRY_CACHE_SIZE=$(removeLeadingSubstring "$CMD_LINE_ARG" $WITH_LOG_ENTRY_CACHE_SIZE_CMD"=")
        if [ "$BUILD_LOG_ENTRY_CACHE_SIZE" == "" ]
        then
            handleError "Log entry cache size must not be empty!"
        fi
        if stringBeginsWithSubstring "$BUILD_LOG_ENTRY_CACHE_SIZE" "-"
        then
            handleError "Log entry cache size can't be negative!"
        fi
        if [ "$BUILD_LOG_ENTRY_CACHE_SIZE" -eq "0" ]
        then
            handleError "Log entry cache size can't be zero!"
        fi
	elif [ "$CMD_LINE_ARG" == $WITH_PROFILING_CMD ]
	then
		if hasGprof
		then
			BUILD_PROFILE=1
		else
			BUILD_PROFILE=0
			printWarning "gprof not available; overriding $WITH_PROFILING_CMD."
            printIt ""
		fi
	elif [ "$CMD_LINE_ARG" == $RELEASE_CMD ]
	then
		BUILD_RELEASE=1
		if [ $BUILD_DEBUG -eq 1 ]
		then
			BUILD_DEBUG=0
		fi
    elif stringBeginsWithSubstring "$CMD_LINE_ARG" "$ROOT_DIRECTORY_PATH_CMD"
	then
		BUILD_ROOT=$(removeLeadingSubstring "$CMD_LINE_ARG" $ROOT_DIRECTORY_PATH_CMD"=")
		if [ "$BUILD_ROOT" == "" ]
		then
			handleError "Root directory path must not be empty!"
		fi
		if ! stringEndsWithSubstring "$BUILD_ROOT" "/"
		then
			BUILD_ROOT="$BUILD_ROOT""/"
		fi
	elif [ "$CMD_LINE_ARG" == $VERBOSE_CMD ]
	then
		BUILD_VERBOSE=1
    elif [ "$CMD_LINE_ARG" == $WITH_SDK_CMD ]
    then
        BUILD_SDK=1
	elif [ "$CMD_LINE_ARG" == $WITH_SHARED_LIBS_CMD ]
	then	
		BUILD_SHARED_LIB=1
    elif [ "$CMD_LINE_ARG" == $WITH_UNIT_TESTING_CMD ]
    then
        BUILD_WITH_UNIT_TESTING=1
	else
		printError "Unrecognized argument: '$CMD_LINE_ARG'."
		printIt ""
		printUsage
		exit 1
	fi
}

# =================================================================================================
#	Setup
# =================================================================================================

# Set build defaults
if isDarwin 
then
    BUILD_OPERATING_ENV="darwin"
elif isLinux
then
    BUILD_OPERATING_ENV="linux"
else
	BUILD_OPERATING_ENV="unknown"
fi

if isARM64Architecture
then
	BUILD_ARCH="arm64"
elif isARMArchitecture
then
    BUILD_ARCH="armhf"
elif isIntel64Architecture
then
    BUILD_ARCH="x64"
else
	BUILD_ARCH="unknown"
fi

# Default values
BUILD_ANALYZE_OPTION=$ANALYZE_OPTION_CPPCHECK
BUILD_CFG=Debug
BUILD_CLEAN=0
BUILD_DEBUG=1
BUILD_DOCS=0
BUILD_LIB_EXTENSION=".a"
BUILD_LOG_ENTRY_CACHE_SIZE="1024"
BUILD_PRODUCTS_BIN_DIR="bin"
BUILD_PRODUCTS_DIR_NAME="sqlite-logger"
BUILD_PRODUCTS_OBJ_DIR="obj"
BUILD_PROFILE=0
BUILD_RELEASE=0
BUILD_ROOT="$HOME"
BUILD_SDK=0
BUILD_SHARED_LIB=0
BUILD_VERBOSE=0
BUILD_WITH_UNIT_TESTING=0

# Log tags
CLEAN_LOG_PREFIX="_clean_log"
BUILD_LOG_PREFIX="_build_log"
CPPCHECK_LOG_PREFIX="_cppcheck_log"
SCAN_BUILD_LOG_PREFIX="_scan-build_log"
LOG_POSTFIX=".txt"

# If there are arguments, check them
for var in "$@"
do
	CMD_LINE_ARG=$var
	parseCommandLineArgument $CMD_LINE_ARG
done

BUILD_PRODUCTS_DIR_NAME="$STARTING_WORKING_DIR"
BUILD_PRODUCTS_DIR_NAME="$(removeLeadingSubstring $BUILD_PRODUCTS_DIR_NAME $BUILD_ROOT)"
BUILD_PRODUCTS_DIR_NAME="$(removeLeadingSubstring $BUILD_PRODUCTS_DIR_NAME '/')"
BUILD_PRODUCTS_DIR_NAME="${BUILD_PRODUCTS_DIR_NAME%/*}"

printBanner "SQLITE LOGGER BUILD"

printIt "Setting up..."

# Sanity check debug vs. release
if [ $BUILD_DEBUG -eq 0 ]
then
	if [ $BUILD_RELEASE -eq 0 ]
	then
		BUILD_DEBUG=1
	fi
fi

# Export debug/release/profile flag
if [ $BUILD_DEBUG -eq 1 ]
then
	BUILD_CFG=Debug
elif [ $BUILD_RELEASE -eq 1 ]
then
	BUILD_CFG=Release
fi

if fileExists "../include/sqlite_logger_config.h"
then
	forceDeleteFile "../include/sqlite_logger_config.h"
fi
echo "#define SL_LOG_ENTRY_CACHE_SIZE $BUILD_LOG_ENTRY_CACHE_SIZE" >> "../include/sqlite_logger_config.h"

# Create build directory if necessary
if ! directoryExists "$BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/"
then
	createDirectory "$BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/"
fi

# Create the logs directory if necessary
BUILD_LOGS_DIR="$BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/logs/"
if ! directoryExists "$BUILD_LOGS_DIR"
then
    createDirectory "$BUILD_LOGS_DIR"
fi

# Create the results directory if necessary
BUILD_RESULTS_DIR="$BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/results/"
if ! directoryExists "$BUILD_RESULTS_DIR"
then
    createDirectory "$BUILD_RESULTS_DIR"
fi

# Define program directory
PROG_DIR="$BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/$BUILD_PRODUCTS_BIN_DIR/$BUILD_OPERATING_ENV/$BUILD_ARCH/$BUILD_CFG"

# Check library file extension
if [ $BUILD_SHARED_LIB -eq 1 ]
then
    BUILD_LIB_EXTENSION=".so"
fi

# Export build environment
export BUILD_ARCH
export BUILD_CFG
export BUILD_LOGS_DIR
export BUILD_OPERATING_ENV
export BUILD_PRODUCTS_BIN_DIR
export BUILD_PRODUCTS_DIR_NAME
export BUILD_PRODUCTS_OBJ_DIR
export BUILD_PROFILE
export BUILD_RESULTS_DIR
export BUILD_ROOT
export BUILD_SHARED_LIB

printIt " "

# Make sure there are no out-of-date libs that could be accidentally linked to
forceDeleteDirectory "$PROG_DIR"/*.*

# =================================================================================================
#	Clean
# =================================================================================================

# Clean?
if [ $BUILD_CLEAN -eq 1 ]
then
	printBanner "CLEAN"

    printIt "Cleaning docs...\n"
    if directoryExists "../docs/html"
    then
        deleteDirectory "../docs/html"
    fi
    if directoryExists "../docs/latex"
    then
        deleteDirectory "../docs/latex"
    fi

	printIt "Cleaning logs...\n"
	if directoryExists "$BUILD_LOGS_DIR"
	then
		deleteDirectory "$BUILD_LOGS_DIR"
		createDirectory "$BUILD_LOGS_DIR"
	fi

    printIt "Cleaning results...\n"
    if directoryExists "$BUILD_RESULTS_DIR"
    then
        deleteDirectory "$BUILD_RESULTS_DIR"
        createDirectory "$BUILD_RESULTS_DIR"
    fi

    cleanIt "libsqlitelogger$BUILD_LIB_EXTENSION" "../src" makefile $BUILD_VERBOSE "$BUILD_LOGS_DIR/libsqlitelogger$CLEAN_LOG_PREFIX$LOG_POSTFIX"
    cleanIt "sqlite_logger_unit_test" "../test" makefile $BUILD_VERBOSE "$BUILD_LOGS_DIR/sqlite_logger_unit_test$CLEAN_LOG_PREFIX$LOG_POSTFIX"
fi

# =================================================================================================
#	Code quality analysis
# =================================================================================================

if [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_FULL ] || [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_CPPCHECK ]
then
    printBanner "CODE QUALITY ANALYSIS: CPPCHECK"

    printIt "Checking for cppcheck..."
    if ! hasCppcheck
    then
        printWarning "\tcppcheck not available.\n"
    else
        printSuccess "\tcppcheck available.\n"

        checkIt "libsqlitelogger" "../src" $BUILD_VERBOSE "$BUILD_LOGS_DIR/libsqlitelogger$CPPCHECK_LOG_PREFIX$LOG_POSTFIX" ""
        checkIt "sqlite_logger_unit_test" "../test" $BUILD_VERBOSE "$BUILD_LOGS_DIR/sqlite_logger_unit_test$CPPCHECK_LOG_PREFIX$LOG_POSTFIX" ""
    fi
fi

if [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_FULL ] || [ $BUILD_ANALYZE_OPTION == $ANALYZE_OPTION_SCAN_BUILD ]
then
    printBanner "CODE QUALITY ANALYSIS: SCAN-BUILD"

    printIt "Checking for scan-build..."
    if ! hasScanBuild
    then
        printWarning "\tscan-build not available.\n"
    else
        printSuccess "\tscan-build available.\n"

        scanIt "libsqlitelogger" "../src" makefile $BUILD_VERBOSE "$BUILD_LOGS_DIR/libsqlitelogger$SCAN_BUILD_LOG_PREFIX$LOG_POSTFIX"
        scanIt "sqlite_logger_unit_test" "../test" makefile $BUILD_VERBOSE "$BUILD_LOGS_DIR/sqlite_logger_unit_test$SCAN_BUILD_LOG_PREFIX$LOG_POSTFIX"
    fi
fi

# =================================================================================================
#	Build
# =================================================================================================

printBanner "BUILD"

# Libraries
buildIt "libsqlitelogger$BUILD_LIB_EXTENSION" "../src" makefile $BUILD_VERBOSE "$BUILD_LOGS_DIR/libsqlitelogger$BUILD_LOG_PREFIX$LOG_POSTFIX" ""

# Programs
buildIt "sqlite_logger_unit_test" "../test" makefile $BUILD_VERBOSE "$BUILD_LOGS_DIR/sqlite_logger_unit_test$BUILD_LOG_PREFIX$LOG_POSTFIX" ""

# =================================================================================================
#   Unit test
# =================================================================================================

if [ $BUILD_WITH_UNIT_TESTING -eq 1 ]
then
    printBanner "UNIT TEST"

    UNIT_TEST_INDENT=30

	printIt "Running SQLite Logger unit test..."
    "$PROG_DIR/sqlite_logger_unit_test"
    mv "$BUILD_LOGS_DIR/SQLite_Logger_Unit_Test-Results.xml" "$BUILD_LOGS_DIR/sqlite_logger_unit_test_log.xml"
    UNIT_TEST_XML=$(cat "$BUILD_LOGS_DIR/sqlite_logger_unit_test_log.xml")
    parseCUnitResults "$UNIT_TEST_XML" "$UNIT_TEST_INDENT"
    printIt ""
fi

# =================================================================================================
#   Documentation
# =================================================================================================

if [ $BUILD_DOCS -eq 1 ]
then
	printBanner "DOCUMENTATION"

	printIt "Building HTML documentation at $BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/docs/html..."

	pushPath "$BUILD_ROOT/$BUILD_PRODUCTS_DIR_NAME/docs" $BUILD_VERBOSE
	doxygen "./Doxyfile"
	if directoryExists "./html"
	then
		printIt "Opening index.html in browser..."
		if isLinux
		then
			xdg-open "./html/index.html"
		elif isDarwin
		then
			open "./html/index.html"
		fi
	else
		printError "Failed to generate HTML documentation!"
	fi
	popPath $BUILD_VERBOSE
	printIt " "
fi

# =================================================================================================
#	Assemble SDK
# =================================================================================================

if [ $BUILD_SDK -eq 1 ]    
then	
    printBanner "ASSEMBLE SDK"

    if [ $BUILD_SHARED_LIB -eq 1 ]
    then
        SQLITE_LOGGER_SDK_DIR="sqlite-logger-sdk-shared-lib-$BUILD_OPERATING_ENV-$BUILD_ARCH-$BUILD_CFG"
    else
        SQLITE_LOGGER_SDK_DIR="sqlite-logger-sdk-static-lib-$BUILD_OPERATING_ENV-$BUILD_ARCH-$BUILD_CFG"
    fi

    pushPath "$BUILD_RESULTS_DIR" $BUILD_VERBOSE

    printIt "Preparing..."
    if fileExists "$SQLITE_LOGGER_SDK_DIR.tar.gz"
    then
        forceDeleteFile "$SQLITE_LOGGER_SDK_DIR.tar.gz"
    fi

    if directoryExists "$SQLITE_LOGGER_SDK_DIR"
    then    
        forceDeleteDirectory "$SQLITE_LOGGER_SDK_DIR"
    fi

    printIt "Creating directories and copying files..."
    createDirectory "$SQLITE_LOGGER_SDK_DIR"
    cp "../README.md" "./$SQLITE_LOGGER_SDK_DIR"
    cp "../LICENSE" "./$SQLITE_LOGGER_SDK_DIR"
    createDirectory "./$SQLITE_LOGGER_SDK_DIR/include"
    cp "../include/sqlite_logger.h" "./$SQLITE_LOGGER_SDK_DIR/include"
    if fileExists "$PROG_DIR/libsqlitelogger$BUILD_LIB_EXTENSION"
    then
        createDirectory "./$SQLITE_LOGGER_SDK_DIR/bin"
        cp "$PROG_DIR/libsqlitelogger$BUILD_LIB_EXTENSION" "./$SQLITE_LOGGER_SDK_DIR/bin"
    else
        handleError "$PROG_DIR/libsqlitelogger.$BUILD_LIB_EXTENSION doesn't exist!"
    fi

    printIt "Creating archive..."
    if [ $BUILD_VERBOSE -eq 1 ]
    then
        tar -czvf "$SQLITE_LOGGER_SDK_DIR.tar.gz" "$SQLITE_LOGGER_SDK_DIR"
    else
        tar -czf "$SQLITE_LOGGER_SDK_DIR.tar.gz" "$SQLITE_LOGGER_SDK_DIR"
    fi
    forceDeleteDirectory "$SQLITE_LOGGER_SDK_DIR"

    popPath $BUILD_VERBOSE
    printSuccess "\tSDK created at $BUILD_RESULTS_DIR\$SQLITE_LOGGER_SDK_DIR.tar.gz."
    printIt ""

fi

# =================================================================================================
#	Summary
# =================================================================================================

if hasNotifySend
then
	notify-send "SQLite Logger Build" "Build succeeded."
fi
printSummary

# =================================================================================================
