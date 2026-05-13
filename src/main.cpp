#include "external_tape_sorter.h"
#include "file_tape.h"
#include "tape_config.h"
#include <filesystem>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0]
              << " <input_file> <output_file> [config_file]\n";
    return 1;
  }
  std::string input_path = argv[1];
  std::string output_path = argv[2];
  std::string config_path = (argc > 3) ? argv[3] : "example_config.cfg";
  auto cfg = load_config(config_path);
  std::filesystem::create_directories("tmp");
  try {
    file_tape in_tape(input_path, cfg, false);
    file_tape out_tape(output_path, cfg, true);
    external_tape_sorter sorter(cfg);
    sorter.sort(in_tape, out_tape);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
