#include "VocabLog.h"

#include <HalStorage.h>
#include <Logging.h>
#include <common/FsApiConstants.h>

namespace VocabLog {
namespace {
std::string g_lastHeadword;

std::string parentOf(const char* path) {
  const std::string p(path);
  const size_t slash = p.rfind('/');
  return slash == std::string::npos || slash == 0 ? std::string("/") : p.substr(0, slash);
}
}  // namespace

bool append(const char* path, const std::string& row) {
  const std::string headword = row.substr(0, row.find('\t'));
  if (headword.empty() || headword == g_lastHeadword) return false;

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

}  // namespace VocabLog
