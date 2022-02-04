#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <ctime>
#include <chrono>
#include <filesystem>

#include "CP437.h"
#include "tjdcls.h"
#include "tjio.h"
#include "tjjuda.h"
#include "Unicode437.hpp"
#include "colman.hpp"
#include "argparse.hpp"
#include "StringSetHash.hpp"

namespace fs = std::filesystem;
using std::unordered_set;
using std::unordered_map;

using StringStringMap = unordered_map<string, const char *>;

static const unordered_set<string> UnimplementedCommands = {
    "pgnos", "need", "lheight", "lmar", "adjusting", "size", "nbfont", "fam", "pagetop"
};

string_set_hash::StringSetHashMap CcovlMap {
    {{"e", "dieresis"}, "ë"},
    {{"a", "macron"}, "ā"},
    {{"dotlessi", "macron"}, "ī"},
    {{"n", "period"}, "ṇ"},
    {{"c", "caron"}, "č"},
    {{"o","acute"}, "ó"}
};

static const char * NBModeToFace[] = {"normal", "i", "b", "b"};

enum Quotrans {QUOTRAN_NONE=0, QUOTRAN_ASCII=1, QUOTRAN_ENGLISH=2, QUOTRAN_FRENCH=3, QUOTRAN_GERMAN=4};

unordered_map<string, Quotrans> QuotranMap {
    {{"none", QUOTRAN_NONE}, {"ascii", QUOTRAN_ASCII}, {"english", QUOTRAN_ENGLISH}, {"french", QUOTRAN_FRENCH}, {"german", QUOTRAN_GERMAN}, {"", QUOTRAN_NONE}
    }
};

static const char * OpenQuotes[] {"&#171;", "\"", "“", "&#171;", "&#187;"};
static const char * CloseQuotes[] {"&#187;", "\"", "”", "&#187;", "&#171;"};

InputSource* I = nullptr;
OutputSink* O = nullptr;
StringOutputSink* FootnoteSink = nullptr;

int PageWidth = 800;

#define HTML_HEADER1 \
"<!DOCTYPE html>\n" \
"<html>\n" \
" <head>\n" \
"   <meta charset=\"utf-8\">\n "

#define HTML_HEADER2 \
" </head>\n " \
" <body style=\"max-width:800px\">\n \
"

struct FontDef {
    string Name;
    string Family;
    string Face;
    string Size;
    int RealSize;
    void Execute(int cx);
    string MapSize();
};

unordered_map<string, FontDef*> FontMap;

struct State {
    int FontState;
    int NoBreak;
    int Quotran;
    int IndentpIng;
    State();
};

State::State () {
    FontState = 0;
    NoBreak = 0;
    Quotran = QUOTRAN_NONE;
    IndentpIng = 0;
}

static State MainState;
static State FootState;
static State* S;
static int Footx = 1;

void outsnout (char ender);
void Process207();
void OutputDateTime();

