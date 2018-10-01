#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "multifinder.h"

#define READBUFFERSIZE 128

void show_help()
{
  printf(
    "Usage:  multifinder_count [[-?|-h] -c] [-i] [-f file] [-t text] [-p <pattern>] <pattern> ...\n" \
    "Parameters:\n" \
    "  -? | -h     \tshow help\n" \
    "  -c          \tcase sensitive matching for next pattern(s) (default)\n" \
    "  -i          \tcase insensitive matching for next pattern(s)\n" \
    "  -f file     \tinput file (default is to use standard input)\n" \
    "  -t text     \tuse text as search data (overrides -f)\n" \
    "  -p pattern  \tpattern to search for (can be used if pattern starts with \"-\")\n" \
    "  pattern     \tpattern to search for\n" \
    "Version: " MULTIFINDER_VERSION_STRING "\n" \
    "\n"
  );
}

int main (int argc, char** argv)
{
  multifinder finder;
  int flags = MULTIFIND_PATTERN_CASE_SENSITIVE;
  const char* srcfile = NULL;
  const char* srctext = NULL;
  size_t count = 0;
  //initialize
  if ((finder = multifinder_create(NULL, NULL, NULL)) == NULL) {
    fprintf(stderr, "Error in multifinder_create()\n");
    return 1;
  }
  //process command line parameters
  {
    int i = 0;
    char* param;
    int paramerror = 0;
    while (!paramerror && ++i < argc) {
      if (argv[i][0] == '-') {
        param = NULL;
        switch (tolower(argv[i][1])) {
          case '?' :
          case 'h' :
            if (argv[i][2])
              paramerror++;
            else
              show_help();
            return 0;
          case 'c' :
            if (argv[i][2])
              paramerror++;
            else
              flags = MULTIFIND_PATTERN_CASE_SENSITIVE;
            break;
          case 'i' :
            if (argv[i][2])
              paramerror++;
            else
              flags = MULTIFIND_PATTERN_CASE_INSENSITIVE;
            break;
          case 'f' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              srcfile = param;
            break;
          case 't' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              srctext = param;
            break;
          case 'p' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              multifinder_add_pattern(finder, param, flags, NULL);
            break;
          default :
            paramerror++;
            break;
        }
      } else {
        multifinder_add_pattern(finder, argv[i], flags, NULL);
      }
    }
    if (paramerror || argc <= 1) {
      if (paramerror)
        fprintf(stderr, "Invalid command line parameters\n");
      show_help();
      return 1;
    }
  }
  //process search data
  if (srctext) {
    //process supplied text
    count += multifinder_process(finder, srctext, strlen(srctext));
    count += multifinder_finalize(finder);
  } else {
    //process file (or standard input)
    FILE* src;
    char buf[READBUFFERSIZE];
    size_t buflen;
    if (!srcfile) {
      src = stdin;
    } else {
      if ((src = fopen(srcfile, "rb")) == NULL) {
        fprintf(stderr, "Error opening file: %s\n", srcfile);
        multifinder_free(finder);
        return 2;
      }
    }
    while ((buflen = fread(buf, 1, READBUFFERSIZE, src)) > 0) {
      count += multifinder_process(finder, buf, buflen);
    }
    count += multifinder_finalize(finder);
    fclose(src);
  }
  //show results
  printf("%lu matches found\n", (unsigned long)count);
  //clean up
  multifinder_free(finder);
  return 0;
}
