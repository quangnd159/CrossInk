#include "CjkSegmenter.h"

#include <Logging.h>
#include <Memory.h>
#include <Utf8.h>

#include <algorithm>
#include <climits>
#include <cstring>
#include <string>
#include <vector>

#ifndef CJK_SEGMENTER_NO_NAVIGATOR
#include "WordSelectNavigator.h"
#endif

bool CjkSegmenter::ensureScratch() {
  if (!scratch_) scratch_ = makeUniqueNoThrow<Scratch>();
  if (!scratch_) LOG_ERR("CJKS", "OOM: segmenter scratch");
  return scratch_ != nullptr;
}

bool CjkSegmenter::segmentCells(const std::string& text, const uint16_t* byteAt, const int cellCount, uint8_t* lens) {
  for (int i = 0; i < cellCount; i++) lens[i] = 1;
  if (cellCount <= 0) return true;
  if (!ensureScratch()) return false;
  int p = 0;
  while (p < cellCount) {
    p += std::max(segmentChunk(text, byteAt, cellCount, p, lens), 1);
  }
  return true;
}

int CjkSegmenter::segmentChunk(const std::string& text, const uint16_t* byteAt, const int cellCount, const int first,
                               uint8_t* lens) {
  Scratch& s = *scratch_;
  const int n = std::min(kChunkCells, cellCount - first);
  // Probe windows (<= kMaxWord characters) may run past the chunk into the rest
  // of the run, so a word starting near the chunk end still sees its whole window.
  for (int j = 0; j < n; j++) {
    std::memset(s.priority[j], 0, sizeof(s.priority[j]));
    s.found[j] = CjkDictionary::matchLengthsAt(text, byteAt[first + j], s.priority[j]);
  }

  // Unigram DP (Matcha Reader): priority is 255 - 28*log10(rank), so 255 - priority
  // is log(rank) at 12 units per e-fold. kWordCost adds the -log P of the commonest
  // word (的, ~5% of text: 3 e-folds) so two common characters do not always beat the
  // word they form. Without frequency data every word costs the same and the fewest
  // words win: longest match. An uncovered cell costs more than any word.
  constexpr int16_t kWordCost = -(255 + 36);
  constexpr int16_t kUnknownCost = -400;
  s.best[n] = 0;
  for (int p = n - 1; p >= 0; p--) {
    int16_t best = INT16_MIN;
    uint8_t bestLen = 0;
    for (int len = 1; len <= kMaxWord; len++) {
      if (!(s.found[p] & (1u << (len - 1)))) continue;
      // A word may overrun the chunk end; it then ends the chunk as one word.
      const int next = std::min(n, p + len);
      const int16_t score = static_cast<int16_t>(s.best[next] + kWordCost + s.priority[p][len - 1]);
      if (score > best || (score == best && len > bestLen)) {
        best = score;
        bestLen = static_cast<uint8_t>(len);
      }
    }
    if (best == INT16_MIN) {
      best = static_cast<int16_t>(s.best[p + 1] + kUnknownCost);
      bestLen = 0;
    }
    s.best[p] = best;
    s.bestLen[p] = bestLen;
  }

  int p = 0;
  while (p < n) {
    const int len = std::min<int>(std::max<int>(s.bestLen[p], 1), cellCount - (first + p));
    lens[first + p] = static_cast<uint8_t>(len);
    for (int k = 1; k < len; k++) lens[first + p + k] = 0;
    p += len;
  }
  return p;
}

#ifndef CJK_SEGMENTER_NO_NAVIGATOR
namespace {
// The layout keeps CJK punctuation on the character it may not be separated
// from ("们，", "“我"), so a run cell is one Han character optionally wrapped in
// punctuation. Punctuation on either side also ends the run there.
struct RunCell {
  bool valid = false;
  bool leadingPunct = false;
  bool trailingPunct = false;
  size_t hanOffset = 0;
  size_t hanLen = 0;
};

bool isPunctuation(const uint32_t cp) {
  if (cp < 0x80) return !((cp >= '0' && cp <= '9') || (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z'));
  return !utf8IsHan(cp) && !utf8IsCjkWordCharacter(cp) &&
         (utf8IsCjkCodepoint(cp) || (cp >= 0x2000 && cp <= 0x2BFF) || (cp >= 0xFE10 && cp <= 0xFE6F) ||
          (cp >= 0xFF01 && cp <= 0xFFEE));
}

RunCell parseRunCell(const char* token) {
  RunCell cell;
  const auto* begin = reinterpret_cast<const unsigned char*>(token);
  const auto* ptr = begin;
  int han = 0;
  while (*ptr) {
    const auto* start = ptr;
    const uint32_t cp = utf8NextCodepoint(&ptr);
    if (utf8IsHan(cp)) {
      if (++han > 1) return {};
      cell.hanOffset = static_cast<size_t>(start - begin);
      cell.hanLen = static_cast<size_t>(ptr - start);
    } else if (isPunctuation(cp)) {
      (han == 0 ? cell.leadingPunct : cell.trailingPunct) = true;
    } else {
      return {};
    }
  }
  cell.valid = han == 1;
  return cell;
}

RunCell runCellAt(const WordSelectNavigator& nav, const int idx) {
  const auto* w = nav.getWordAt(idx);
  return w ? parseRunCell(nav.getLookup(*w)) : RunCell{};
}

// True when run cell idx continues the run of cell idx - 1.
bool continuesRun(const WordSelectNavigator& nav, const int idx) {
  const RunCell prev = runCellAt(nav, idx - 1);
  const RunCell cur = runCellAt(nav, idx);
  return prev.valid && cur.valid && !prev.trailingPunct && !cur.leadingPunct && nav.joinsPreviousWithoutSpace(idx);
}
}  // namespace

bool CjkSegmenter::isRunCell(const WordSelectNavigator& nav, const int idx) { return runCellAt(nav, idx).valid; }

void CjkSegmenter::segmentRunAt(const WordSelectNavigator& nav, uint8_t* spans, const int idx) {
  if (!spans || !nav.getWordAt(idx)) return;
  if (!isRunCell(nav, idx)) {
    spans[idx] = 1;
    return;
  }
  int first = idx;
  while (first > 0 && continuesRun(nav, first)) first--;
  int last = idx;
  while (nav.getWordAt(last + 1) && continuesRun(nav, last + 1)) last++;
  const int count = last - first + 1;

  // Transient per-run buffers: a run is one clause between punctuation marks,
  // typically under 30 characters, and is freed before the cursor redraws.
  // Only the Han character of each cell is probed; attached punctuation is not.
  std::string text;
  std::vector<uint16_t> byteAt;
  text.reserve(static_cast<size_t>(count) * 3);
  byteAt.reserve(static_cast<size_t>(count) + 1);
  for (int i = first; i <= last; i++) {
    const RunCell cell = runCellAt(nav, i);
    byteAt.push_back(static_cast<uint16_t>(text.size()));
    text.append(nav.getLookup(*nav.getWordAt(i)) + cell.hanOffset, cell.hanLen);
  }
  byteAt.push_back(static_cast<uint16_t>(text.size()));
  segmentCells(text, byteAt.data(), count, spans + first);
}
#endif
