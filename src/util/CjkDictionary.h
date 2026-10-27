#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "Dictionary.h"

// Bridge between the reader's StarDict lookup flow and the segmentation-ready
// CJK dictionary packs in lib/Dict (the Matcha Reader vocab.idx/.dat/.spx
// format, built from CC-CEDICT with pinyin, frequency ranks, HSK/TOCFL levels
// and example sentences by tools/dict_convert/convert_jmdict.py).
//
// A Mandarin pack lives in /.dictionaries/zh/ or /dictionaries/zh/. When one is
// installed, Han lookups go to it first and fall back to the configured StarDict
// dictionary. The matched entry is formatted as simple HTML into a tiny scratch
// StarDict entry under /.crosspoint/cjkdict/, so the existing definition view,
// paging, nested lookup, history and clipping flows all work unchanged.
namespace CjkDictionary {

// Language folder of the Mandarin pack (also accepted for Traditional packs:
// every entry is indexed under both its simplified and traditional forms).
constexpr const char* kMandarinFolder = "zh";

// True when a Mandarin pack (vocab.idx + vocab.dat) is on the SD card. The
// result is cached until releaseCaches(), so render paths can call it freely.
bool available();

// True when `word` contains a Han ideograph and should be tried in the pack.
bool wantsWord(const std::string& word);

// Look `word` up in the pack. Only an exact headword matches unless
// `prefixNoticeFormat` is given; then a miss falls back to the longest dictionary
// word at the start of `word` (selecting 我们的 finds 我们), and the entry opens
// with that notice, formatted with the selection and the headword shown (two %s).
// On success the entry is written to the scratch StarDict file and the returned
// location points at it. readError is set only for SD failures.
// When `vocab` is given, a found entry is also staged for the learner's
// vocabulary log (see VocabLog.h) with that sentence and book; it is written
// only if the learner presses Save in the definition popup.
struct VocabContext {
  std::string sentenceHtml;  // sentence around the selection, word in <b></b>
  std::string book;
};
DictLocation locate(const std::string& word, const VocabContext* vocab = nullptr,
                    const char* prefixNoticeFormat = nullptr);

// Longest-first dictionary match lengths at `byteOffset` in `text`, used by the
// word-select segmenter. Bit L-1 of the result is set when the L-character window
// is a headword; priority[L-1] carries its frequency rank (255 = most common).
// No definitions are read. Returns 0 when no pack is installed.
constexpr int kMaxWordChars = 8;
uint8_t matchLengthsAt(const std::string& text, size_t byteOffset, uint8_t priority[kMaxWordChars]);

// Free the pack's index caches (~30 KB) and forget the availability probe. Call
// when a lookup session ends so the reader gets its heap back.
void releaseCaches();

// Convert one pack definition (Matcha's plain layout: 【reading】, [level] tags,
// "→ note" lines, "• gloss" bullets, example lines, entries joined by "\n\n---\n")
// into the HTML subset DictHtmlRenderer draws. Exposed for host tests.
std::string formatEntryHtml(const std::string& headword, const std::string& definition);

// One tab-separated vocabulary row (no trailing newline): headword, reading,
// up to three glosses joined by "; ", sentence, book, level tags. Tabs and
// newlines inside fields become spaces. Exposed for host tests.
std::string formatVocabRow(const std::string& headword, const std::string& definition, const std::string& sentenceHtml,
                           const std::string& book);

}  // namespace CjkDictionary
