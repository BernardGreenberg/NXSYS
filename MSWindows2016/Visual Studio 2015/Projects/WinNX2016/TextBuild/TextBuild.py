import os
import re
import sys
import argparse

def doit():
    parser=argparse.ArgumentParser(description="Parse .otc's into c file")
    parser.add_argument('-o', '--output', metavar="FILE", help="Output file")
    parser.add_argument('files', nargs="+", help="input otc files")
    args = parser.parse_args(sys.argv)
    cfile = ""

    for f in args.files:
        (otcdir,otcent) = os.path.split(f)
        for line in open(f):
            m = re.match(r"\s*Symbol:.*Name\s*=\s*(.*?)\s*,.*path\s*=\s*(.*?)[,;]", line)
            if m:
                (symbol,path) = m.groups()
                path = os.path.join(otcdir,path)
                data = open(path).read()
                data = data.replace("\\", "\\\\")
                data = data.replace("\"", "\\\"")
                data = data.replace("'", "\\\'")
                

                cfile += ("\nchar * " + symbol + " =\n" + "\n \"" + data + "\";\n")
    open(args.output,"w").write(cfile)


if __name__ == "__main__":
    doit()