import os
import sys
import shutil
import getversion

 # ("Myrtle","Atlantic","Progman","Islington", "240thSt")
#Packed by Xcode or MSI now.
SPECS = ()
APPS_PATH = os.path.expanduser("~/Applications")

def copyI(src, dest):
    shutil.copytree(src, os.path.join(dest, src))

def copy_file(fname, dest):
    path = os.path.join(APPS_PATH,fname)
    destpath = os.path.join(dest, fname)
    if (not ((os.path.exists(path)))):
        print ("Putative file", path, "is not an extant file.", file=sys.stderr)
        sys.exit(6)
    print ("Copying", fname, "to", destpath)
    shutil.copy(path,destpath)

def copy_app(appname,dest):
    path = os.path.join(APPS_PATH,appname)
    destpath = os.path.join(dest, appname)
    if (not ((os.path.exists(path) and os.path.isdir(path)))):
        print ("Putative app", path, "is not an extant directory.", file=sys.stderr)
        sys.exit(5)
    print ("Copying", appname, getversion.getMacAppVersion(APPS_PATH,appname), "to", destpath)
    shutil.copytree(path, destpath)

def main():
    for d in SPECS:
        if (not ((os.path.exists(d) and os.path.isdir(d)))):
            print("Source interlocking \"" + d + "\" is not an extant directory.", file=sys.stderr)
            sys.exit(4)


    if (len(sys.argv) != 2):
        print >>sys.stderr,"Usage: %s -dir-, where -dir- is likely a .dmg top dir." % sys.argv[0]
        sys.exit(2)

    dest = os.path.abspath(sys.argv[1])
    if (not ((os.path.exists(dest) and os.path.isdir(dest)))):
           print (dest, "is not an extant directory.", file=sys.stderr)
           sys.exit(3)

    for d in SPECS:
        copyI(d,dest)

    shutil.copy2("Mac-Readme.txt", os.path.join(dest,"Mac-Readme.txt"))
    copy_app("NXSYSMac.app",dest)
    copy_app("TLEdit.app",dest)
    copy_file("RelayIndex",dest)
    print (f"dest: {dest}")
    os.system("ls -lt \"" + dest + "\"")

if __name__ == "__main__":
    main()
    
