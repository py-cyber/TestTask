#pragma once
#include "tape_config.h"
#include "tape_interface.h"
#include <fstream>
#include <string>

class file_tape : public tape_interface {
public:
  file_tape(const std::string &filepath, const tape_config &cfg,
            bool truncate = false);
  ~file_tape() override;

  bool read(int32_t &value) override;
  bool write(int32_t value) override;
  void rewind() override;
  std::size_t length() const override { return length_; }
  std::size_t position() const override { return pos_; }
  const char *name() const override { return filepath_.c_str(); }

private:
  std::string filepath_;
  std::fstream fs_;
  tape_config cfg_;
  std::size_t pos_ = 0;
  std::size_t length_ = 0;

  void apply_delay(int ms) const;
};
