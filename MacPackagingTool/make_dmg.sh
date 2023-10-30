#!/bin/sh

# BASH Script to create NXSYS DMG including NXSYS, TLEdit, RelayIndex.
# Just Run. Places temp file and output bz.dmg in the working dir and finds the
# applications in the user Applications dir unless you change these:

temp_dmg_dir=.
output_dir=.
user_applications=$HOME/Applications

buddy=/usr/libexec/PlistBuddy
#See http://www.macdisk.com/dmgen.php

nxsys_path=$user_applications/NXSYSMac.app

VERPAT="#_#_##_##"
VERRE="^[0-9]_[0-9]_[0-9]+_[0-9]+$"

if [ -f $nxsys_path ]; then
    echo Application path $nxsys_path "doesn't exist."
    exit 6
fi


volname="NXSYSDelivery"
vplace=/Volumes/$volname
bz="BZ"

if [ -d $vplace ]; then
    echo "$vplace exists already. Resolve this first. Dismount it."
    exit 1
fi

pl_path="$nxsys_path"/Contents/Info.plist

build_number=$($buddy -c "Print CFBundleVersion" "$pl_path")
verstr=$($buddy -c "Print CFBundleShortVersionString" "$pl_path" | sed "s/\./_/g")
version=${verstr}_$build_number
echo $version

if [[ ! "$version" =~ $VERRE ]]; then
    echo "'release'" arg "$version" "isn't of pattern" $VERPAT
    exit 1
fi

outfile=$output_dir/NXSYSMac$version$bz.dmg
dpath=$temp_dmg_dir/NXSYSMac$version.dmg
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

echo Creating "(dpath)" $dpath
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
rm $dpath
ls -lt $outfile
echo Le voila.
