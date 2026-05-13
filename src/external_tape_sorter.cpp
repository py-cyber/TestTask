#include "external_tape_sorter.h"
#include "file_tape.h"
#include <algorithm>
#include <filesystem>

external_tape_sorter::external_tape_sorter(const tape_config &cfg)
    : cfg_(cfg), tape_counter_(0) {}

void external_tape_sorter::sort(tape_interface &input, tape_interface &output) {
  std::vector<std::unique_ptr<tape_interface>> tapes;
  std::vector<std::vector<std::size_t>> run_lengths;

  generate_runs(input, tapes, run_lengths);

  if (tapes.empty()) {
    return;
  }

  if (tapes.size() == 1) {
    tapes[0]->rewind();
    int32_t val;
    while (tapes[0]->read(val))
      output.write(val);
  } else {
    while (tapes.size() > 1) {
      std::vector<std::unique_ptr<tape_interface>> next_tapes;
      std::vector<std::vector<std::size_t>> next_runs;
      merge_pass(tapes, run_lengths, next_tapes, next_runs);
      tapes = std::move(next_tapes);
      run_lengths = std::move(next_runs);
    }

    tapes[0]->rewind();
    int32_t val;
    while (tapes[0]->read(val))
      output.write(val);
  }

  output.rewind();
}

void external_tape_sorter::generate_runs(
    tape_interface &input, std::vector<std::unique_ptr<tape_interface>> &tapes,
    std::vector<std::vector<std::size_t>> &run_lengths) {
  std::size_t chunk_size =
      std::max(std::size_t(1), cfg_.memory_limit_bytes / sizeof(int32_t));
  std::vector<int32_t> buffer;
  buffer.reserve(chunk_size);

  int32_t val;
  while (input.read(val)) {
    buffer.push_back(val);
    if (buffer.size() == chunk_size) {
      std::sort(buffer.begin(), buffer.end());
      auto temp = create_temp_tape();
      std::vector<std::size_t> runs;
      for (auto v : buffer)
        temp->write(v);
      runs.push_back(buffer.size());
      tapes.push_back(std::move(temp));
      run_lengths.push_back(std::move(runs));
      buffer.clear();
    }
  }
  if (!buffer.empty()) {
    std::sort(buffer.begin(), buffer.end());
    auto temp = create_temp_tape();
    std::vector<std::size_t> runs;
    for (auto v : buffer)
      temp->write(v);
    runs.push_back(buffer.size());
    tapes.push_back(std::move(temp));
    run_lengths.push_back(std::move(runs));
  }
}

void external_tape_sorter::merge_pass(
    const std::vector<std::unique_ptr<tape_interface>> &src_tapes,
    const std::vector<std::vector<std::size_t>> &src_runs,
    std::vector<std::unique_ptr<tape_interface>> &dst_tapes,
    std::vector<std::vector<std::size_t>> &dst_runs) {
  for (std::size_t i = 0; i < src_tapes.size(); i += 2) {
    auto out = create_temp_tape();
    std::vector<std::size_t> out_runs;

    if (i + 1 < src_tapes.size()) {
      merge_two_tapes(*src_tapes[i], src_runs[i], *src_tapes[i + 1],
                      src_runs[i + 1], *out, out_runs);
    } else {
      src_tapes[i]->rewind();
      for (std::size_t run : src_runs[i]) {
        out_runs.push_back(run);
        int32_t v;
        for (std::size_t k = 0; k < run && src_tapes[i]->read(v); ++k) {
          out->write(v);
        }
      }
    }
    dst_tapes.push_back(std::move(out));
    dst_runs.push_back(std::move(out_runs));
  }
}

void external_tape_sorter::merge_two_tapes(tape_interface &t1,
                                           const std::vector<std::size_t> &r1,
                                           tape_interface &t2,
                                           const std::vector<std::size_t> &r2,
                                           tape_interface &out,
                                           std::vector<std::size_t> &out_runs) {
  t1.rewind();
  t2.rewind();

  std::size_t max_runs = std::max(r1.size(), r2.size());
  for (std::size_t k = 0; k < max_runs; ++k) {
    std::size_t len1 = (k < r1.size()) ? r1[k] : 0;
    std::size_t len2 = (k < r2.size()) ? r2[k] : 0;
    out_runs.push_back(len1 + len2);

    std::size_t c1 = 0, c2 = 0;
    int32_t v1 = 0, v2 = 0;
    bool has1 = (len1 > 0) && t1.read(v1);
    bool has2 = (len2 > 0) && t2.read(v2);

    for (std::size_t m = 0; m < len1 + len2; ++m) {
      bool take1 = false;
      if (has1 && has2)
        take1 = (v1 <= v2);
      else if (has1)
        take1 = true;

      if (take1) {
        out.write(v1);
        ++c1;
        has1 = (c1 < len1) ? t1.read(v1) : false;
      } else {
        out.write(v2);
        ++c2;
        has2 = (c2 < len2) ? t2.read(v2) : false;
      }
    }
  }
}

std::unique_ptr<tape_interface> external_tape_sorter::create_temp_tape() {
  std::string filepath = "tmp/tape_" + std::to_string(tape_counter_++) + ".bin";
  return std::make_unique<file_tape>(filepath, cfg_, true);
}
