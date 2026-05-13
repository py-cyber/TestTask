#include "file_tape.h"
#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <thread>

file_tape::file_tape(const std::string &filepath, const tape_config &cfg,
                     bool truncate)
    : filepath_(filepath), cfg_(cfg) {
  std::filesystem::path p(filepath_);
  if (!p.parent_path().empty()) {
    std::filesystem::create_directories(p.parent_path());
  }

  if (truncate) {
    std::ofstream tmp(filepath_, std::ios::binary | std::ios::trunc);
    tmp.close();
  }

  fs_.open(filepath_, std::ios::in | std::ios::out | std::ios::binary);
  if (!fs_.is_open()) {
    throw std::runtime_error("Cannot open tape file: " + filepath_);
  }

  if (truncate) {
    length_ = 0;
  } else {
    length_ = std::filesystem::file_size(filepath_) / sizeof(int32_t);
  }

  fs_.seekg(0, std::ios::beg);
  fs_.seekp(0, std::ios::beg);
  pos_ = 0;
}

file_tape::~file_tape() {
  if (fs_.is_open()) {
    fs_.flush();
    fs_.close();
  }
}

bool file_tape::read(int32_t &value) {
  if (pos_ >= length_)
    return false;

  fs_.clear();
  fs_.seekg(pos_ * sizeof(int32_t));
  fs_.read(reinterpret_cast<char *>(&value), sizeof(int32_t));

  if (fs_.gcount() != sizeof(int32_t))
    return false;

  apply_delay(cfg_.read_delay_ms + cfg_.step_delay_ms);
  ++pos_;
  return true;
}

bool file_tape::write(int32_t value) {
  fs_.clear();
  fs_.seekp(pos_ * sizeof(int32_t));
  fs_.write(reinterpret_cast<const char *>(&value), sizeof(int32_t));

  if (pos_ >= length_)
    length_ = pos_ + 1;
  ++pos_;
  apply_delay(cfg_.write_delay_ms + cfg_.step_delay_ms);
  return true;
}

void file_tape::rewind() {
  fs_.flush();
  fs_.clear();
  pos_ = 0;
  fs_.seekg(0, std::ios::beg);
  fs_.seekp(0, std::ios::beg);
  apply_delay(cfg_.rewind_delay_ms);
}

void file_tape::apply_delay(int ms) const {
  if (ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
}
