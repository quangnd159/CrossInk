#include "CjkDictionary.h"

#include <DictIndex.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Utf8.h>
#include <WordLookup.h>

#include <algorithm>
#include <cstring>

#include "VocabLog.h"

namespace CjkDictionary {
namespace {

// Scratch StarDict entry the definition view streams from. The parent folder's
// name is what the view shows as the dictionary name, so it is the pack title.
constexpr const char* kScratchRoot = "/.crosspoint/cjkdict";

enum class Availability : uint8_t { Unknown, Present, Absent };
Availability g_availability = Availability::Unknown;

void selectMandarin() { DictIndex::setLanguageFolder(kMandarinFolder); }

void appendEscaped(std::string& out, const char* text, const size_t len) {
  for (size_t i = 0; i < len; i++) {
    const char c = text[i];
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      default:
        out += c;
        break;
    }
  }
}

bool startsWith(const char* text, const size_t len, const char* prefix) {
  const size_t prefixLen = std::strlen(prefix);
  return len >= prefixLen && std::memcmp(text, prefix, prefixLen) == 0;
}

// Folder-safe pack title for the scratch path: the definition view prints the
// folder name in its footer, and FAT rejects a handful of ASCII punctuation
// marks. UTF-8 bytes pass through, since long names are UTF-8 (Trung-Việt).
std::string scratchFolderName() {
  const char* title = DictIndex::slotTitle(DictIndex::DICT_JMDICT);
  std::string name;
  for (const char* p = title; p && *p; p++) {
    const char c = *p;
    const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ' ||
                      c == '-' || c == '_' || c == '.' || c == '(' || c == ')' || c == '+' ||
                      (static_cast<unsigned char>(c) & 0x80);
    if (safe) name += c;
  }
  while (!name.empty() && (name.back() == ' ' || name.back() == '.')) name.pop_back();
  if (name.empty()) name = "Chinese";
  return name;
}

bool writeScratchEntry(const std::string& html, std::string& stemOut) {
  const std::string folder = std::string(kScratchRoot) + "/" + scratchFolderName();
  if (!Storage.ensureDirectoryExists(folder.c_str())) {
    LOG_ERR("CJKD", "Cannot create %s", folder.c_str());
    return false;
  }
  stemOut = folder + "/entry";

  HalFile ifo;
  if (!Storage.openFileForWrite("CJKD", stemOut + ".ifo", ifo)) {
    LOG_ERR("CJKD", "Cannot write scratch .ifo");
    return false;
  }
  static constexpr char kIfo[] = "StarDict's dict ifo file\nversion=2.4.2\nsametypesequence=h\n";
  const bool ifoOk = ifo.write(reinterpret_cast<const uint8_t*>(kIfo), sizeof(kIfo) - 1) == sizeof(kIfo) - 1;
  ifo.close();

  HalFile dict;
  if (!ifoOk || !Storage.openFileForWrite("CJKD", stemOut + ".dict", dict)) {
    LOG_ERR("CJKD", "Cannot write scratch .dict");
    return false;
  }
  const size_t written = dict.write(reinterpret_cast<const uint8_t*>(html.data()), html.size());
  dict.close();
  if (written != html.size()) {
    LOG_ERR("CJKD", "Short scratch write (%u/%u)", static_cast<unsigned>(written), static_cast<unsigned>(html.size()));
    return false;
  }
  return true;
}

// One entry block of a pack definition (between "---" separators).
void formatBlock(std::string& out, const std::string& headword, const char* text, const size_t len) {
  std::string reading;
  std::string tags;
  std::string notes;
  std::string glosses;
  std::string examples;

  size_t pos = 0;
  while (pos < len) {
    size_t end = pos;
    while (end < len && text[end] != '\n') end++;
    const char* line = text + pos;
    size_t lineLen = end - pos;
    pos = end + 1;
    while (lineLen > 0 && (line[lineLen - 1] == ' ' || line[lineLen - 1] == '\r')) lineLen--;
    if (lineLen == 0) continue;

    static constexpr char kOpen[] = "\xE3\x80\x90";    // 【
    static constexpr char kClose[] = "\xE3\x80\x91";   // 】
    static constexpr char kArrow[] = "\xE2\x86\x92";   // →
    static constexpr char kBullet[] = "\xE2\x80\xA2";  // •
    if (reading.empty() && startsWith(line, lineLen, kOpen)) {
      size_t inner = sizeof(kOpen) - 1;
      size_t innerEnd = lineLen;
      if (lineLen >= inner + sizeof(kClose) - 1 &&
          std::memcmp(line + lineLen - (sizeof(kClose) - 1), kClose, sizeof(kClose) - 1) == 0) {
        innerEnd = lineLen - (sizeof(kClose) - 1);
      }
      appendEscaped(reading, line + inner, innerEnd - inner);
    } else if (line[0] == '[' && line[lineLen - 1] == ']') {
      if (!tags.empty()) tags += ' ';
      appendEscaped(tags, line + 1, lineLen - 2);
    } else if (startsWith(line, lineLen, kArrow)) {
      notes += "<i>";
      appendEscaped(notes, line, lineLen);
      notes += "</i><br/>";
    } else if (startsWith(line, lineLen, kBullet)) {
      size_t skip = sizeof(kBullet) - 1;
      while (skip < lineLen && line[skip] == ' ') skip++;
      glosses += "<li>";
      appendEscaped(glosses, line + skip, lineLen - skip);
      glosses += "</li>";
    } else {
      appendEscaped(examples, line, lineLen);
      examples += "<br/>";
    }
  }

  // Header: headword in bold, then the pinyin/zhuyin reading and level tags.
  out += "<p><b>";
  appendEscaped(out, headword.data(), headword.size());
  out += "</b>";
  if (!reading.empty()) {
    out += "  ";
    out += reading;
  }
  if (!tags.empty()) {
    out += "  <small>";
    out += tags;
    out += "</small>";
  }
  out += "</p>";
  if (!notes.empty()) {
    out += "<p>";
    out += notes;
    out += "</p>";
  }
  if (!glosses.empty()) {
    out += "<ol>";
    out += glosses;
    out += "</ol>";
  }
  if (!examples.empty()) {
    out += "<blockquote>";
    out += examples;
    out += "</blockquote>";
  }
}

// Append `len` bytes to a TSV field, turning tabs and line breaks into spaces.
void appendField(std::string& out, const char* text, const size_t len) {
  for (size_t i = 0; i < len; i++) {
    const char c = text[i];
    out += (c == '\t' || c == '\n' || c == '\r') ? ' ' : c;
  }
}

}  // namespace

