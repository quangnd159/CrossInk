#pragma once

#include <string>

// Learner vocabulary log: a word the learner chooses to save from the
// definition popup is appended as one tab-separated row to a file on the SD
// card, ready to import into Anki (File > Import; the header lines set the
// separator, HTML and column names). The file is plain text, so the web file
// manager can download or delete it.
//
// Saving is deliberate: a lookup only stages its row (stage()), and nothing
// reaches the SD card until the popup's Save action calls saveStaged().
namespace VocabLog {

constexpr const char* kMandarinPath = "/vocab/mandarin.tsv";

// Append `row` (no trailing newline). Creates the file with Anki's import
// header on first use. A row whose headword repeats the previous logged
// headword is skipped (and still reported as saved), so saving one word twice
// in a row logs it once.
bool append(const char* path, const std::string& row);

// Stage the row of the definition being shown, replacing any earlier one.
// Called from the lookup worker before it reports completion.
void stage(const char* path, std::string row);

// Drop the staged row. Call before a new lookup starts.
void clearStaged();

// True while a row is staged (saved or not), i.e. the popup offers Save.
bool hasStaged();

// True once the staged row has been written.
bool stagedSaved();

// Append the staged row. True on success or when it was already saved.
bool saveStaged();

}  // namespace VocabLog
