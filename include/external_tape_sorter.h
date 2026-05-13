#pragma once
#include "tape_config.h"
#include "tape_interface.h"
#include <memory>
#include <string>
#include <vector>

class external_tape_sorter {
public:
  explicit external_tape_sorter(const tape_config &cfg);

  void sort(tape_interface &input, tape_interface &output);

private:
  tape_config cfg_;
  std::size_t tape_counter_ = 0;

  std::unique_ptr<tape_interface> create_temp_tape();

  void generate_runs(tape_interface &input,
                     std::vector<std::unique_ptr<tape_interface>> &tapes,
                     std::vector<std::vector<std::size_t>> &run_lengths);

  void merge_pass(const std::vector<std::unique_ptr<tape_interface>> &src_tapes,
                  const std::vector<std::vector<std::size_t>> &src_runs,
                  std::vector<std::unique_ptr<tape_interface>> &dst_tapes,
                  std::vector<std::vector<std::size_t>> &dst_runs);

  void merge_tapes(
      const std::vector<tape_interface *> &tapes,
      const std::vector<std::vector<std::size_t>::const_iterator> &runs_beg,
      const std::vector<std::size_t> &runs_count, tape_interface &out,
      std::vector<std::size_t> &out_runs);

  void remove_temp_tape(tape_interface &tape);
};