std::string formatVocabRow(const std::string& headword, const std::string& definition, const std::string& sentenceHtml,
                           const std::string& book) {
  static constexpr char kOpen[] = "\xE3\x80\x90";    // 【
  static constexpr char kClose[] = "\xE3\x80\x91";   // 】
  static constexpr char kBullet[] = "\xE2\x80\xA2";  // •
  static constexpr char kSeparator[] = "\n\n---\n";
  static constexpr int kMaxGlosses = 3;

  // Only the first (most common) entry block feeds the card.
  const size_t blockEnd = std::min(definition.find(kSeparator), definition.size());
  std::string reading;
  std::string glosses;
  std::string tags;
  int glossCount = 0;
  size_t pos = 0;
  while (pos < blockEnd) {
    size_t end = definition.find('\n', pos);
    if (end == std::string::npos || end > blockEnd) end = blockEnd;
    const char* line = definition.data() + pos;
    size_t lineLen = end - pos;
    pos = end + 1;
    while (lineLen > 0 && (line[lineLen - 1] == ' ' || line[lineLen - 1] == '\r')) lineLen--;
    if (lineLen == 0) continue;
    if (reading.empty() && startsWith(line, lineLen, kOpen)) {
      size_t innerEnd = lineLen;
      if (lineLen >= 2 * (sizeof(kClose) - 1) &&
          std::memcmp(line + lineLen - (sizeof(kClose) - 1), kClose, sizeof(kClose) - 1) == 0) {
        innerEnd -= sizeof(kClose) - 1;
      }
      appendField(reading, line + sizeof(kOpen) - 1, innerEnd - (sizeof(kOpen) - 1));
    } else if (line[0] == '[' && line[lineLen - 1] == ']') {
      if (!tags.empty()) tags += ' ';
      appendField(tags, line + 1, lineLen - 2);
    } else if (glossCount < kMaxGlosses && startsWith(line, lineLen, kBullet)) {
      size_t skip = sizeof(kBullet) - 1;
      while (skip < lineLen && line[skip] == ' ') skip++;
      if (glossCount++ > 0) glosses += "; ";
      appendField(glosses, line + skip, lineLen - skip);
    }
  }

  std::string row;
  row.reserve(headword.size() + reading.size() + glosses.size() + sentenceHtml.size() + book.size() + tags.size() + 5);
  appendField(row, headword.data(), headword.size());
  row += '\t';
  row += reading;
  row += '\t';
  row += glosses;
  row += '\t';
  appendField(row, sentenceHtml.data(), sentenceHtml.size());
  row += '\t';
  appendField(row, book.data(), book.size());
  row += '\t';
  row += tags;
  return row;
}

