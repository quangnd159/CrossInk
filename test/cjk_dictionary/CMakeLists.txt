add_executable(CjkDictionaryTest
  CjkDictionaryTest.cpp
  stubs/VocabLogStub.cpp
  ${REPO_ROOT}/src/util/CjkDictionary.cpp
  ${REPO_ROOT}/src/util/CjkSegmenter.cpp
  ${REPO_ROOT}/lib/Dict/DictIndex.cpp
  ${REPO_ROOT}/lib/Dict/WordLookup.cpp
  ${REPO_ROOT}/lib/Dict/Deinflector.cpp
  ${REPO_ROOT}/lib/Utf8/Utf8.cpp)
target_include_directories(CjkDictionaryTest BEFORE PRIVATE stubs)
target_include_directories(CjkDictionaryTest PRIVATE
  ${REPO_ROOT}/src/util ${REPO_ROOT}/lib/Dict ${REPO_ROOT}/lib/Utf8 ${REPO_ROOT}/lib/Memory)
target_compile_definitions(CjkDictionaryTest PRIVATE CJK_SEGMENTER_NO_NAVIGATOR
  CJK_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixture")
target_link_libraries(CjkDictionaryTest PRIVATE crosspoint_test_common GTest::gtest_main)
gtest_discover_tests(CjkDictionaryTest)
