#include "helpers.h"

String truncate(const String &s, u_int maxLen) {
  if (maxLen < 0) return "";
  if (s.length() <= maxLen) return s;
  String out = s.substring(0, maxLen) + ".";
  return out;
}
