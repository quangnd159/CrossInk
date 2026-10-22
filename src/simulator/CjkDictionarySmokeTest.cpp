#if defined(SIMULATOR)
#include "CjkDictionarySmokeTest.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "CrossPointSettings.h"
#include "FilenameFontSystem.h"
#include "SdCardFontSystem.h"
#include "activities/ActivityManager.h"
#include "activities/reader/DictionaryWordSelectActivity.h"
#include "activities/reader/EpubReaderActivity.h"

extern ActivityManager activityManager;
extern GfxRenderer renderer;
extern MappedInputManager mappedInputManager;

namespace {
void require(const bool condition, const char* message) {
  if (condition) return;
  LOG_ERR("SMOKE", "CJK dictionary: %s", message);
  std::_Exit(1);
}

void capture(const char* name) {
  const char* directory = std::getenv("CROSSINK_SIMULATOR_SMOKE_STATUS_BAR_CAPTURES");
  if (!directory) return;
  std::filesystem::create_directories(directory);
  const auto path = std::filesystem::path(directory) / (std::string(name) + ".pgm");
  FILE* output = std::fopen(path.c_str(), "wb");
  require(output, "Cannot create screenshot");
  RenderLock lock;
  const int width = renderer.getScreenWidth(), height = renderer.getScreenHeight();
  std::fprintf(output, "P5\n%d %d\n255\n", width, height);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x) std::fputc(renderer.isPixelBlack(x, y) ? 0 : 255, output);
  std::fclose(output);
}

std::string selectedPhrase(const WordSelectNavigator& nav) {
  int from = -1, to = -1;
  if (!nav.getLookupSelectionRange(from, to)) return {};
  return nav.buildPhrase(from, to);
}
}  // namespace

void CjkDictionarySmokeTest::prepareReaderFont() {
  const char* family = std::getenv("CROSSINK_SIMULATOR_SMOKE_CJK_FONT");
  if (!family || !family[0]) return;
  std::snprintf(SETTINGS.sdFontFamilyName, sizeof(SETTINGS.sdFontFamilyName), "%s", family);
  SETTINGS.readerFontPointSize = 12;
  sdFontSystem.ensureLoaded(renderer);
  LOG_INF("SMOKE", "CJK dictionary: reader font %s", family);
}

