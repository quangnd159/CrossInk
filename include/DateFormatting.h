#pragma once

#include <cstddef>
#include <cstdint>

// Format a known date using translated month names and the persisted date-format values.
bool formatDateParts(char* buf, size_t bufSize, uint16_t year, uint8_t month, uint8_t day, uint8_t dateFormat,
                     char numericSeparator = '/');

// Short translated month name ("Oct", "10月"), or "-" when month is out of range.
const char* shortMonthName(uint8_t month);

// Compact month and day for stats ("Oct 3", "10月3日").
bool formatShortMonthDay(char* buf, size_t bufSize, uint8_t month, uint8_t day);

// Translated AM/PM marker for 12-hour clocks.
const char* meridiemLabel(bool pm);
