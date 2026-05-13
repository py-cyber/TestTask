#pragma once
#include <cstddef>
#include <string>

struct tape_config {
  int read_delay_ms = 1;
  int write_delay_ms = 1;
  int rewind_delay_ms = 5;
  int step_delay_ms = 0;
  std::size_t memory_limit_bytes = 1024 * 1024;
  std::string config_path = "config.cfg";
};

tape_config load_config(const std::string &path);
