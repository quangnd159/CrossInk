#pragma once
// Host stub: HalStorage backed by real files under a test-chosen root, so the
// dictionary code reads a pack produced by the real converter.
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <string>

inline std::string& halStorageRoot() {
  static std::string root;
  return root;
}

class HalFile {
 public:
  HalFile() = default;
  HalFile(const HalFile&) = delete;
  HalFile& operator=(const HalFile&) = delete;
  HalFile(HalFile&& o) noexcept : f_(o.f_) { o.f_ = nullptr; }
  HalFile& operator=(HalFile&& o) noexcept {
    if (this != &o) {
      close();
      f_ = o.f_;
      o.f_ = nullptr;
    }
    return *this;
  }
  ~HalFile() { close(); }
  bool open(const std::string& path, const char* mode) {
    close();
    f_ = std::fopen(path.c_str(), mode);
    return f_ != nullptr;
  }
  bool isOpen() const { return f_ != nullptr; }
  explicit operator bool() const { return isOpen(); }
  void close() {
    if (f_) std::fclose(f_);
    f_ = nullptr;
  }
  bool seek(uint32_t pos) { return f_ && std::fseek(f_, static_cast<long>(pos), SEEK_SET) == 0; }
  bool seekSet(uint32_t pos) { return seek(pos); }
  int read() {
    if (!f_) return -1;
    const int c = std::fgetc(f_);
    return c == EOF ? -1 : c;
  }
  int read(void* buf, size_t n) { return f_ ? static_cast<int>(std::fread(buf, 1, n, f_)) : -1; }
  size_t write(const void* buf, size_t n) { return f_ ? std::fwrite(buf, 1, n, f_) : 0; }
  size_t write(const uint8_t* buf, size_t n) { return write(static_cast<const void*>(buf), n); }
  uint32_t size() {
    if (!f_) return 0;
    const long cur = std::ftell(f_);
    std::fseek(f_, 0, SEEK_END);
    const long end = std::ftell(f_);
    std::fseek(f_, cur, SEEK_SET);
    return static_cast<uint32_t>(end);
  }
  uint32_t fileSize() { return size(); }

 private:
  FILE* f_ = nullptr;
};

class HalStorage {
 public:
  static std::string real(const char* p) { return halStorageRoot() + p; }
  bool exists(const char* p) { return std::filesystem::exists(real(p)); }
  bool ensureDirectoryExists(const char* p) {
    std::error_code ec;
    std::filesystem::create_directories(real(p), ec);
    return !ec;
  }
  bool openFileForRead(const char*, const char* p, HalFile& f) { return f.open(real(p), "rb"); }
  bool openFileForRead(const char* m, const std::string& p, HalFile& f) { return openFileForRead(m, p.c_str(), f); }
  bool openFileForWrite(const char*, const char* p, HalFile& f) { return f.open(real(p), "wb"); }
  bool openFileForWrite(const char* m, const std::string& p, HalFile& f) { return openFileForWrite(m, p.c_str(), f); }
};

inline HalStorage Storage;
