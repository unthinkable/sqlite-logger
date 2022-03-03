#!/bin/bash
# =================================================================================================
#
#   bash-misc-utils.sh
#
#   Copyright (c) 2019 Unthinkable Research LLC. All rights reserved.
#
#	Author: Gary Woodcock
#
#   Supported host operating systems:
#       *nix systems capable of running bash shell.
#
#	Description:
#		This file contains a collection of miscellaneous bash support functions.
#
# =================================================================================================

source "./bash-shell-utils.sh"
source "./bash-os-utils.sh"
source "./bash-string-utils.sh"

# Function to check for glide
function hasGlide () {
	if cmdInstalled "glide"; then
		true
	else
		false
	fi
}

# Function to check for Go
function hasGo () {
	if cmdInstalled "go"; then
		true
	else
		false
	fi
}

# Function to check for libBSD
function hasLibBsd () {
	if isDarwin
	then	
		# libbsd is integrated with macOS
		true
	elif isLinux
	then
		LIBBSD_INSTALLED=$(ldconfig -p | grep libbsd)
		if stringHasSubstring "$LIBBSD_INSTALLED" "libbsd"; then
			true
		else
			false
		fi
	else
		false
	fi
}

# Function to check for dieharder
function hasDieharder () {
	if cmdInstalled "dieharder"; then
		true
	else
		false
	fi
}

# Function to check for zenity
function hasZenity () {
	if cmdInstalled "zenity"; then
		true
	else
		false
	fi
}

# Function to check for notify-send
function hasNotifySend () {
	if cmdInstalled "notify-send"; then
		true
	else
		false
	fi
}

# Function to check for jq
function hasJq () {
    if cmdInstalled "jq"; then
        true 
    else
        false
    fi
}

# Function to get jq version
function jqVersion () {
    if hasJq
    then
        JQ_VER="$(jq --version)"
        if stringBeginsWithSubstring "$JQ_VER" "jq-"
        then
            JQ_VER=${JQ_VER#"jq-"}
            printf $JQ_VER
        else
            printf "unknown"
        fi
    else
        printf "N/A"
    fi
}

# Function to check for xmllint
function hasXmllint () {
    if cmdInstalled "xmllint"; then
        true
    else
        false
    fi
}

# Function to check for sqlite3
function hasSqlite3 () {
    if cmdInstalled "sqlite3"; then
        true
    else
        false
    fi
}

# Function to get sqlite3 version
function sqlite3Version () {
    if hasSqlite3
    then
        SQLITE3_VER="$(sqlite3 --version)"
        SQLITE3_VER=${SQLITE3_VER% *}
        SQLITE3_VER=${SQLITE3_VER% *}
        SQLITE3_VER=${SQLITE3_VER% *}
        echo "$SQLITE3_VER"
    else
        printf "N/A"
    fi
}

# =================================================================================================

