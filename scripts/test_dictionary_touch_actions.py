#!/usr/bin/env python3
"""Run production selection handlers with coalesced taps and a non-recursive lock."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = "src/activities/reader/DictionaryWordSelectActivity.cpp"


def method(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


FIXTURE = r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
static bool locked = false;
struct RenderLock {
  bool owns = true;
  template<class T> explicit RenderLock(T&) {
    if (locked) { std::cerr << "recursive render lock\n"; std::exit(3); }
    locked = true;
  }
  void unlock() { if (owns) { locked = false; owns = false; } }
  ~RenderLock() { unlock(); }
};
struct Renderer { int getLineHeight(int) const { return 20; } };
struct Settings { int getReaderFontId() const { return 1; } } SETTINGS;
namespace MappedInputManager { enum class Button { Back, Confirm }; }
struct Input {
  bool down = false, tap = false;
  int x = 0, y = 0;
  bool isScreenTouchHeld(int&, int&) const { return false; }
  bool wasReleased(MappedInputManager::Button) const { return false; }
  bool wasScreenTapped(int& a, int& b) const { if (!tap) return false; a=x; b=y; return true; }
  bool wasScreenTouchDown(int& a, int& b) const { if (!down) return false; a=x; b=y; return true; }
};
namespace TextSelectionOverlay {
struct Rect {
  int x, y, width, height;
  bool contains(int a,int b) const { return a>=x && a<x+width && b>=y && b<y+height; }
};
struct Layout { Rect actions[2]{{10,20,80,40},{100,20,80,40}}; Rect startHandle{90,140,10,10},endHandle{120,140,10,10}; };
int actionAt(const Layout& l,int x,int y) {
  for (int i=0;i<2;++i) if(l.actions[i].contains(x,y)) return i;
  return -1;
}
}
struct Word {
  int screenX=100,screenY=100,width=20;
  int heightOr(int height) const { return height; }
};
struct Navigator {
  Word word;
  int getCurrentFlatIndex() const { return 0; }
  const Word* getSelected() const { return &word; }
  const Word* getWordAt(int i) const { return i==0 ? &word : nullptr; }
  void getExpandedSelectionRange(int& a,int& b) const { a=b=0; }
  bool selectNearestWord(int,int,int) { return false; }
  void clearTouchSelection() {}
  size_t getLookupSelectionWordCount() const { return 1; }
  bool setTouchSelection(int,int) { return false; }
  void selectWordAtPoint(int,int,int,bool*) {}
  bool beginTouchMultiSelect() { return false; }
};
namespace ClipSelectionPaging {
bool hasDraggedFrom(int,int,int,int) { return false; }
bool hasHeldPageEndLongEnough(uint32_t,uint32_t) { return false; }
}
uint32_t millis() { return 0; }
namespace Dictionary { bool exists(const char*) { return true; } }
namespace DictionaryReturnFrame {
static int presented=0;
bool present(Renderer&) { assert(locked); ++presented; return true; }
}
struct Controller {
  int lookups=0;
  void lookupOrPopup(const std::string& word,size_t count) {
    assert(!locked && word=="老师" && count==1); ++lookups;
  }
};
class DictionaryWordSelectActivity {
 public:
  Renderer renderer;
  Input mappedInput;
  Navigator navigator;
  Controller controller;
  bool ignoreTouchUntilRelease_=false,touchDragLookup_=false,touchSelectionActive_=true;
  bool touchLookupEnabled_=true,hasPendingClippingRequest_=true;
  int touchDragStartX_=0,touchDragStartY_=0,touchPageEndIndex_=-1;
  int touchDragOffsetX_=0,touchDragOffsetY_=0,touchPressedAction_=-1;
  bool touchDragHasMoved_=false;
  uint32_t touchPageEndHeldSince_=0;
  size_t crossPageLookupWordCount_=0,pendingTouchWordCount_=0;
  std::string pendingTouchPhrase_,cachePath;
  bool page=true,cancelled=false;
  int clips=0;
  void requestUpdate() {}
  void forceFullRepaintOnNextRender() {}
  bool isWithinSelectedPageEndDwellSlop(int,int) const { return false; }
  bool continueTouchSelectionOnNextPage() { return false; }
  void captureClippingRequest() {}
  void resetCrossPageSelection() {}
  std::string finishTouchLookupPhrase() { return "老师"; }
  void finishWithClippingRequest() { assert(locked); ++clips; }
  TextSelectionOverlay::Layout touchSelectionLayout() const { return {}; }
  void performTouchSelectionAction(int);
  bool handleTouchSelection();
  void presentReaderPage();
  void cancelToReader();
};
namespace DictUtils {
void cancelAndFinish(DictionaryWordSelectActivity& a) { a.cancelled=true; }
}
'''

CASES = r'''
int main(int argc,char** argv) {
  assert(argc==2);
  std::string scenario=argv[1];
  DictionaryWordSelectActivity a;
  a.mappedInput.x=scenario=="outside" ? 250 : (scenario=="clip" ? 120 : 30);
  a.mappedInput.y=scenario=="outside" ? 250 : 30;
  if (scenario=="slow") {
    a.mappedInput.down=true;
    assert(a.handleTouchSelection());
    assert(a.controller.lookups==0);
  }
  a.mappedInput.down=scenario=="lookup" || scenario=="clip";
  a.mappedInput.tap=true;
  if (scenario=="disabled") a.touchLookupEnabled_=false;
  assert(a.handleTouchSelection());
  assert(!locked);
  if (scenario=="outside") {
    assert(a.cancelled && DictionaryReturnFrame::presented==1);
  } else if (scenario=="disabled") {
    assert(a.controller.lookups==0 && a.touchSelectionActive_);
  } else if (scenario=="clip") {
    assert(a.clips==1 && !a.touchSelectionActive_);
  } else {
    assert(a.controller.lookups==1 && !a.touchSelectionActive_);
  }
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-ref", help="Test a Git snapshot instead of the working tree")
    args = parser.parse_args()
    source = subprocess.check_output(
        ["git", "show", f"{args.source_ref}:{SOURCE}"], cwd=ROOT, text=True
    ) if args.source_ref else (ROOT / SOURCE).read_text()
    signatures = [
        "void DictionaryWordSelectActivity::presentReaderPage()",
        "void DictionaryWordSelectActivity::cancelToReader()",
        "void DictionaryWordSelectActivity::performTouchSelectionAction(",
        "bool DictionaryWordSelectActivity::handleTouchSelection()",
    ]
    with tempfile.TemporaryDirectory(prefix="crossink-touch-actions-") as directory:
        base = Path(directory)
        (base / "test.cpp").write_text(FIXTURE + "\n".join(method(source, s) for s in signatures) + CASES)
        subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                        str(base / "test.cpp"), "-o", str(base / "test")], check=True)
        failed = []
        for scenario in ("lookup", "clip", "release-only", "slow", "disabled", "outside"):
            result = subprocess.run([str(base / "test"), scenario], capture_output=True, text=True)
            print(f"{scenario}: {'PASS' if result.returncode == 0 else 'FAIL'}", flush=True)
            if result.returncode:
                print(result.stderr, flush=True)
                failed.append(scenario)
        return int(bool(failed))


if __name__ == "__main__":
    raise SystemExit(main())
