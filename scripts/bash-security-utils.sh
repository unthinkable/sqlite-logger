#!/bin/bash
# =================================================================================================
#
#   bash-security-utils.sh
#
#   Copyright (c) 2019 Unthinkable Research LLC. All rights reserved.
#
#	Author: Gary Woodcock
#
#   Supported host operating systems:
#       *nix systems capable of running bash shell.
#
#	Description:
#		This file contains a collection of bash security support functions.
#
# =================================================================================================

source "./bash-os-utils.sh"
source "./bash-string-utils.sh"
source "./bash-misc-utils.sh"

# Function to check for OpenSSL
function hasOpenSSL () {
	if isDarwin
	then
		if hasMacPorts
		then
			OPENSSL_INSTALLED=$(port installed | grep openssl)
			if stringHasSubstring "$OPENSSL_INSTALLED" "openssl"; then
				true
			else
				false
			fi
		else
			false
		fi
	elif isLinux
	then
		OPENSSL_INSTALLED=$(ldconfig -p | grep libcrypto)
		if stringHasSubstring "$OPENSSL_INSTALLED" "libcrypto"; then
			OPENSSL_INSTALLED=$(ldconfig -p | grep libssl)
			if stringHasSubstring "$OPENSSL_INSTALLED" "libssl"; then
				true
			else
				false
			fi
		else
			false
		fi
	else
		false
	fi
}

# =================================================================================================
