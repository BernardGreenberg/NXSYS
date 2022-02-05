#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "parsargs.h"

int ParseArgsArgCount (char ** argv) {
    int i = 0;
    if (argv == NULL)
	return 0;			/* not really cool ... */
    for (i = 0; argv[i] != NULL; i++);
    return i;
}

void ParseArgsFree (char ** argv) {
    if (argv != NULL) {
	int i = ParseArgsArgCount (argv);
	free (argv[i+1]);
	free (argv);
    }
}

static void pack_arg (char * p ) {
    char * q = p;
m1:
    while (!isspace (*q) && *q != '\0' &&*q != '\"')
	*p++ = *q++;
    if (*q == '\"') {
	q++;
m2:	    while (*q != '\\' && *q != '\0' &&*q != '\"')
		*p++ = *q++;
	    if (*q == '\\') {
		q++;
		if (*q == '\0')
		    goto m1;
		*p++ = *q++;
		goto m2;
	    }
	    else if (*q == '\"') {
		q++;
		goto m1;
	    }
    }
    *p++ = '\0';
}
    
static char * find_arg_end (char * q, int * has_quotes) {
    *has_quotes = 0;
m1:
    while (!isspace (*q) && *q != '\0' &&*q != '\"') q++;
    if (*q == '\"') {
	*has_quotes = 1;
	q++;
m2:	while (*q != '\\' && *q != '\0' &&*q != '\"') q++;
	if (*q == '\\') {
	    q++;
	    if (*q == '\0')
		goto m1;
	    q++;
	    goto m2;
	}
	else if (*q == '\"') {
	    q++;
	    goto m1;
	}
    }
    return q;
}	



char ** ParseArgString (char * arg) {
    char * p, *p0, *q, *r;
    char ** argv;
    int args = 0, ano;
    int have_quotes;
    p0 = p = _strdup (arg);
    if (p == NULL)
	return NULL;
    r = p;
    for (;; p = q) {
	while (isspace (*p)) p++;
	if (*p == '\0') break;
	q = find_arg_end (p, &have_quotes);
	args++;
    }
    argv = (char **) malloc ((args + 2) * sizeof (char *));
    if (argv == NULL) {
	free (p0);
	return NULL;
    }
    p = r;
    ano = 0;
    for (;ano < args; p = q) {
	while (isspace (*p)) p++;
	if (*p == '\0') break;
	q = find_arg_end (p, &have_quotes);
	if (have_quotes)
	    pack_arg (p);
	*q++ = '\0';
	argv[ano++] = p;
    }
    argv[ano] = NULL;
    argv[ano+1] = p0;
    return argv;
}

#ifdef TEST
#include <stdio.h>

void main () {
    char buf [256];
    char ** args;
    int i, count;

    printf ("Type line to be parsed, ENTER:\n");
    gets(buf);
    
    args = ParseArgString (buf);
    count = ParseArgsArgCount (args);
    printf ("Count = %d\n", count);
    for (i = 0; i < count; i++)
	printf ("%2d %3d: %s\n", i+1, strlen(args[i]), args[i]);
    ParseArgsFree (args);
}
#endif