static int HexDigit (char c) {
    if (isupper(c))
        c = tolower(c);
    if ('0'<= c && c <='9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

char get_matching_delim (char c) {
    switch (c) {
        case '(': return ')';
        case '{': return '}';
        case '[': return ']';
        case '<': return '>';
        case '"': return '"';
        default: return 0;
    }
}

char collect_arg(string& result, char xc, char xc2, bool no_iq) {
    int poss_cx  = 0, poss_cx_closer = 0;
    if (not no_iq) { //iq = "internal quote"
        poss_cx = I->getc();
        poss_cx_closer = get_matching_delim(poss_cx);
        if (!poss_cx_closer)
            I->ungetc(poss_cx);
    }
    while (true) {
        int c = I->getc();
        if (c == poss_cx_closer) //c cannot be 0
            return I->getc();
        if (c == xc || c == xc2 || c == EOF)
            return c;
        result += c;
    }
}

string collect_arg(char xc, char xc2, bool no_iq) {
    string result;
    collect_arg(result, xc, xc2, no_iq);
    return result;
}

char collect_name (string& result,  int c) {
    while (isspace(c))
        c = I->getc();
    while (isalnum(c)) {
        result += c;
        c = I->getc();
    }
    stdlwr(result);
    return c;
}

vector<string> collect_variadic_args(int cx) {
    vector<string>result;
    while (true) {
        string temp;
        char delim = collect_arg(temp, ',', cx);
        result.push_back(temp);
        if (delim == cx)
            return result;
    }
}

string CCovl(char cx) {
    vector<string> key = collect_variadic_args(cx);
    if (CcovlMap.count(key))
        return CcovlMap[key];
    else
        fabort("ccovl case not known: %s\n",
               string_accumulate(key, [](string s) {return s + " ";}).c_str());
    return "";
}

void ExecuteWithOutput(OutputSink * OS, State* state, std::function<void()> fcn){
    State* SaveState = S;
    S = state;
    OutputSink *SaveOS = O;
    O = FootnoteSink;
    fcn();
    S = SaveState;
    O = SaveOS;
}

void footnote (int cx) {
    int fno = Footx++;
    O->putf("<sup><a  id=\"ret%d\" href=\"#foot%d\">%d</a></sup>", fno, fno, fno);
    ExecuteWithOutput(FootnoteSink, &FootState,
      [fno,cx] () {
        O->putf("<p id=\"foot%d\">%d.&nbsp;&nbsp;", fno, fno);
        outsnout (cx);
        O->putf(" <a href=\"#ret%d\">BACK</a></p>\n", fno);
    });
}

void fndump () {
    O->puts("<div class=\"footnotes\">\n");
    O->puts(FootnoteSink->get());
    O->puts("</div>\n");
    FootnoteSink->clear();
}

void fncontext(int cx) {
    ExecuteWithOutput(FootnoteSink, &FootState, [cx] () {outsnout(cx);});
}
                      
vector<string> SplitEnvVar(string varname) {
    vector<string> result;
    const char * slchars = getenv(varname.c_str());
    if (!slchars)  // No env variable, return 0 len array
        return result;
    string val(slchars);
    while (val.find(ENVDELIM) != string::npos) {
        size_t i = val.find(ENVDELIM);
        result.push_back(val.substr(0, i));
        val = val.substr(i + 1);
    }
    result.push_back(val);
    return result;
}

void libinclude (char cx) {
    fs::path ename = fs::path(collect_arg(cx));
    fs::path path = ename;
    auto dirs = SplitEnvVar("snoutlib");
    for (auto dir : dirs) {
        fs::path try_path = fs::path(dir) / ename;
        if (fs::exists(try_path)) {
            path = try_path;
            break;
        }
    }
    /* will succeed or bomb out with pathless name -- Has destructor to close */
    FileInputSource FIS (path);
    run_pushed_source(&FIS);
}


void ignorearg (int cx) {
    while (true) {
        int c = I->getc();
        if (c == cx || c == EOF)
            return;
    }
}

void inff  (int cx, int hf) {
    FontDef F;

    if (hf)
        collect_arg (','); //ignore for now
    
    F.Family = collect_arg (',');
    F.Face = collect_arg (',');
    F.Size = collect_arg(',');
 
    F.Execute(cx);
}

void FontDef::Execute(int cx) {

    if (Family == "IBM-PC-Bats") {
        if (collect_arg(cx) == "Q")
            O->putf("%s", UTF8_RAW_SNOUT);
        return;
    }

    if (Size.length())
        O->putf("<font class=\"fontsize-%s\">",
                Size.c_str());


    if (Face ==  "italic")
        O->puts("<i>");
    else if (Face == "bold")
        O->puts("<b>");

    outsnout (cx);
    
    if (Face == "bold")
        O->puts("</b>");
    else if (Face == "italic")
        O->puts("</i>");

    if (Size.length())
        O->puts("</font>");
}

string FontDef::MapSize() {
    if (Size == "20")
        return "140%";
    printf("SIZE %s\n", Size.c_str());
    return "100%";

}

void deffont (int cx) {
    FontDef * fd = new FontDef;
    if ('=' != collect_arg (fd->Name, cx, '='))
        fabort("Bad syntax in deffont %s\n", fd->Name.c_str());
    if (',' == collect_arg (fd->Family, cx, ','))
        if (',' == collect_arg (fd->Face, cx, ','))
            fd->Size = atoi(collect_arg(cx).c_str());
    FontMap[fd->Name] = fd;
}

unordered_map<unsigned char, string> CharMacs;

CU parse_charspec(const string& spec) {
    if (spec[0] == '/')
        return (CU)spec[1];
    fabort ("Unknown char spec: %s\n", spec.c_str());
    return 0;
}

void defcharmac(int cx) {
    string charspec;
    if ('=' != collect_arg (charspec, cx, '='))
        fabort("Bad syntax in defcharmac\n");
    CharMacs[parse_charspec(charspec)] = collect_arg(cx);
}

void brace (int cx, const char * tag) {
    O->putf("<%s>", tag);
    outsnout (cx);
    O->putf("</%s>", tag);
}

void run_pushed_source(InputSource* is) {
    InputSource * saveIS = I;
    I = is;
    outsnout(EOF);
    I = saveIS;
}

static StringStringMap TJInternalCharsets
    {   {"ellipsis", "..."},  // ☻lwch
        {"oe", "œ"},
        {"ae", "æ"},
        {"OE", "Œ"},
        {"AE", "Æ"},
        {"aacute", "á"},
        {"eacute", "é"},
        {"iacute", "í"},
        {"oacute", "ó"},
        {"uacute", "ú"},
        {"ograve", "ò"},
        {"dagger", "†"},
        {"lslash", "ł"},
        {"trademark", "<sup>TM</sup>"}, // ☻sym, ☻symbol
        {"trademarkserif", "<sup>TM</sup>"},
        {"plus", "+"},
        {"minus", "-"},
        {"second", "\""},
        {"partialdiff", "∂"}
};

static StringStringMap BraceCommands
  {   {"super", "sup"},
      {"sub", "sub"},
      {"i", "i"},
      {"b", "b"},
      {"u", "u"},
      {"tt", "tt"},
      {"center", "center"}
  };


void process_tjctl  (const string cmd, char c) {
    string arg;
    char cx = get_matching_delim (c);

    if (cmd == "lwch" || cmd == "sym" || cmd == "symbol") {
        collect_arg (arg, cx);
        if (TJInternalCharsets.count(arg))
            O->puts(TJInternalCharsets[arg]);
        else
            O->putf("☻%s(%s)", cmd.c_str(), arg.c_str()); //just leave in output as is in source
    }
    else if (cmd == "dash") {
        collect_arg (arg, cx);
        if (arg == "en")
            O->puts("—");
        else if (arg == "em")
            O->puts("—");
        else O->puts("—");
    }
    else if (BraceCommands.count(cmd))
        brace(cx, BraceCommands[cmd]);
    else if (cmd =="q") {
        O->puts (OpenQuotes[S->Quotran]);
        outsnout (cx);
        O->puts (CloseQuotes[S->Quotran]);
    }
    else if (cmd == "nobrk") {
        S->NoBreak++;
        outsnout (cx);
        S->NoBreak--;
    }
    else if (cmd == "hbar") {
        ignorearg (cx);
        O->puts("<hr>");
    }
    
    else if (cmd =="ul" || cmd == "hl") {
        O->puts("<font face=\"Arial,Helvetica\">");
        outsnout (cx);
        O->puts("</font>");
    }
    else if (cmd == "comment")
        ignorearg (cx);
    else if (cmd =="indentp") {
        string arg = collect_arg (cx);
        if (arg == "" ||  arg == "0") {
            if (S->IndentpIng) {
                O->puts("</blockquote>");
                S->IndentpIng = 0;
            }
        }
        else if (!S->IndentpIng) {
            S->IndentpIng = 1;
            O->puts("<blockquote>");
        }
    }
    else if (cmd == "quotran") {
        string arg(collect_arg(cx));
        stdlwr(arg);
        if (QuotranMap.count(arg))
            S->Quotran = QuotranMap[arg];
        /* else ignore */
    }
    else if (cmd == "inff")
        inff (cx, 0);
    else if (cmd == "inffh")
        inff (cx, 1);
    else if (cmd == "columns") {
        auto args = collect_variadic_args(cx);
        if (args.size() == 0 || args[0] == "")
            column_dumpfinish();
        else
            columns_command(atoi(args[0].c_str()));
    }
    else if (cmd == "column") {
        ignorearg(cx);
        column_command();
    }
    else if (cmd == "footnote")
        footnote (cx);
    else if (cmd == "fncontext")
        fncontext (cx);
    else if (cmd == "fndump") {
        ignorearg(cx);
        fndump();
    }
    else if (cmd == "libinclude")
        libinclude (cx);
    else if (cmd =="deffont")
        deffont (cx);
    else if (cmd == "hex") {
        Process207();
        I->getc();
    }
    else if (cmd == "typeout")
        printf("%s\n", CP437toUTF8String(collect_arg(cx)).c_str());  /* NOT HTML-escaped! */

    /* These two do not exist in real TJ(oo), but are defined to help debug this program */
    /* Interpret snoutsource expressed in UTF-8 (437 smileys, too, accepted) */
    else if (cmd == "utf8") {
        StringInputSource UIS (UnicodeTo437(collect_arg(cx)));
        run_pushed_source(&UIS);
    }
    
    /* Interpret the argument as UTF-8/HTML, and copy into output page literally */
    else if (cmd == "litout") {
        O->puts(collect_arg(cx));
        I->getc(); // No sé.
    }


    else if (cmd == "ccovl")
        O->puts(CCovl(cx));
    else if (cmd == "rjust") {
        ignorearg(cx);
        O->puts("&nbsp;&nbsp;&nbsp;&nbsp;");
    }
    else if (cmd == "defcharmac")
        defcharmac(cx);
    else if (cmd == "datime") {
        ignorearg(cx);
        OutputDateTime();
    }
    else if (cmd == "hebrew1")
        O->puts(hebrew1(collect_arg(cx), true));
    else if (cmd == "yiddish1")
        O->puts(yiddish1(collect_arg(cx)));
    else if (cmd == "nphebrew1")
        O->puts(hebrew1(collect_arg(cx), false));
    else if (InterpretPossibleMacroCommand(cmd, cx)) {
        /* all will be done there if it is one*/
    }
    else if (FontMap.count(cmd))
        FontMap[cmd]->Execute(cx);
    /* Just throw these away */
    else if (UnimplementedCommands.count(cmd))
        ignorearg(cx);
    /* keep all the rest as snout source in the document */
    else {
        O->putf("%s%s%c", UTF8_INVERSE_SMILEY, cmd.c_str(), c);
        outsnout (cx); /* but process stuff inside it */
        O->putc(cx);
    }
}

void ProcessNSmiley () {
    int c = I->getc();
    if (c == '\r' || c == '\n') {
        if (c == '\r')
            I->getc();
        return;
    }
    else if (c == CP437_INVERSE_SMILEY) {
        O->puts(UTF8_INVERSE_SMILEY);
        return;
    }
    else if (c == ';') {
        c = I->getc();
        int saveit = (c == '/');
        while (c != EOF && c != '\r')
            c = I->getc();
        if (c == EOF)
            return;
        /* must be \r */
        if (saveit)
            I->ungetc(c);
        else {
            c = I->getc();
            if (c != '\n' && c != EOF)
                I->ungetc(c);
        }
        return;
    }
    else if (c == '.') {
        O->puts("&nbsp;&nbsp;");
        return;
    }
    else if (c == '*')
        return;
    string ctlname;
    c = collect_name (ctlname, c);
    while (isspace(c))
        c = I->getc();
    process_tjctl (ctlname, c);
}

void processIP() {
    bool got2 = false;
    int arg1 = 0, arg2 = 0;
    int c = 0;
    while (true) {
        c = I->getc();
        if (isdigit(c))
            arg1 = 10*arg1 + c - '0';
        else break;
    }
    if (c == ',') {
        got2 = true;
        while (true) {
            c = I->getc();
            if (isdigit(c))
                arg2 = arg2*10 + c - '0';
            else break;
        }
    }
    if (c == CP437_CLOSE_GUILLEMOT) {
        if (got2)
            O->putf("<br/><b>[IP&nbsp;%d,%d]</b><br/>", arg1, arg2);
        else
            O->putf("<br/><b>[IP&nbsp;%d]</b><br/>", arg1);
    }
}

void ProcessNotaBene () {
    static unordered_map<string, enum Fstate> NBModeMap = {
        { {"MDNM", FSTATE_NORMAL}, {"MDRV", FSTATE_ITALIC}, {"MDBO", FSTATE_BOLD},
          {"MDUL", FSTATE_UNDERLINE} }};

    char buf[5];
    memset(buf, 0, sizeof(buf));
    I->read_bytes(buf, 2);
    string word(buf);
    stdupr(word);
    if (word == "IP") {
        processIP();
        return;
    }
    I->read_bytes(buf+2, 2);
    word = buf;
    I->getc();  // close guillemot
    stdupr(word);

    if (!NBModeMap.count(word))
        fabort("Unknown NB MDxx: %s\n", word.c_str());
    enum Fstate newstate = NBModeMap["word"];
    if (S->FontState != newstate) {
        if (S->FontState != 0)
            O->putf("</%s>", NBModeToFace[S->FontState]);
        if (newstate != 0)
            O->putf("<%s>", NBModeToFace[newstate]);
        S->FontState = newstate;
    }
}

void ProcessCarriageReturn () {
    int c = I->getc();
    if (c == '\n') {
        c = I->getc();
        if (c == '\r') {
            c = I->getc();
            if (c == '\n') {
                O->putf("\n<p>\n");
                return;
            }
            O->putf("\n<br/>");
        }
        else if (c == '\n') {
            O->putf("\n<p>\n");
        }
        else
            O->putf("\n<br/>");
    }
    else
        //        O->putf("\r", c);
        O->putc('\r');
    I->ungetc(c);
}

void ProcessLineFeed () {
    int c = I->getc();
    if (c == '\n') {
        O->putf("</p>\n\n<p>");
        return;
    }
    O->putc(' ');
    I->ungetc(c);
}

void ProcessSingleQuote(char unsigned actual_char) {
    int c2 = I->getc();
    if (actual_char == c2)
        switch(actual_char) {
            case '`': O->puts("“"); return;
            case '\'': O->puts("”"); return;
    }
    O->putc(actual_char);
    I->ungetc(c2);
}



void HTML437Putc(char c) {
    switch (c) {
        case '>': O->puts("&gt;"); return;
        case '<': O->puts("&lt;"); return;
        case '&': O->puts("&amp;"); return;
        default: O->puts(CP437CharToUTF8CharStar((CU)c)); return;
    }
}

void ProcessRuboutEscape () {
    CU cu = HexDigit ((CU)I->getc()) << 4;
    cu += HexDigit ((CU)I->getc());
    switch (cu) {
        case CP437_OPEN_GUILLEMOT:
            O->putf("%s", OpenQuotes[S->Quotran]); break;
        case CP437_CLOSE_GUILLEMOT:
            O->putf("%s", CloseQuotes[S->Quotran]); break;
        default:
            HTML437Putc(cu); break;
    };
}

void Process207() {
    CU dig1 = HexDigit(I->getc());
    CU dig2 = HexDigit(I->getc());
    CU c = (dig1 << 4) | dig2;
    HTML437Putc(c);
}

void ProcessSpace() {
    if (S->NoBreak)
        O->puts("&nbsp;");
    else
        O->putc (' ');
}

void run_charmac (char c) {
    StringInputSource SIS(CharMacs[(CU)c]);
    run_pushed_source(&SIS);
}

void outsnout (char ender) {

    while (true) {
        int c = I->getc();
        if (c == EOF || c == ender || c == CP437_CONTROL_Z)
            break;
        c = c & 0xFF;
        switch (c) {
            case CP437_INVERSE_SMILEY:
                ProcessNSmiley ();
                break;
            case ' ':
                ProcessSpace();
                break;
            case RUBOUT_ESC:
                ProcessRuboutEscape();
                break;
            case '\r':
                ProcessCarriageReturn ();
                break;
            case '\n':
                ProcessLineFeed ();
                break;
            case CP437_OPEN_GUILLEMOT:
                ProcessNotaBene ();
                break;
            case 0207:
                Process207 ();
                break;
            case '\'':
            case '`':
                ProcessSingleQuote(c);
                break;
            default:
                if (CharMacs.count((CU)c))
                    run_charmac(c);
                else
                    HTML437Putc(c);
                break;
        }
    }
}

time_t NowAsTime_t () {
    auto now = std::chrono::system_clock::now();
    return std::chrono::system_clock::to_time_t(now);
}

string CTime(time_t t) {
    string s = std::ctime(&t);
    if (s[s.length()-1] == '\n')
        s = s.substr(0, s.length()-1);
    return s;
}

void OutputDateTime () {
    O->puts(CTime(NowAsTime_t()));
}

string FileMTime(fs::path path) {
    struct stat buff;   //C++ stuff doesn't work.
    if (stat(path.c_str(), &buff) != 0)
        return "";
    return ", modified " + CTime(buff.st_mtime);
}

string ExeMTime;

void GenHTMLFileReport(fs::path path) {
    O->putf("<b>File %s%s</b><br/>", path.c_str(), FileMTime (path).c_str());
    O->putf("<b>Outsnouted by tjhtml%s, at %s</b><br/>", ExeMTime.c_str(), CTime(NowAsTime_t()).c_str());
}

void CSSReport(fs::path path) {
    O->putf("<b>Incorporated CSS %s%s</b><br/>", path.c_str(), FileMTime (path).c_str());
}

void fabort(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(3);
}

fs::path default_outpath(fs::path inpath) {
    fs::path outpath = fs::path(inpath).replace_extension(".html");
    if (outpath.has_parent_path()) {
        fs::path parent = outpath.parent_path();
        auto dperms = fs::status(parent).permissions();
        if ((dperms & fs::perms::owner_write) == fs::perms::none){
            printf("Directory %s not writable, directing to working dir.\n",
                   fs::absolute(parent).c_str());
            outpath = outpath.filename();
        }
    }
    printf("Output to %s\n", fs::absolute(outpath).c_str());
    return outpath;
}

static string requote(const string& s) {
    /* Note that SPACE and TAB are NOT THERE; as we are requoting, they must not be. */
    static const unordered_set<char> EvilChars = {'\\', '\'', '(', ')', '"', '*', '?'};
    string out;
    for (auto c : s) {
        if (EvilChars.count(c))
            out += '\\';
        out += c;
    }
    out.insert(0, "\"");
    out += '"';
    return out;
}

int main (int argc, const char* argv[]) {
    ExeMTime = FileMTime(argv[0]);
    argparse::ArgSet arg_set
    ("Great Snout Device TJ(oo) to HTML.",
     {  {"inpath", "help=TJ(oo) document source file path."},
        {"outpath", "optional=", "help=Optional output HTML path."},
        {"--open", "-o", "optional=", "boolean=", "help=Open output file in system browser."},
        {"--css", "-c", "metavar=path", "help=CSS file to copy into output."},
        {"--view", "-v", "boolean=", "help=View source on console in Unicode"}
    });

    auto args = arg_set.Parse(argc, argv);

    fs::path outpath, inpath = fs::absolute(fs::path(args["inpath"]));
    if (!fs::exists(inpath))
        fabort("Input file %s does not exist.\n", inpath.c_str());

    if (args["view"]) {
        printf("%s\n", CP437toUTF8String(FileInputSource(inpath).read_all()).c_str());
        exit(0);
    }

    string css;
    if (args["css"])
        css = FileInputSource(args["css"]).read_all();
    if (args["outpath"])
        outpath = fs::path(args["outpath"]);
    else
        outpath = default_outpath(inpath);
   
    FileInputSource FIS(inpath); // checks and bombs if fail. has dtor.
    FileOutputSink FOS(outpath);
    StringOutputSink FootOS;

    I = &FIS;
    O = &FOS;
    FootnoteSink = &FootOS;
    S = &MainState;
    O->puts(HTML_HEADER1);
    if (css.length())
        O->puts("<style>\n" + css + "</style>\n");
    O->puts(HTML_HEADER2);
    GenHTMLFileReport(inpath);
    if (css.length())
        CSSReport(args["css"]);
    outsnout (CONTROL_Z);
    column_dumpfinish();
    if (FootOS.gravid()) {
        O->puts("<hr><h2>Footnotes</h2>\n");
        fndump();
    }
    O->puts("</body>\n</html>\n");
    FOS.close();
    if (args["open"]) {
        string cmd("open ");
        cmd += requote(outpath.string());
        std::system(cmd.c_str());
    }
    return 0;
}
