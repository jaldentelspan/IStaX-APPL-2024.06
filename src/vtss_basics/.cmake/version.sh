#!/usr/bin/env bash
#
# Copyright (c) 2006-2017 Microsemi Corporation "Microsemi". All Rights Reserved.
#
# Unpublished rights reserved under the copyright laws of the United States of
# America, other countries and international treaties. Permission to use, copy,
# store and modify, the software and its source code is granted but only in
# connection with products utilizing the Microsemi switch and PHY products.
# Permission is also granted for you to integrate into other products, disclose,
# transmit and distribute the software only in an absolute machine readable
# format (e.g. HEX file) and only in or with products utilizing the Microsemi
# switch and PHY products.  The source code of the software may not be
# disclosed, transmitted or distributed without the prior written permission of
# Microsemi.
#
# This copyright notice must appear in any copy, modification, disclosure,
# transmission or distribution of the software.  Microsemi retains all
# ownership, copyright, trade secret and proprietary rights in the software and
# its source code, including all modifications thereto.
#
# THIS SOFTWARE HAS BEEN PROVIDED "AS IS". MICROSEMI HEREBY DISCLAIMS ALL
# WARRANTIES OF ANY KIND WITH RESPECT TO THE SOFTWARE, WHETHER SUCH WARRANTIES
# ARE EXPRESS, IMPLIED, STATUTORY OR OTHERWISE INCLUDING, WITHOUT LIMITATION,
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR USE OR PURPOSE AND
# NON-INFRINGEMENT.
#

