#pragma once
#include <cstddef>
#define LOG_ERR(...) ((void)0)
#define LOG_INF(...) ((void)0)
#define LOG_DBG(...) ((void)0)
// DictIndex sizes its caches against the largest free heap block.
struct EspHeapStub {
  size_t getMaxAllocHeap() const { return 1u << 20; }
};
inline EspHeapStub ESP;
