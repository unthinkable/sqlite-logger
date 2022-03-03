#!/bin/bash
# =================================================================================================
#
#   bash-check-env.sh
#
#   Copyright (c) 2019 Unthinkable Research LLC. All rights reserved.
#
#	Author: Gary Woodcock
#
#   Supported host operating systems:
#       *nix systems capable of running bash shell.
#
#	Description:
#		This file contains a collection of bash console support functions.
#
# =================================================================================================

source "./bash-c-build-utils.sh"
source "./bash-package-manager-utils.sh"

function aptCheck () {
    printWithRightJustification "apt: " "${1}"
    if isDarwin
    then
        printColor $CONSOLE_YELLOW "Not applicable for Darwin/macOS"
    else
        if hasAdvancedPackagingTool 
        then
            APT_VERSION=$(advancedPackagingToolVersion)
            printColor $CONSOLE_GREEN "Installed (v$APT_VERSION)"
        else
            printColor $CONSOLE_RED "Not installed"
        fi
    fi
}

function bashCheck () {
    printWithRightJustification "bash: " "${1}"
    if hasBash
    then
        BASH_VERSION=$(bashVersion)
        printColor $CONSOLE_GREEN "Installed (v$BASH_VERSION)"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

function clangCheck () {
	printWithRightJustification "clang: " "${1}"
	if hasClang
	then
		CLANG_VERSION=$(clangVersion)
		printColor $CONSOLE_GREEN "Installed (v$CLANG_VERSION)"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function clangC99Check () {
	printWithRightJustification "clang C99 support: " "${1}"
    if hasClang
    then
        if clangSupportsC99
        then
            printColor $CONSOLE_GREEN "Available"
        else
            printColor $CONSOLE_RED "Not available"
        fi
    else
        printColor $CONSOLE_RED "Not applicable"
    fi
}

function clangC11Check () {
	printWithRightJustification "clang C11 support: " "${1}"
    if hasClang
    then
        if clangSupportsC11
        then
            printColor $CONSOLE_GREEN "Available"
        else
            printColor $CONSOLE_RED "Not available"
        fi
    else
        printColor $CONSOLE_RED "Not applicable"
    fi
}

function clangC17Check () {
	printWithRightJustification "clang C17 support: " "${1}"
    if hasClang
    then
        if clangSupportsC17
        then
            printColor $CONSOLE_GREEN "Available"
        else
            printColor $CONSOLE_RED "Not available"
        fi
    else
        printColor $CONSOLE_RED "Not applicable"
    fi
}

function cppcheckCheck () {
	printWithRightJustification "cppcheck: " "${1}"
	if hasCppcheck 
	then
		CPPCHECK_VERSION=$(cppcheckVersion)
		printColor $CONSOLE_GREEN "Installed (v$CPPCHECK_VERSION)"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function cunitCheck () {
	printWithRightJustification "CUnit: " "${1}"
	if hasCUnit
	then
		printColor $CONSOLE_GREEN "Installed"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function doxygenCheck () {
	printWithRightJustification "Doxygen: " "${1}"
	if hasDoxygen
	then
		DOXYGEN_VERSION=$(doxygenVersion)
		printColor $CONSOLE_GREEN "Installed (v$DOXYGEN_VERSION)"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function dumpcapCheck() {
	printWithRightJustification "dumpcap: " "${1}"
	if hasDumpcap
	then
		printColor $CONSOLE_GREEN "Installed"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function gccCheck () {
	printWithRightJustification "gcc: " "${1}"
	if hasGcc
	then
		GCC_VERSION=$(gccVersion)
		printColor $CONSOLE_GREEN "Installed (v$GCC_VERSION)"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function gccC99Check () {
	printWithRightJustification "gcc C99 support: " "${1}"
    if hasGcc 
    then
        if gccSupportsC99
        then
            printColor $CONSOLE_GREEN "Available"
        else
            printColor $CONSOLE_RED "Not available"
        fi
    else
        printColor $CONSOLE_RED "Not applicable"
    fi
}

function gccC11Check () {
	printWithRightJustification "gcc C11 support: " "${1}"
    if hasGcc 
    then
        if gccSupportsC11
        then
            printColor $CONSOLE_GREEN "Available"
        else
            printColor $CONSOLE_RED "Not available"
        fi
    else
        printColor $CONSOLE_RED "Not applicable"
    fi
}

function gccC17Check () {
	printWithRightJustification "gcc C17 support: " "${1}"
    if hasGcc 
    then
        if gccSupportsC17
        then
            printColor $CONSOLE_GREEN "Available"
        else
            printColor $CONSOLE_RED "Not available"
        fi
    else
        printColor $CONSOLE_RED "Not applicable"
    fi
}

function gitCheck () {
    printWithRightJustification "git: " "${1}"
    if hasGit 
    then
        GIT_VERSION=$(gitVersion)
        printColor $CONSOLE_GREEN "Installed (v$GIT_VERSION)"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

function gprofCheck () {
	printWithRightJustification "gprof: " "${1}"
	if hasGprof
	then
        GPROF_VERSION=$(gprofVersion)
		printColor $CONSOLE_GREEN "Installed (v$GPROF_VERSION)"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function homebrewCheck () {
    printWithRightJustification "Homebrew: " "${1}"
    if isDarwin
    then
        if hasHomebrew
        then
            printColor $CONSOLE_GREEN "Installed"
        else
            printColor $CONSOLE_RED "Not installed"
        fi
    else
        printColor $CONSOLE_YELLOW "Not applicable for Linux"
    fi
}

function jqCheck () {
    printWithRightJustification "jq: " "${1}"
    if hasJq
    then
        JQ_VERSION=$(jqVersion)
        printColor $CONSOLE_GREEN "Installed (v$JQ_VERSION)"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

function macPortsCheck () {
    printWithRightJustification "MacPorts: " "${1}"
    if isDarwin
    then
        if hasMacPorts
        then
            MACPORTS_VERSION=$(macPortsVersion)
            printColor $CONSOLE_GREEN "Installed (v$MACPORTS_VERSION)"
        else
            printColor $CONSOLE_RED "Not installed"
        fi
    else
        printColor $CONSOLE_YELLOW "Not applicable for Linux"
    fi
}

function makeCheck () {
	printWithRightJustification "make: " "${1}"
	if hasMake
	then
		MAKE_VERSION=$(makeVersion)
		printColor $CONSOLE_GREEN "Installed (v$MAKE_VERSION)"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function scanBuildCheck () {
	printWithRightJustification "scan-build: " "${1}"
	if hasScanBuild
	then
		printColor $CONSOLE_GREEN "Installed"
	else
		printColor $CONSOLE_RED "Not installed"
	fi
}

function sqlite3BuildCheck () {
    printWithRightJustification "sqlite: " "${1}"
    if hasSqlite3
    then
        SQLITE3_VERSION=$(sqlite3Version)
        printColor $CONSOLE_GREEN "Installed (v$SQLITE3_VERSION)"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

function tsharkCheck () {
    printWithRightJustification "tshark: " "${1}"
    if hasTshark
    then
        printColor $CONSOLE_GREEN "Installed"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

function valgrindCheck () {
    printWithRightJustification "valgrind: " "${1}"
    if hasValgrind
    then
        VALGRIND_VERSION=$(valgrindVersion)
        printColor $CONSOLE_GREEN "Installed (v$VALGRIND_VERSION)"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

function xmllintCheck () {
    printWithRightJustification "xmllint: " "${1}"
    if hasXmllint
    then
        printColor $CONSOLE_GREEN "Installed"
    else
        printColor $CONSOLE_RED "Not installed"
    fi
}

# =================================================================================================
