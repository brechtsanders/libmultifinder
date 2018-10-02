/*
Copyright (c) 2018 Brecht Sanders

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <string.h>
#include "multifinder.h"

#if defined(_MSC_VER) || (defined(__MINGW32__) && !defined(__MINGW64__))
#define strncasecmp _strnicmp
#endif

struct multifinder_pattern_list {
  char* data;                                           //pattern
  size_t datalen;                                       //length of pattern
  int (*strncmp_fn)(const char*, const char*, size_t);  //compare function
  void* callbackdata;                                   //user callback data
  struct multifinder_pattern_list* next;                //next entry in linked list
};

struct multifinder_struct {
  struct multifinder_pattern_list* patterns;    //user callback function called on pattern match
  multifinder_found_callback_fn foundfunction;  //user callback function called for each pattern match
  multifinder_flush_callback_fn flushfunction;  //user callback function called for data without pattern match
  void* callbackdata;                           //user callback data
  //size_t shortestpattern;                       //length of shortest pattern
  size_t longestpattern;                        //length of longest pattern
  size_t streampos;                             //position in input stream (only updated at end of multifinder_process())
  size_t flushedpos;                            //number of input stream bytes that have been sent processed (by multifinder_found_callback_fn or multifinder_found_callback_fn)
  int abortstatus;                              //when non-zero a callback functions requested to abort
  char* buf;                                    //buffer containing data that comes before data currently being processed
  size_t buflen;                                //current length of buf
};

DLL_EXPORT_MULTIFINDER void multifinder_get_version (int* pmajor, int* pminor, int* pmicro)
{
  if (pmajor)
    *pmajor = MULTIFINDER_VERSION_MAJOR;
  if (pminor)
    *pminor = MULTIFINDER_VERSION_MINOR;
  if (pmicro)
    *pmicro = MULTIFINDER_VERSION_MICRO;
}

DLL_EXPORT_MULTIFINDER const char* multifinder_get_version_string ()
{
  return MULTIFINDER_VERSION_STRING;
}

DLL_EXPORT_MULTIFINDER multifinder multifinder_create (multifinder_found_callback_fn foundfunction, multifinder_flush_callback_fn flushfunction, void* callbackdata)
{
  struct multifinder_struct* result;
  if ((result = (struct multifinder_struct*)malloc(sizeof(struct multifinder_struct))) != NULL) {
    result->patterns = NULL;
    result->foundfunction = foundfunction;
    result->flushfunction = flushfunction;
    result->callbackdata = callbackdata;
    //result->shortestpattern = 0;
    result->longestpattern = 0;
    result->streampos = 0;
    result->flushedpos = 0;
    result->abortstatus = 0;
    result->buf = NULL;
    result->buflen = 0;
  }
  return result;
}

DLL_EXPORT_MULTIFINDER void multifinder_free (multifinder handle)
{
  if (handle) {
    struct multifinder_pattern_list* current;
    struct multifinder_pattern_list* next;
    current = handle->patterns;
    while (current) {
      next = current->next;
      free(current->data);
      free(current);
      current = next;
    }
    if(handle->buf)
      free(handle->buf);
    free(handle);
  }
}

DLL_EXPORT_MULTIFINDER void multifinder_reset (multifinder handle)
{
  if (handle) {
    handle->streampos = 0;
    handle->flushedpos = 0;
    handle->abortstatus = 0;
    if(handle->buf)
      free(handle->buf);
    handle->buf = NULL;
    handle->buflen = 0;
  }
}

DLL_EXPORT_MULTIFINDER void multifinder_add_pattern (multifinder handle, const char* pattern, unsigned int flags, void* patterncallbackdata)
{
  if (pattern && *pattern) {
    char* s;
    size_t len = strlen(pattern);
    if ((s = (char*)malloc(len)) != NULL) {
      memcpy(s, pattern, len);
      multifinder_add_allocated_pattern(handle, s, len, flags, patterncallbackdata);
    }
  }
}

DLL_EXPORT_MULTIFINDER void multifinder_add_allocated_pattern (multifinder handle, char* pattern, size_t patternlen, unsigned int flags, void* patterncallbackdata)
{
  struct multifinder_pattern_list* entry;
  struct multifinder_pattern_list** last;
  //create new entry
  entry = (struct multifinder_pattern_list*)malloc(sizeof(struct multifinder_pattern_list));
  entry->data = pattern;
  entry->datalen = patternlen;
  entry->strncmp_fn = (flags & MULTIFIND_PATTERN_CASE_INSENSITIVE ? &strncasecmp : &strncmp);
  entry->callbackdata = patterncallbackdata;
  entry->next = NULL;
  //add after last entry
  last = &(handle->patterns);
  while (*last) {
    last = &((*last)->next);
  }
  *last = entry;
  //update values
  //if (handle->shortestpattern == 0 || patternlen < handle->shortestpattern)
  //  handle->shortestpattern = patternlen;
  if (patternlen > handle->longestpattern)
    handle->longestpattern = patternlen;
}

DLL_EXPORT_MULTIFINDER size_t multifinder_count_patterns (multifinder handle)
{
  size_t count = 0;
  struct multifinder_pattern_list* p = handle->patterns;
  while (p) {
    count++;
    p = p->next;
  }
  return count;
}

int compare_across_2_buffers (int (*strncmp_fn)(const char*, const char*, size_t), const char* s1, size_t s1len, const char* s2a, size_t s2alen, const char* s2b, size_t s2blen)
{
  int result;
/*
  //check if the 2 buffers are long enough to compare with
  if (s1len > s2alen + s2blen)
    return 1;
*/
  //if first buffer is larger than s1 only compare for the length of s1
  if (s2alen > s1len) {
    s2alen = s1len;
    s2blen = 0;
  }
  //compare with first buffer
  if ((result = (s2alen > 0 ? (*strncmp_fn)(s1, s2a, s2alen) : 0)) == 0) {
    if (s2alen + s2blen > s1len)
      s2blen = s1len - s2alen;
    //compare with second buffer
    result = (s2blen > 0 ? (*strncmp_fn)(s1 + s2alen, s2b, s2blen) : 0);
  }
  return result;
}

