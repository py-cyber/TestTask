#include "tape_config.h"
#include <fstream>
#include <iostream>

tape_config load_config(const std::string &path) {
  tape_config cfg;
  cfg.config_path = path;
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "\033[33mWARNING: Config file not found\033[0m\n";
    return cfg;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    auto eq_pos = line.find('=');
    if (eq_pos == std::string::npos)
      continue;

    std::string key = line.substr(0, eq_pos);
    std::string val = line.substr(eq_pos + 1);
    while (!val.empty() && isspace(val.back()))
      val.pop_back();

    if (key == "read_delay_ms")
      cfg.read_delay_ms = std::stoi(val);
    else if (key == "write_delay_ms")
      cfg.write_delay_ms = std::stoi(val);
    else if (key == "rewind_delay_ms")
      cfg.rewind_delay_ms = std::stoi(val);
    else if (key == "step_delay_ms")
      cfg.step_delay_ms = std::stoi(val);
    else if (key == "memory_limit_bytes")
      cfg.memory_limit_bytes = std::stoull(val);
  }
  return cfg;
}