bool CjkDictionarySmokeTest::tick() {
  static int stage = 0;
  static int moves = 0;
  static int previousIndex = -1;
  static uint32_t lastStep = 0;
  static uint32_t lookupStarted = 0;
  if (millis() - lastStep < 300 || RenderLock::peek()) return false;
  lastStep = millis();

  Activity* current = activityManager.simulatorCurrentActivity();
  auto* reader = dynamic_cast<EpubReaderActivity*>(current);
  auto* selection = dynamic_cast<DictionaryWordSelectActivity*>(current);
  constexpr int kMoves = 6;

  switch (stage) {
    case 0:
      if (!reader) return false;
      reader->openWordSelect(false);
      stage = 1;
      return false;

    case 1: {
      if (!selection) return false;
      auto& nav = selection->navigator;
      require(nav.hasSegmentation(), "Han page did not attach Mandarin segmentation");
      previousIndex = nav.getCurrentFlatIndex();
      require(previousIndex >= 0, "No selected word");
      require(nav.segmentStartOf(previousIndex) == previousIndex, "Initial cursor is not at a word start");
      LOG_INF("SMOKE", "CJK word 0: %s", selectedPhrase(nav).c_str());
      capture("cjk-select-0");
      stage = 2;
      return false;
    }

    case 2:  // press Next word
      require(selection != nullptr, "Word select closed unexpectedly");
      mappedInputManager.simulatorInjectPress(MappedInputManager::Button::Right);
      stage = 3;
      return false;

    case 3:
      mappedInputManager.simulatorInjectRelease(MappedInputManager::Button::Right);
      stage = 4;
      return false;

    case 4: {
      require(selection != nullptr, "Word select closed unexpectedly");
      auto& nav = selection->navigator;
      const int idx = nav.getCurrentFlatIndex();
      const int expected = nav.segmentEndOf(previousIndex) + 1;
      require(idx == expected || (!nav.getWordAt(expected) && idx == 0), "Next did not jump past the whole word");
      require(nav.segmentStartOf(idx) == idx, "Cursor landed inside a word");
      const std::string phrase = selectedPhrase(nav);
      LOG_INF("SMOKE", "CJK word %d: %s (span %d)", moves + 1, phrase.c_str(), nav.segmentEndOf(idx) - idx + 1);
      char name[32];
      std::snprintf(name, sizeof(name), "cjk-select-%d", moves + 1);
      capture(name);
      previousIndex = idx;
      stage = ++moves < kMoves ? 2 : 5;
      return false;
    }

    case 5: {
      // Look up the first multi-character word on the page.
      require(selection != nullptr, "Word select closed unexpectedly");
      auto& nav = selection->navigator;
      int target = -1;
      for (int i = 0; nav.getWordAt(i); i = nav.segmentEndOf(i) + 1) {
        nav.selectIndex(i);
        nav.snapToSegmentStart();
        const int start = nav.getCurrentFlatIndex();
        if (nav.segmentEndOf(start) > start) {
          target = start;
          break;
        }
      }
      require(target >= 0, "No multi-character word found on the page");
      LOG_INF("SMOKE", "CJK lookup target: %s", selectedPhrase(nav).c_str());
      selection->requestUpdate();
      stage = 6;
      return false;
    }

    case 6:
      capture("cjk-select-target");
      mappedInputManager.simulatorInjectPress(MappedInputManager::Button::Confirm);
      stage = 7;
      return false;

    case 7:
      mappedInputManager.simulatorInjectRelease(MappedInputManager::Button::Confirm);
      lookupStarted = millis();
      stage = 8;
      return false;

    case 8:
      if (!activityManager.isCurrentActivityNamed("DictionaryDefinition")) {
        require(millis() - lookupStarted < 15000, "Definition view did not open");
        return false;
      }
      stage = 9;
      return false;

    case 9:
      capture("cjk-definition");
      {
        HalFile vocab;
        require(Storage.openFileForRead("SMOKE", "/vocab/mandarin.tsv", vocab), "Vocabulary log was not written");
        std::string text;
        for (int c = vocab.read(); c >= 0; c = vocab.read()) text += static_cast<char>(c);
        vocab.close();
        const size_t row = text.rfind('\n', text.size() - 2);
        const std::string last = text.substr(row == std::string::npos ? 0 : row + 1);
        LOG_INF("SMOKE", "CJK vocab row: %s", last.c_str());
        require(text.rfind("#separator:tab", 0) == 0, "Vocabulary log lacks the Anki header");
        require(last.find("<b>") != std::string::npos && last.find("test_mandarin_reading") != std::string::npos,
                "Vocabulary row lacks sentence or book");
      }
      LOG_INF("SMOKE", "CJK dictionary: Mandarin word segmentation and lookup passed");
      // Chinese file names in a Chinese folder; 龘 is outside the built-in UI set.
      require(Storage.ensureDirectoryExists("/书库"), "Cannot create Chinese folder");
      for (const char* name : {"/书库/三体：地球往事.epub", "/书库/红楼梦 第一卷.epub",
                               "/书库/Harry Potter 哈利·波特.epub", "/书库/龘龘 Rare.epub"}) {
        HalFile file;
        require(Storage.openFileForWrite("SMOKE", name, file), "Cannot create Chinese file name");
        file.close();
      }
      if (const char* uiFont = std::getenv("CROSSINK_SIMULATOR_SMOKE_CJK_UI_FONT")) {
        std::snprintf(SETTINGS.filenameFallbackFont, sizeof(SETTINGS.filenameFallbackFont), "%s", uiFont);
        filenameFontSystem.invalidate();
      }
      activityManager.goToFileBrowser("/书库");
      stage = 10;
      return false;

    case 10:
      if (!activityManager.isCurrentActivityNamed("FileBrowser")) return false;
      stage = 11;
      return false;

    case 11:
      capture("cjk-ui-browser");
      activityManager.goToSettings();
      stage = 12;
      return false;

    case 12:
      if (!activityManager.isCurrentActivityNamed("Settings")) return false;
      stage = 13;
      return false;

    case 13:
      capture("cjk-ui-settings");
      activityManager.goHome();
      stage = 14;
      return false;

    case 14:
      if (!activityManager.isCurrentActivityNamed("Home")) return false;
      stage = 15;
      return false;

    case 15:
      capture("cjk-ui-home");
      LOG_INF("SMOKE", "Simulator smoke test passed: Mandarin word segmentation, lookup and Chinese UI");
      return true;

    default:
      return false;
  }
}
#endif