size_t flush_data (multifinder handle, size_t flushpos, const char* data)
{
  if (flushpos > handle->flushedpos) {
    size_t flushlen = flushpos - handle->flushedpos;
    if (!handle->flushfunction) {
      handle->flushedpos = flushpos;
    } else {
      size_t flushremaining = flushlen;
      //flush buffer first if needed
      if (handle->flushedpos < handle->streampos) {
        size_t bufstartpos = (handle->flushedpos > (handle->streampos - handle->buflen) ? handle->flushedpos - (handle->streampos - handle->buflen) : 0);
        size_t bufflushlen = (bufstartpos + flushremaining <= handle->buflen ? flushremaining : handle->buflen - bufstartpos);
        if (bufflushlen > 0) {
          (*(handle->flushfunction))(handle->buf + bufstartpos, bufflushlen, handle->callbackdata);
          handle->flushedpos += bufflushlen;
          flushremaining -= bufflushlen;
        }
      }
      //flush data up to position
      if (flushremaining > 0 && flushpos > handle->streampos) {
        size_t datastartpos = (handle->flushedpos > handle->streampos ? handle->flushedpos - handle->streampos : 0);
        (*(handle->flushfunction))(data + datastartpos, flushremaining, handle->callbackdata);
        handle->flushedpos += flushremaining;
      }
    }
  }
  return flushpos;
}

