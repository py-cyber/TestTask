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
    length_ = 0;
  }
  fs_.open(filepath_, std::ios::in | std::ios::out | std::ios::binary);
  if (!fs_.is_open()) {
    throw std::runtime_error("Cannot open tape file: " + filepath_);
  }
  if (!truncate) {
    length_ = std::filesystem::file_size(filepath_) / sizeof(int32_t);
  }
  seek_to_pos();
}

file_tape::~file_tape() {
  if (fs_.is_open()) {
    fs_.flush();
    fs_.close();
  }
}

void file_tape::seek_to_pos() {
  fs_.clear();
  fs_.seekg(pos_ * sizeof(int32_t));
  fs_.seekp(pos_ * sizeof(int32_t));
}

bool file_tape::read(int32_t &value) {
  if (pos_ >= length_)
    return false;
  apply_delay(cfg_.read_delay_ms);
  fs_.clear();
  fs_.seekg(pos_ * sizeof(int32_t));
  fs_.read(reinterpret_cast<char *>(&value), sizeof(int32_t));
  if (fs_.gcount() != sizeof(int32_t))
    return false;
  return true;
}

bool file_tape::write(int32_t value) {
  apply_delay(cfg_.write_delay_ms);
  fs_.clear();
  fs_.seekp(pos_ * sizeof(int32_t));
  fs_.write(reinterpret_cast<const char *>(&value), sizeof(int32_t));
  if (fs_.fail()) {
    throw std::runtime_error("Write failed on tape: " + filepath_);
  }
  if (pos_ >= length_) {
    length_ = pos_ + 1;
  }
  return true;
}

void file_tape::rewind() {
  apply_delay(cfg_.rewind_delay_ms);
  pos_ = 0;
  seek_to_pos();
}

void file_tape::moveForward() {
  apply_delay(cfg_.step_delay_ms);
  if (pos_ < length_) {
    ++pos_;
  }
  seek_to_pos();
}

void file_tape::moveBackward() {
  apply_delay(cfg_.step_delay_ms);
  if (pos_ > 0) {
    --pos_;
  }
  seek_to_pos();
}

void file_tape::apply_delay(int ms) const {
  if (ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }
}