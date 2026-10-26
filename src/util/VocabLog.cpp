#include "VocabLog.h"

#include <HalStorage.h>
#include <Logging.h>
#include <common/FsApiConstants.h>

#include <atomic>

namespace VocabLog {
namespace {
std::string g_lastHeadword;

// One staged row. The lookup worker writes it before setting g_staged; render
// reads only the atomics, and saveStaged() runs on the main loop after the
// lookup finished, so the string is never touched by two tasks at once.
const char* g_stagedPath = nullptr;
std::string g_stagedRow;
std::atomic<bool> g_staged{false};
std::atomic<bool> g_stagedSaved{false};

std::string parentOf(const char* path) {
  const std::string p(path);
  const size_t slash = p.rfind('/');
  return slash == std::string::npos || slash == 0 ? std::string("/") : p.substr(0, slash);
}
}  // namespace

bool append(const char* path, const std::string& row) {
  const std::string headword = row.substr(0, row.find('\t'));
  if (headword.empty()) return false;
  if (headword == g_lastHeadword) return true;

  if (!Storage.ensureDirectoryExists(parentOf(path).c_str())) {
    LOG_ERR("VOCAB", "Cannot create folder for %s", path);
    return false;
  }
  const bool isNew = !Storage.exists(path);
  HalFile file = Storage.open(path, O_WRONLY | O_CREAT | O_AT_END);
  if (!file) {
    LOG_ERR("VOCAB", "Failed to open for append: %s", path);
    return false;
  }
  static constexpr char kHeader[] =
      "#separator:tab\n#html:true\n#columns:Word\tReading\tMeaning\tSentence\tBook\tLevel\n";
  bool ok = !isNew || file.write(kHeader, sizeof(kHeader) - 1) == sizeof(kHeader) - 1;
  ok = ok && file.write(row.data(), row.size()) == row.size() && file.write("\n", 1) == 1;
  file.flush();
  file.close();
  if (!ok) {
    LOG_ERR("VOCAB", "Failed to append to %s", path);
    return false;
  }
  g_lastHeadword = headword;
  return true;
}

void stage(const char* path, std::string row) {
  g_staged = false;
  g_stagedPath = path;
  g_stagedRow = std::move(row);
  g_stagedSaved = false;
  g_staged = !g_stagedRow.empty();
}

void clearStaged() {
  g_staged = false;
  g_stagedSaved = false;
  g_stagedRow.clear();
  g_stagedRow.shrink_to_fit();
}

bool hasStaged() { return g_staged; }

bool stagedSaved() { return g_staged && g_stagedSaved; }

bool saveStaged() {
  if (!g_staged || !g_stagedPath) return false;
  if (g_stagedSaved) return true;
  if (!append(g_stagedPath, g_stagedRow)) return false;
  g_stagedSaved = true;
  LOG_INF("VOCAB", "Saved %s", g_stagedRow.substr(0, g_stagedRow.find('\t')).c_str());
  return true;
}

}  // namespace VocabLog
