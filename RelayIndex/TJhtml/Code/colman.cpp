//
//  colman.cpp
//  "Te colman de bendiciones"
//  TJhtml
//
//  Created by Bernard Greenberg on 1/21/22.
//



#include "tjdcls.h"
#include "colman.hpp"

#include "tjio.h"

static int NCols = 0;
static int Colgroup = 1;

static vector<StringOutputSink> Columns;
static OutputSink * OrigSink = nullptr;

void NewCol() {
    Columns.emplace_back();
    O = &Columns.back();
}

void columns_command(int ncols) {
    Columns.clear();
    NCols = ncols;
    if (ncols == 1)
        return;
    OrigSink = O;
    NewCol();
}

void column_command() {
    
    if (Columns.size() == NCols)
        column_dumpfinish();
    NewCol();
}

void column_dumpfinish() {
    if (Columns.size() == 0)
        return;
    O = OrigSink;
    O->puts("<table>\n");
    for (int i = 0; i < NCols; i++)
        O->putf("    <col width=\"%d\">\n", PageWidth/NCols);
    O->puts("<tr>\n");
    int index = 1;
    for (auto col : Columns) {
        O->putf("    <td class=\"column-%d column-%d.%d\" style=\"vertical-align:top;\">\n",
                index, Colgroup, index);
        index ++;
        O->puts(col.get());
        fndump();
        O->puts("</td>\n");
    }
    O->puts("</tr></table>");
    Columns.clear();
    Colgroup ++;
 //    OrigSink = nullptr;
}
