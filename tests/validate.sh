#!/bin/sh
# Copyright (C) 1999-2012 ImageMagick Studio LLC
#
# This program is covered by multiple licenses, which are described in
# LICENSE. You should have received a copy of LICENSE with this
# package; otherwise see http://www.wizards-toolkit.org/script/license.php.
#
#  Test for 'validate' utility.
#

set -e # Exit on any error
. ${srcdir}/tests/common.sh

${VALIDATE} ${PASSPHRASE}
