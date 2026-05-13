#pragma once
#include "tape_config.h"
#include "tape_interface.h"
#include <memory>
#include <vector>

class external_tape_sorter {
public:
  explicit external_tape_sorter(const tape_config &cfg);
  void sort(tape_interface &input, tape_interface &output);

private:
  tape_config cfg_;
  std::size_t tape_counter_ = 0;

  void generate_runs(tape_interface &input,
                     std::vector<std::unique_ptr<tape_interface>> &tapes,
                     std::vector<std::vector<std::size_t>> &run_lengths);
  void merge_pass(const std::vector<std::unique_ptr<tape_interface>> &src_tapes,
                  const std::vector<std::vector<std::size_t>> &src_runs,
                  std::vector<std::unique_ptr<tape_interface>> &dst_tapes,
                  std::vector<std::vector<std::size_t>> &dst_runs);
  void merge_two_tapes(tape_interface &t1, const std::vector<std::size_t> &r1,
                       tape_interface &t2, const std::vector<std::size_t> &r2,
                       tape_interface &out, std::vector<std::size_t> &out_runs);
  std::unique_ptr<tape_interface> create_temp_tape();
};
