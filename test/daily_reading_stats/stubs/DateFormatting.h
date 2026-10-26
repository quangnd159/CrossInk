#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

// English-only stand-ins for lib/I18n/DateFormatting.cpp.
inline const char* shortMonthName(const uint8_t month) {
  static constexpr const char* kNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return month >= 1 && month <= 12 ? kNames[month - 1] : "-";
}

inline bool formatShortMonthDay(char* buf, const size_t bufSize, const uint8_t month, const uint8_t day) {
  if (bufSize == 0 || month < 1 || month > 12) return false;
  const int length = std::snprintf(buf, bufSize, "%s %u", shortMonthName(month), day);
  return length >= 0 && static_cast<size_t>(length) < bufSize;
}
