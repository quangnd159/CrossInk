#pragma once

// English-only stand-ins for the formatters ReadingStatsUtils pulls from I18n.
inline constexpr int STR_DURATION_LESS_THAN_MINUTE_SHORT = 0;
inline constexpr int STR_DURATION_MINUTES_SHORT_FORMAT = 1;
inline constexpr int STR_DURATION_HOURS_SHORT_FORMAT = 2;
inline constexpr int STR_DURATION_HOURS_MINUTES_SHORT_FORMAT = 3;
inline const char* tr(const int id) {
  switch (id) {
    case STR_DURATION_MINUTES_SHORT_FORMAT:
      return "%lum";
    case STR_DURATION_HOURS_SHORT_FORMAT:
      return "%luh";
    case STR_DURATION_HOURS_MINUTES_SHORT_FORMAT:
      return "%luh %lum";
    default:
      return "<1m";
  }
}
