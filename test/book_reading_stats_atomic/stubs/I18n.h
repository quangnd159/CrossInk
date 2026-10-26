#pragma once

inline constexpr int STR_STATS_LESS_THAN_MIN = 0;
inline constexpr int STR_DURATION_MINUTES_FORMAT = 1;
inline constexpr int STR_DURATION_HOURS_MINUTES_FORMAT = 2;
inline const char* tr(const int id) {
  switch (id) {
    case STR_DURATION_MINUTES_FORMAT:
      return "%lu min";
    case STR_DURATION_HOURS_MINUTES_FORMAT:
      return "%luh %lu min";
    default:
      return "< 1 min";
  }
}