DLL_EXPORT_MULTIFINDER size_t multifinder_process (multifinder handle, const char* data, size_t datalen)
{
  size_t count = 0;
  if (handle->abortstatus == 0) {
    struct multifinder_pattern_list* pattern;
#if 0
    size_t i = 0;
    size_t j = 0;
    //scan buffered data in combination with supplied data
    while (handle->buflen - i + datalen - j >= handle->longestpattern) {
      pattern = handle->patterns;
      while (pattern) {
        if (compare_across_2_buffers(pattern->strncmp_fn, pattern->data, pattern->datalen, handle->buf + i, handle->buflen - i, data + j, datalen - j) == 0) {
          //match found
          count++;
          //flush data
          flush_data(handle, handle->streampos - handle->buflen + i + j, data);
          //call callback
          if (handle->foundfunction && (handle->abortstatus = (*handle->foundfunction)(handle, pattern->datalen, pattern->callbackdata, handle->callbackdata)) != 0) {
            //abort when requested by callback function
            handle->streampos += datalen;
            return count;
          }
          handle->flushedpos += pattern->datalen;
          if ((i += pattern->datalen - 1) > handle->buflen) {
            //all of buffer processed
            //flush data
            //flush_data(handle, handle->streampos, data);
            //point to supplied data
            j += (i - handle->buflen);
            i = handle->buflen;
          }
          break;
        }
        pattern = pattern->next;
      }
      if (i < handle->buflen)
        i++;
      else if (++j >= datalen)
        break;
    }
#else
    size_t i = 0;
    //scan buffered data in combination with supplied data
    //while (i < handle->buflen && handle->buflen - i + datalen >= handle->longestpattern) {
    while (i < handle->buflen && i - handle->buflen + handle->longestpattern <= datalen) {
      pattern = handle->patterns;
      while (pattern) {
        if (compare_across_2_buffers(pattern->strncmp_fn, pattern->data, pattern->datalen, handle->buf + i, handle->buflen - i, data, datalen) == 0) {
          //match found
          count++;
          //flush data
          flush_data(handle, handle->streampos - handle->buflen + i, data);
          //call callback
          if (handle->foundfunction && (handle->abortstatus = (*handle->foundfunction)(handle, pattern->datalen, pattern->callbackdata, handle->callbackdata)) != 0) {
            //abort when requested by callback function
            handle->streampos += datalen;
            return count;
          }
          handle->flushedpos += pattern->datalen;
          i += pattern->datalen - 1;
          break;
        }
        pattern = pattern->next;
      }
      i++;
    }
    //flush data
    //flush_data(handle, handle->streampos, data);
    //scan rest of supplied data
    i -= handle->buflen;
    //while (datalen - i >= handle->longestpattern) {
    while (i + handle->longestpattern <= datalen) {
      pattern = handle->patterns;
      while (pattern) {
        if ((*(pattern->strncmp_fn))(pattern->data, data + i, pattern->datalen) == 0) {
          //match found
          count++;
          //flush data
          flush_data(handle, handle->streampos + i, data);
          //call callback
          if (handle->foundfunction && (handle->abortstatus = (*handle->foundfunction)(handle, pattern->datalen, pattern->callbackdata, handle->callbackdata)) != 0) {
            //abort when requested by callback function
            handle->streampos += datalen;
            return count;
          }
          handle->flushedpos += pattern->datalen;
          i += pattern->datalen - 1;
          break;
        }
        pattern = pattern->next;
      }
      i++;
    }
#endif
    //if buffer is not allocated yet allocate it with the size of the longest pattern
    size_t bufsize = handle->longestpattern - 1;
    if (!handle->buf)
      handle->buf = (char*)malloc(bufsize);
    //keep data in buffer for the next time
    if (handle->buflen + datalen <= bufsize) {
      //existing + supplied data is shorter than longest pattern => append all supplied data to buffer
      memcpy(handle->buf + handle->buflen, data, datalen);
      handle->buflen += datalen;
    } else {
      //existing + supplied data is longer than longest pattern => keep what is needed of buffer and append data needed
      if (datalen >= bufsize) {
        //supplied data is longer than longest pattern
        //flush data
        flush_data(handle, handle->streampos + datalen - bufsize, data);
        //copy from end of supplied data
        memcpy(handle->buf, data + datalen - bufsize, bufsize);
      } else {
        //supplied data is shorter than longest pattern
        size_t bufreuselen = bufsize - datalen;
        //flush data
        if (handle->flushedpos < handle->streampos - bufreuselen)
          flush_data(handle, handle->streampos - bufreuselen, data);
        //keep needed part of buffer add supplied data
        memmove(handle->buf, handle->buf + handle->buflen - bufreuselen, bufreuselen);
        memcpy(handle->buf + bufreuselen, data, datalen);
      }
      handle->buflen = bufsize;
    }
  }
  handle->streampos += datalen;
  return count;
}

DLL_EXPORT_MULTIFINDER size_t multifinder_finalize (multifinder handle)
{
  struct multifinder_pattern_list* pattern;
  size_t count = 0;
  size_t i = 0;
  while (i < handle->buflen) {
    pattern = handle->patterns;
    while (pattern) {
      if (i <= handle->buflen - pattern->datalen) {
        if ((*(pattern->strncmp_fn))(handle->buf + i, pattern->data, pattern->datalen) == 0) {
          //match found
          count++;
          //flush data
          flush_data(handle, handle->streampos - handle->buflen + i, NULL);
          //call callback
          if (handle->foundfunction && (*handle->foundfunction)(handle, pattern->datalen, pattern->callbackdata, handle->callbackdata) != 0) {
            return count;
          }
          handle->flushedpos += pattern->datalen;
          i += count;
          continue;
        }
      }
      pattern = pattern->next;
    }
    i++;
  }
  flush_data(handle, handle->streampos, NULL);
  if (handle->flushfunction)
    (*(handle->flushfunction))(NULL, 0, handle->callbackdata);
  return count;
}

DLL_EXPORT_MULTIFINDER int multifinder_aborted (multifinder handle)
{
  return handle->abortstatus;
}

DLL_EXPORT_MULTIFINDER size_t multifinder_position (multifinder handle)
{
  return handle->flushedpos;
}
