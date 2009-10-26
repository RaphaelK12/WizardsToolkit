#!/bin/sh
# Copyright (C) 1999-2008 ImageMagick Studio LLC
#
# This program is covered by multiple licenses, which are described in
# LICENSE. You should have received a copy of LICENSE with this
# package; otherwise see http://www.imagemagick.org/script/license.php.
#
#  Test for '${ENCIPHER}' utility.
#

set -e # Exit on any error
. ${srcdir}/utilities/tests/common.sh

${ENCIPHER} -verbose -keyring ${MYKEYRING} -passphrase ${PASSPHRASE} ${PLAINTEXT} ${CIPHERTEXT}