bool available() {
  if (g_availability == Availability::Unknown) {
    selectMandarin();
    g_availability = DictIndex::isAvailable() ? Availability::Present : Availability::Absent;
    LOG_DBG("CJKD", "Mandarin pack %s", g_availability == Availability::Present ? "found" : "absent");
  }
  return g_availability == Availability::Present;
}

bool wantsWord(const std::string& word) {
  const auto* ptr = reinterpret_cast<const unsigned char*>(word.c_str());
  while (*ptr) {
    if (utf8IsHan(utf8NextCodepoint(&ptr))) return true;
  }
  return false;
}

std::string formatEntryHtml(const std::string& headword, const std::string& definition) {
  std::string out;
  out.reserve(definition.size() + definition.size() / 2 + 64);
  static constexpr char kSeparator[] = "\n\n---\n";
  size_t start = 0;
  bool first = true;
  while (start <= definition.size()) {
    size_t sep = definition.find(kSeparator, start);
    if (sep == std::string::npos) sep = definition.size();
    if (!first) out += "<p></p>";
    formatBlock(out, headword, definition.data() + start, sep - start);
    first = false;
    if (sep >= definition.size()) break;
    start = sep + sizeof(kSeparator) - 1;
  }
  return out;
}

DictLocation locate(const std::string& word, const VocabContext* vocab) {
  DictLocation loc;
  if (!available()) return loc;
  selectMandarin();

  DictEntry entry;
  bool found = DictIndex::lookupExact(word.c_str(), entry);
  if (!found) {
    // Longest dictionary word at the start of the selection.
    WordLookupResult result;
    if (WordLookup::lookup(word, 0, result)) {
      entry = std::move(result.entry);
      found = true;
    }
  }
  if (!found || entry.definition.empty()) {
    if (DictIndex::consumeHeapLimited()) {
      LOG_ERR("CJKD", "Lookup dropped for low heap: %s", word.c_str());
      loc.readError = true;
    }
    return loc;
  }

  std::string html = formatEntryHtml(entry.headword, entry.definition);
  if (vocab) {
    VocabLog::stage(VocabLog::kMandarinPath,
                    formatVocabRow(entry.headword, entry.definition, vocab->sentenceHtml, vocab->book));
  }
  entry.definition.clear();
  entry.definition.shrink_to_fit();

  std::string stem;
  if (!writeScratchEntry(html, stem)) {
    loc.readError = true;
    return loc;
  }
  loc.folderPath = std::move(stem);
  loc.headword = std::move(entry.headword);
  loc.offset = 0;
  loc.size = static_cast<uint32_t>(html.size());
  loc.found = true;
  return loc;
}

uint8_t matchLengthsAt(const std::string& text, const size_t byteOffset, uint8_t priority[kMaxWordChars]) {
  if (!available()) return 0;
  selectMandarin();
  return WordLookup::lookupAll(text, byteOffset, priority, kMaxWordChars);
}

void releaseCaches() {
  DictIndex::releaseCaches();
  g_availability = Availability::Unknown;
}

}  // namespace CjkDictionary
