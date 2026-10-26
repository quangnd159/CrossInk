#pragma once

class GfxRenderer;

// PSRAM-only copy of the BW reader page taken when a dictionary lookup opens
// over it. Dismissing the lookup shows this copy at once, before the lookup
// screens tear down (definition font release, reader font restore, cache
// drains) and before the reader re-renders the same pixels behind it.
// The 48 KB buffer is allocated once per book and reused; on the C3 the PSRAM
// allocation fails and every call here is a cheap no-op.
namespace DictionaryReturnFrame {

// Caller holds RenderLock and has verified the framebuffer shows the page.
void capture(const GfxRenderer& renderer);

// Forget the copy without showing it (e.g. the lookup turns into a clipping).
void invalidate();

// Show the copy with a fast refresh and consume it. Caller holds RenderLock.
// Returns false when there is no valid copy.
bool present(GfxRenderer& renderer);

// Copy the saved page into the framebuffer without refreshing or consuming it,
// so a modal can repaint its backdrop without re-rendering the reader page (or
// swapping the reader SD font back in). Caller holds RenderLock.
bool copyTo(GfxRenderer& renderer);

// Free the buffer (reader exit).
void release();

}  // namespace DictionaryReturnFrame
