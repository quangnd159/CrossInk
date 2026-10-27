// Host tests for the Mandarin pack bridge and segmenter, run against a tiny pack
// built by tools/dict_convert/convert_jmdict.py from fixture-src/ (see README).
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "CjkDictionary.h"
#include "CjkSegmenter.h"
#include "HalStorage.h"

namespace fs = std::filesystem;

namespace {

std::string readFile(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

class CjkDictionaryTest : public ::testing::Test {
 protected:
  fs::path root;

  void SetUp() override {
    root = fs::temp_directory_path() / ("cjkdict-" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
                                        "-" + ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(root);
    fs::create_directories(root / "dictionaries");
    fs::copy(fs::path(CJK_TEST_FIXTURE_DIR) / "zh", root / "dictionaries" / "zh", fs::copy_options::recursive);
    halStorageRoot() = root.string();
    CjkDictionary::releaseCaches();
  }

  void TearDown() override {
    CjkDictionary::releaseCaches();
    fs::remove_all(root);
  }

  std::string entryHtml(const DictLocation& loc) {
    return readFile(root.string() + loc.folderPath + ".dict").substr(loc.offset, loc.size);
  }

  std::vector<int> segment(const std::string& text) {
    std::vector<uint16_t> byteAt;
    for (size_t i = 0; i < text.size(); i++) {
      if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) byteAt.push_back(static_cast<uint16_t>(i));
    }
    const int cells = static_cast<int>(byteAt.size());
    byteAt.push_back(static_cast<uint16_t>(text.size()));
    std::vector<uint8_t> lens(static_cast<size_t>(cells));
    CjkSegmenter seg;
    EXPECT_TRUE(seg.segmentCells(text, byteAt.data(), cells, lens.data()));
    return {lens.begin(), lens.end()};
  }
};

TEST_F(CjkDictionaryTest, DetectsInstalledPack) { EXPECT_TRUE(CjkDictionary::available()); }

TEST_F(CjkDictionaryTest, AbsentPackDisablesLookup) {
  fs::remove_all(root / "dictionaries");
  CjkDictionary::releaseCaches();
  EXPECT_FALSE(CjkDictionary::available());
  EXPECT_FALSE(CjkDictionary::locate("银行").found);
}

TEST_F(CjkDictionaryTest, WantsOnlyHanText) {
  EXPECT_TRUE(CjkDictionary::wantsWord("银行"));
  EXPECT_TRUE(CjkDictionary::wantsWord("ATM机"));
  EXPECT_FALSE(CjkDictionary::wantsWord("bank"));
  EXPECT_FALSE(CjkDictionary::wantsWord("ひらがな"));
  EXPECT_FALSE(CjkDictionary::wantsWord("한국어"));
}

TEST_F(CjkDictionaryTest, ExactLookupWritesFormattedEntry) {
  const DictLocation loc = CjkDictionary::locate("银行");
  ASSERT_TRUE(loc.found);
  EXPECT_EQ(loc.headword, "银行");
  EXPECT_EQ(loc.folderPath, "/.crosspoint/cjkdict/Mini CEDICT/entry");
  const std::string html = entryHtml(loc);
  EXPECT_NE(html.find("<b>银行</b>"), std::string::npos) << html;
  EXPECT_NE(html.find("yín háng"), std::string::npos) << html;
  EXPECT_NE(html.find("<li>bank</li>"), std::string::npos) << html;
  // The scratch .ifo makes the definition view treat the entry as HTML.
  const std::string ifo = readFile(root.string() + loc.folderPath + ".ifo");
  EXPECT_EQ(ifo.rfind("StarDict's dict ifo file\n", 0), 0u);
  EXPECT_NE(ifo.find("sametypesequence=h"), std::string::npos);
}

TEST_F(CjkDictionaryTest, MultipleReadingsAreAllShown) {
  const DictLocation loc = CjkDictionary::locate("行");
  ASSERT_TRUE(loc.found);
  const std::string html = entryHtml(loc);
  EXPECT_NE(html.find("háng"), std::string::npos) << html;
  EXPECT_NE(html.find("xíng"), std::string::npos) << html;
}

TEST_F(CjkDictionaryTest, TraditionalFormFindsTheSameEntry) {
  const DictLocation loc = CjkDictionary::locate("銀行");
  ASSERT_TRUE(loc.found);
  EXPECT_NE(entryHtml(loc).find("<li>bank</li>"), std::string::npos);
}

TEST_F(CjkDictionaryTest, SelectionThatIsNoWordIsNotFoundByDefault) {
  const DictLocation loc = CjkDictionary::locate("我们的");
  EXPECT_FALSE(loc.found);
  EXPECT_FALSE(loc.readError);
}

TEST_F(CjkDictionaryTest, FallsBackToLongestPrefixWithNotice) {
  const DictLocation loc = CjkDictionary::locate("我们的", nullptr, "\"%s\" missing, showing %s:");
  ASSERT_TRUE(loc.found);
  EXPECT_EQ(loc.headword, "我们");
  EXPECT_EQ(entryHtml(loc).rfind("<p><i>\"我们的\" missing, showing 我们:</i></p><p><b>我们</b>", 0), 0u)
      << entryHtml(loc);
}

TEST_F(CjkDictionaryTest, ExactMatchCarriesNoNotice) {
  const DictLocation loc = CjkDictionary::locate("我们", nullptr, "%s %s");
  ASSERT_TRUE(loc.found);
  EXPECT_EQ(entryHtml(loc).rfind("<p><b>我们</b>", 0), 0u) << entryHtml(loc);
}

TEST_F(CjkDictionaryTest, UnknownWordIsNotFound) {
  const DictLocation loc = CjkDictionary::locate("龘");
  EXPECT_FALSE(loc.found);
  EXPECT_FALSE(loc.readError);
}

TEST_F(CjkDictionaryTest, SegmentsByFrequencyNotLongestMatch) {
  // 和尚 (monk) is a word, but 和 + 尚未 ("and not yet") is the likelier reading.
  EXPECT_EQ(segment("和尚未"), (std::vector<int>{1, 2, 0}));
}

TEST_F(CjkDictionaryTest, SegmentsCommonWords) {
  EXPECT_EQ(segment("我们的"), (std::vector<int>{2, 0, 1}));
  EXPECT_EQ(segment("不是学生"), (std::vector<int>{2, 0, 2, 0}));
  EXPECT_EQ(segment("银行"), (std::vector<int>{2, 0}));
}

TEST_F(CjkDictionaryTest, UnknownCharactersStayAlone) {
  EXPECT_EQ(segment("龘银行龘"), (std::vector<int>{1, 2, 0, 1}));
}

TEST_F(CjkDictionaryTest, LongRunsSegmentAcrossChunks) {
  std::string text;
  for (int i = 0; i < 20; i++) text += "学生";  // 40 cells: spans the 32-cell chunk
  const auto lens = segment(text);
  ASSERT_EQ(lens.size(), 40u);
  for (size_t i = 0; i < lens.size(); i += 2) {
    EXPECT_EQ(lens[i], 2) << i;
    EXPECT_EQ(lens[i + 1], 0) << i;
  }
}

TEST_F(CjkDictionaryTest, ChunkEdgeSplitsLikeTheMiddleOfARun) {
  // 和尚未 straddles the 32-cell chunk edge (cells 30-32). Scored alone, the
  // chunk would end on 和尚 and leave 未; with the rest of the run it is 和 + 尚未.
  std::string text;
  for (int i = 0; i < 15; i++) text += "学生";
  text += "和尚未";
  const auto lens = segment(text);
  ASSERT_EQ(lens.size(), 33u);
  EXPECT_EQ(std::vector<int>(lens.begin() + 30, lens.end()), (std::vector<int>{1, 2, 0}));
}

TEST(CjkDictionaryFormat, EscapesMarkupAndStructuresEntry) {
  const std::string html = CjkDictionary::formatEntryHtml(
      "你好", "【nǐ hǎo】\n[HSK 1]\n→ 你好 / 你好\n• hello <greeting>\n• hi\n你好吗？\nHow are you?");
  EXPECT_EQ(html,
            "<p><b>你好</b>  nǐ hǎo  <small>HSK 1</small></p><p><i>→ 你好 / 你好</i><br/></p>"
            "<ol><li>hello &lt;greeting&gt;</li><li>hi</li></ol>"
            "<blockquote>你好吗？<br/>How are you?<br/></blockquote>");
}

TEST(CjkDictionaryFormat, SeparatesMergedEntries) {
  const std::string html = CjkDictionary::formatEntryHtml("行", "【háng】\n• row\n\n---\n【xíng】\n• to walk");
  EXPECT_NE(html.find("háng</p><ol><li>row</li></ol><p></p><p><b>行</b>  xíng"), std::string::npos) << html;
}

}  // namespace

TEST(CjkDictionaryFormat, VocabRowUsesFirstEntryAndCleansFields) {
  const std::string row = CjkDictionary::formatVocabRow(
      "行", "【xíng】\n[HSK 1]\n• to walk\n• to go\n• OK\n• capable\n例句\n\n---\n【háng】\n• row", "我们<b>行</b>吗？",
      "Book\tTitle");
  EXPECT_EQ(row, "行\txíng\tto walk; to go; OK\t我们<b>行</b>吗？\tBook Title\tHSK 1");
}

TEST(CjkDictionaryFormat, VocabRowToleratesMissingParts) {
  EXPECT_EQ(CjkDictionary::formatVocabRow("们", "• plural marker", "", ""), "们\t\tplural marker\t\t\t");
}
