#pragma once
#if defined(SIMULATOR)
// Button-driven Mandarin lookup flow: open word select on a Chinese page, check
// that the cursor moves and highlights by dictionary word, confirm a lookup and
// check the definition view shows the Mandarin pack entry. Enabled by
// CROSSINK_SIMULATOR_SMOKE_CJK_DICTIONARY (see run_simulator_smoke_test.py --cjk-dictionary).
struct CjkDictionarySmokeTest {
  // Before the reader opens: select the CJK SD font named by
  // CROSSINK_SIMULATOR_SMOKE_CJK_FONT so the page can render Han text.
  static void prepareReaderFont();
  // Returns true once the scenario has passed; exits the process on failure.
  static bool tick();
};
#endif