if [ $# -ne 2 ]
then
    echo "Usage: version.sh <projectname> <output_path>"
    exit $E_BADARGS
fi

NAME=$1
OUTPUT_PATH=$2

C_HDR=${OUTPUT_PATH}/buildinfo_${NAME}.h
CMAKE=${OUTPUT_PATH}/buildinfo_${NAME}.cmake

HASH=$(git rev-parse HEAD)

if [[ $(git diff --shortstat 2> /dev/null | tail -n1) != "" ]] ||
   [[ $(git diff --cached --shortstat 2> /dev/null | tail -n1) != "" ]]
then
    DIRTY="-dirty"
    DIRTY_NUM="1"
else
    DIRTY=""
    DIRTY_NUM="0"
fi

if [ $(git describe --match "v[0-9]*.[0-9]*" --dirty --abbrev=8 2> /dev/null) ]
then
    VERSION_TAG_STRING="$(git describe --match "v[0-9]*.[0-9]*" --dirty --abbrev=8)"

    echo $VERSION_TAG_STRING | grep -q "^v\([0-9]\+\)\.\([0-9]\+\)-\([0-9]\+\)"
    if [ $? -eq 0 ]
    then
        VERSION1=`echo $VERSION_TAG_STRING | sed -e "s/^v\([0-9]\+\)\.\([0-9]\+\).*/\1/"`
        VERSION2=`echo $VERSION_TAG_STRING | sed -e "s/^v\([0-9]\+\)\.\([0-9]\+\).*/\2/"`
        VERSION3=`echo $VERSION_TAG_STRING | sed -e "s/^v\([0-9]\+\)\.\([0-9]\+\)-\([0-9]\+\).*/\3/"`
    else
        VERSION1=`echo $VERSION_TAG_STRING | sed -e "s/^v\([0-9]\+\)\.\([0-9]\+\).*/\1/"`
        VERSION2=`echo $VERSION_TAG_STRING | sed -e "s/^v\([0-9]\+\)\.\([0-9]\+\).*/\2/"`
        VERSION3="0"
    fi

else
    VERSION_TAG_STRING=$HASH$DIRTY
    VERSION1="0"
    VERSION2="0"
    VERSION3="0"

fi

VERSION_FULL="$VERSION1.$VERSION2.$VERSION3"


BUILDNO_FILE="${OUTPUT_PATH}/buildno.txt"
BUILDNO_FILE_NEW="${OUTPUT_PATH}/buildno_new.txt"

if [ -f $BUILDNO_FILE ]
then
    awk '/[0-9]+/{ printf "%d\n", $1+1 }' < $BUILDNO_FILE > $BUILDNO_FILE_NEW
    mv $BUILDNO_FILE_NEW $BUILDNO_FILE
else
    echo 0 > $BUILDNO_FILE
fi

BUILDNO=$(cat $BUILDNO_FILE)

BRANCH_NAME=$(git symbolic-ref HEAD 2>/dev/null | sed -e 's,.*/\(.*\),\1,') || BRANCH_NAME="(unnamed branch)"


rm -f $C_HDR
echo "#define ${NAME}_NAME           \"$NAME\""                      >> $C_HDR
echo "#define ${NAME}_BRANCH_NAME    \"$BRANCH_NAME\""               >> $C_HDR
echo "#define ${NAME}_VERSION_TAG_STRING \"$VERSION_TAG_STRING\""    >> $C_HDR
echo "#define ${NAME}_VERSION_HASH   \"$HASH\""                      >> $C_HDR
echo "#define ${NAME}_VERSION1       $VERSION1"                      >> $C_HDR
echo "#define ${NAME}_VERSION2       $VERSION2"                      >> $C_HDR
echo "#define ${NAME}_VERSION3       $VERSION3"                      >> $C_HDR
echo "#define ${NAME}_VERSION        \"$VERSION_FULL\""              >> $C_HDR
echo "#define ${NAME}_USER           \"$USER\""                      >> $C_HDR
echo "#define ${NAME}_MACHINE        \"$(hostname -f)\""             >> $C_HDR
echo "#define ${NAME}_BUILD_TIME     \"$(date --rfc-3339=seconds)\"" >> $C_HDR
echo "#define ${NAME}_BUILD_NUMBER   $BUILDNO"                       >> $C_HDR
echo "#define ${NAME}_BUILD_DIRTY    $DIRTY_NUM"                     >> $C_HDR

rm -f $CMAKE
echo "set(${NAME}_NAME               \"$NAME\")"                      >> $CMAKE
echo "set(${NAME}_BRANCH_NAME        \"$BRANCH_NAME\")"               >> $CMAKE
echo "set(${NAME}_VERSION_TAG_STRING \"$VERSION_TAG_STRING\")"        >> $CMAKE
echo "set(${NAME}_VERSION_HASH       \"$HASH\")"                      >> $CMAKE
echo "set(${NAME}_VERSION1           $VERSION1)"                      >> $CMAKE
echo "set(${NAME}_VERSION2           $VERSION2)"                      >> $CMAKE
echo "set(${NAME}_VERSION3           $VERSION3)"                      >> $CMAKE
echo "set(${NAME}_VERSION            \"$VERSION_FULL\")"              >> $CMAKE
echo "set(${NAME}_USER               \"$USER\")"                      >> $CMAKE
echo "set(${NAME}_MACHINE            \"$(hostname -f)\")"             >> $CMAKE
echo "set(${NAME}_BUILD_TIME         \"$(date --rfc-3339=seconds)\")" >> $CMAKE
echo "set(${NAME}_BUILD_NUMBER       $BUILDNO)"                       >> $CMAKE
echo "set(${NAME}_BUILD_DIRTY        $DIRTY_NUM)"                     >> $CMAKE

echo "set(THIS_PROJECT_NAME               \"$NAME\")"                      >> $CMAKE
echo "set(THIS_PROJECT_BRANCH_NAME        \"$BRANCH_NAME\")"               >> $CMAKE
echo "set(THIS_PROJECT_VERSION_TAG_STRING \"$VERSION_TAG_STRING\")"        >> $CMAKE
echo "set(THIS_PROJECT_VERSION_HASH       \"$HASH\")"                      >> $CMAKE
echo "set(THIS_PROJECT_VERSION1           $VERSION1)"                      >> $CMAKE
echo "set(THIS_PROJECT_VERSION2           $VERSION2)"                      >> $CMAKE
echo "set(THIS_PROJECT_VERSION3           $VERSION3)"                      >> $CMAKE
echo "set(THIS_PROJECT_VERSION            \"$VERSION_FULL\")"              >> $CMAKE
echo "set(THIS_PROJECT_USER               \"$USER\")"                      >> $CMAKE
echo "set(THIS_PROJECT_MACHINE            \"$(hostname -f)\")"             >> $CMAKE
echo "set(THIS_PROJECT_BUILD_TIME         \"$(date --rfc-3339=seconds)\")" >> $CMAKE
echo "set(THIS_PROJECT_BUILD_NUMBER       $BUILDNO)"                       >> $CMAKE
echo "set(THIS_PROJECT_BUILD_DIRTY        $DIRTY_NUM)"                     >> $CMAKE

