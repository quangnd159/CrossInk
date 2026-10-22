#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "CjkDictionary.h"

class WordSelectNavigator;

// Dictionary-driven word segmentation for unspaced Chinese text on a word-select
// page. The navigator holds one token per Han character; this fills its span
// table (see WordSelectNavigator::setSegmentation) one run at a time, where a
// run is consecutive Han characters with no space or punctuation between them.
//
// Each run is split by the unigram cost model Chinese segmenters use (ported
// from Matcha Reader's WordSelectionScan): a word costs -log P(word), taken from
// the pack's frequency rank, and the cheapest split of the run wins. That picks
// 和+尚未 over 和尚+未 where longest-match would not. Characters no entry covers
// stay single. Cost per run: up to 8 index probes per character, most answered
// from the pack's in-RAM sparse index.
class CjkSegmenter {
 public:
  // Allocates the fixed scratch (~0.5 KB) on first use; false on OOM.
  bool ensureScratch();
  void releaseScratch() { scratch_.reset(); }

  // True when `idx` is a single-Han-character token, i.e. part of a run.
  static bool isRunCell(const WordSelectNavigator& nav, int idx);

  // Segment the run containing `idx` into `spans` (one entry per navigator
  // word). Non-run indexes get span 1. Safe to call repeatedly.
  void segmentRunAt(const WordSelectNavigator& nav, uint8_t* spans, int idx);

  // Split a run of `cellCount` characters into words. Cell i is the UTF-8 text
  // [byteAt[i], byteAt[i+1]) of `text`. Writes lens[i] = word length in cells
  // for a word starting at i, 0 for a cell inside a word. False on OOM (every
  // cell is then left as its own word).
  bool segmentCells(const std::string& text, const uint16_t* byteAt, int cellCount, uint8_t* lens);

 private:
  static constexpr int kChunkCells = 32;
  static constexpr int kMaxWord = CjkDictionary::kMaxWordChars;

  struct Scratch {
    uint8_t found[kChunkCells];
    uint8_t priority[kChunkCells][kMaxWord];
    int16_t best[kChunkCells + 1];
    uint8_t bestLen[kChunkCells + 1];
  };
  std::unique_ptr<Scratch> scratch_;

  // Segment cells [first, first + min(kChunkCells, cellCount - first)); returns
  // the number of cells the chosen words cover (the last may overrun the chunk).
  int segmentChunk(const std::string& text, const uint16_t* byteAt, int cellCount, int first, uint8_t* lens);
};
