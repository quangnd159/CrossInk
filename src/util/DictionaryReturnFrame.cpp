#include "DictionaryReturnFrame.h"

#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <Logging.h>
#include <Memory.h>

#include <cstring>

namespace DictionaryReturnFrame {
namespace {
HeapByteBuffer g_frame;
size_t g_frameBytes = 0;
bool g_valid = false;
}  // namespace

void capture(const GfxRenderer& renderer) {
  g_valid = false;
  const size_t bytes = renderer.getBufferSize();
  if (!g_frame || g_frameBytes != bytes) {
    g_frame = makePsramByteBufferNoThrow(bytes);
    g_frameBytes = g_frame ? bytes : 0;
    if (!g_frame) {
      LOG_DBG("DICT", "No PSRAM for dictionary return frame (%u bytes); dismiss will re-render",
              static_cast<unsigned>(bytes));
      return;
    }
  }
  memcpy(g_frame.get(), renderer.getFrameBuffer(), bytes);
  g_valid = true;
}

void invalidate() { g_valid = false; }

bool present(GfxRenderer& renderer) {
  if (!g_valid || !g_frame || g_frameBytes != renderer.getBufferSize()) return false;
  g_valid = false;
  memcpy(renderer.getFrameBuffer(), g_frame.get(), g_frameBytes);
  // Same pixels the reader's next render() produces, so its refresh is a no-op
  // apart from text anti-aliasing; this one removes the lookup UI right away.
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  return true;
}

bool copyTo(GfxRenderer& renderer) {
  if (!g_valid || !g_frame || g_frameBytes != renderer.getBufferSize()) return false;
  memcpy(renderer.getFrameBuffer(), g_frame.get(), g_frameBytes);
  return true;
}

void release() {
  g_valid = false;
  g_frame.reset();
  g_frameBytes = 0;
}

}  // namespace DictionaryReturnFrame
