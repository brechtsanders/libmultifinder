#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "multifinder.h"

#define READBUFFERSIZE 128

int whenfound (multifinder handle, size_t length, void* patterncallbackdata, void* callbackdata)
{
  fprintf(*(FILE**)callbackdata, "%s", (char*)patterncallbackdata);
  return 0;
}

void flushsearchdata (const char* data, size_t datalen, void* callbackdata)
{
  if (datalen)
    fprintf(*(FILE**)callbackdata, "%.*s", (int)datalen, data);
}

void show_help()
{
  printf(
    "Usage:  multifinder_replace [-?|-h] [-c] [-i] [-f file] [-t text] [-p <pattern> <replacement>] <pattern> <replacement> ...\n" \
    "Parameters:\n" \
    "  -? | -h     \tshow help\n" \
    "  -c          \tcase sensitive matching for next pattern(s) (default)\n" \
    "  -i          \tcase insensitive matching for next pattern(s)\n" \
    "  -f file     \tinput file (default is to use standard input)\n" \
    "  -o file     \toutput file (default is to use standard output)\n" \
    "  -v          \tprint number of replacements done\n" \
    "  -t text     \tuse text as search data (overrides -f)\n" \
    "  -p          \tnext 2 parameters are pattern and replacement (can be used if pattern or replacement starts with \"-\")\n" \
    "  pattern     \tpattern to search for\n" \
    "  replacement \treplacement to replace pattern with\n" \
    "Version: " MULTIFINDER_VERSION_STRING "\n" \
    "\n"
  );
}

int main (int argc, char** argv)
{
  multifinder finder;
  FILE* dst;
  int flags = MULTIFIND_PATTERN_CASE_SENSITIVE;
  int verbose = 0;
  const char* srcfile = NULL;
  const char* dstfile = NULL;
  const char* srctext = NULL;
  size_t count = 0;
  //initialize
  if ((finder = multifinder_create(whenfound, flushsearchdata, &dst)) == NULL) {
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
          case 'o' :
            if (argv[i][2])
              param = argv[i] + 2;
            else if (i + 1 < argc && argv[i + 1])
              param = argv[++i];
            if (!param)
              paramerror++;
            else
              dstfile = param;
            break;
          case 'v' :
            if (argv[i][2])
              paramerror++;
            else
              verbose = 1;
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
            {
              const char* param2 = NULL;
              if (argv[i][2])
                param = argv[i] + 2;
              else if (i + 2 < argc && argv[i + 1] && argv[i + 2]) {
                param = argv[++i];
                param2 = argv[++i];
              }
              if (!param || !param2)
                paramerror++;
              else
                multifinder_add_pattern(finder, param, flags, (char*)param2);
              break;
            }
          default :
            paramerror++;
            break;
        }
      } else if (i + 1 < argc) {
        multifinder_add_pattern(finder, argv[i], flags, argv[i + 1]);
        i++;
      } else {
        paramerror++;
        break;
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
  if (!dstfile)
    dst = stdout;
  else
    dst = fopen(dstfile, "wb");
  if (dst == NULL) {
    fprintf(stderr, "Error opening output file: %s\n", srcfile);
    multifinder_free(finder);
    return 4;
  }
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
        fprintf(stderr, "Error opening input file: %s\n", srcfile);
        multifinder_free(finder);
        return 3;
      }
    }
    while ((buflen = fread(buf, 1, READBUFFERSIZE, src)) > 0) {
      count += multifinder_process(finder, buf, buflen);
    }
    count += multifinder_finalize(finder);
    fclose(src);
  }
  if (dst != stdout)
    fclose(dst);
  //show results
  if (verbose) {
    if (dst == stdout)
      printf("\n");
    printf("%lu matches replaced\n", (unsigned long)count);
  }
  //clean up
  multifinder_free(finder);
  return 0;
}
