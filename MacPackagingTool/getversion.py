
#Not used anymore -- PListBuddy now used -- ut not deleting from tree...
#PlistBuddy is necessary for build, anyway.

import os
import re
import sys
import xml.etree.ElementTree as ET

def processTrackDict(dictNode):  # as it were
    d = {}
    key = None
    for n in dictNode.iter():
        if key:
            d[key] = True if n.tag =="true" else n.text
            key = None
        elif n.tag == "key":
            key = n.text
    return d

def getMacAppVersion(app_path):
    infop = os.path.join(app_path, "Contents/Info.plist")
    tree = ET.parse(infop)
    dictElt = tree.findall("./dict")[0]
    dict = processTrackDict(dictElt)
    version = dict["CFBundleShortVersionString"]
    rversion = re.sub("/.*$", "", version)
    buildno = dict["CFBundleVersion"]
    return rversion + "." + buildno

if __name__ == "__main__":
    if (len(sys.argv) < 2):
        print("getversion.py: missing argumment (application pathname\n", file=sys.stderr)
        sys.exit(3)
    sys.stdout.write(getMacAppVersion(sys.argv[1]))
