#pragma once

#include <string>

// Learner vocabulary log: every word looked up in a language pack is appended
// as one tab-separated row to a file on the SD card, ready to import into Anki
// (File > Import; the header lines set the separator, HTML and column names).
// The file is plain text, so the web file manager can download or delete it.
namespace VocabLog {

constexpr const char* kMandarinPath = "/vocab/mandarin.tsv";

// Append `row` (no trailing newline). Creates the file with Anki's import
// header on first use. A row whose headword repeats the previous logged
// headword is skipped, so paging back and forth over one word logs it once.
bool append(const char* path, const std::string& row);

}  // namespace VocabLog
