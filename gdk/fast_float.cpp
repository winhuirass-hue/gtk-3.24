#include "fast_float.h"
#include <cstring>

extern "C" {

double
strtod_fast (const char  *first,
             const char  *last,
             const char **endp)
{
  fast_float::from_chars_result res;
  double value;

  res = fast_float::from_chars (first, last, value);

  if (res.ec == std::errc{})
    {
      *endp = res.ptr;
      return value;
    }
  else
    {
      *endp = first;
      return 0;
    }
}

}
