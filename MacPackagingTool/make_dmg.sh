#!/bin/sh
#See http://www.macdisk.com/dmgen.php

VERPAT="#_#_##"
VERRE="^[0-9]_[0-9]_[0-9]+$"

volname="NXSYSDelivery"
vplace=/Volumes/$volname
bz="BZ"

if [ -d $vplace ]; then
    echo "$vplace exists already. Resolve this first."
    exit 1
fi

if [[ "$1" = "" ]]; then
    version=$(python getversion.py | sed "s/\./_/g")
else
    version="$1"
fi

if [[ ! "$version" =~ $VERRE ]]; then
    echo "'release'" arg "$version" "isn't of pattern" $VERPAT
    exit 1
fi

outfile=../NXSYSMac$version$bz.dmg
dpath=../Documents/NXSYSMac$version.dmg
if [ -f $dpath ]; then
    echo $dpath exists already.
    echo Get rid of it if you want this to work.
    exit 4
fi
if [ -f $outfile ]; then
    echo Anticipated compressed vehicle $outfile already exists.
    echo Banish it on your own terms.
    exit 5
fi

echo Creating $dpath
hdiutil create -size 40m -fs HFS+ -attach $dpath -volname $volname
rc=$?
if [[ $rc != 0 ]] ; then
    echo ":(:(:( HDIUTIL create ran aground."
    exit $rc
fi

# test code
python deliver.py /Volumes/$volname
rc=$?
if [[ $rc != 0 ]] ; then
    echo ":(:(:( Python deliver.py porked it."
    exit $rc
fi

ls -lt $vplace/NXSYSMac.app/Contents/MacOS
echo unmounting ...
hdiutil detach $vplace
rc=$?
if [[ $rc != 0 ]] ; then
    echo ":(:(:( HDIUTIL unmount upchucked."
    exit $rc
fi


echo Outfile to be $outfile
hdiutil convert $dpath -format UDBZ -o $outfile
rc=$?
if [[ $rc != 0 ]]; then
    echo ":(:(:( HDIUTIL convert fouled up"
    exit $rc
fi
ls -lt $outfile
echo Le voila.
