#include "external_tape_sorter.h"
#include "file_tape.h"
#include <algorithm>
#include <filesystem>
#include <queue>
#include <stdexcept>

external_tape_sorter::external_tape_sorter(const tape_config &cfg)
    : cfg_(cfg), tape_counter_(0) {}

void external_tape_sorter::sort(tape_interface &input, tape_interface &output) {
  std::vector<std::unique_ptr<tape_interface>> tapes;
  std::vector<std::vector<std::size_t>> run_lengths;

  generate_runs(input, tapes, run_lengths);
  if (tapes.empty())
    return;

  while (tapes.size() > 1) {
    std::vector<std::unique_ptr<tape_interface>> next_tapes;
    std::vector<std::vector<std::size_t>> next_runs;
    merge_pass(tapes, run_lengths, next_tapes, next_runs);

    for (auto &t : tapes)
      remove_temp_tape(*t);
    tapes = std::move(next_tapes);
    run_lengths = std::move(next_runs);
  }

  tapes[0]->rewind();
  int32_t val;
  while (tapes[0]->read(val)) {
    output.write(val);
    output.moveForward();
    tapes[0]->moveForward();
  }
  output.rewind();

  remove_temp_tape(*tapes[0]);
}

void external_tape_sorter::generate_runs(
    tape_interface &input, std::vector<std::unique_ptr<tape_interface>> &tapes,
    std::vector<std::vector<std::size_t>> &run_lengths) {
  const std::size_t chunk_size =
      std::max(std::size_t(1), cfg_.memory_limit_bytes / sizeof(int32_t));
  std::vector<int32_t> buffer;
  buffer.reserve(chunk_size);

  int32_t val;
  while (input.read(val)) {
    buffer.push_back(val);
    if (buffer.size() == chunk_size) {
      std::sort(buffer.begin(), buffer.end());
      auto temp = create_temp_tape();
      for (auto v : buffer) {
        temp->write(v);
        temp->moveForward();
      }
      run_lengths.push_back({buffer.size()});
      tapes.push_back(std::move(temp));
      buffer.clear();
    }
    input.moveForward();
  }

  if (!buffer.empty()) {
    std::sort(buffer.begin(), buffer.end());
    auto temp = create_temp_tape();
    for (auto v : buffer) {
      temp->write(v);
      temp->moveForward();
    }
    run_lengths.push_back({buffer.size()});
    tapes.push_back(std::move(temp));
  }
}

void external_tape_sorter::merge_pass(
    const std::vector<std::unique_ptr<tape_interface>> &src_tapes,
    const std::vector<std::vector<std::size_t>> &src_runs,
    std::vector<std::unique_ptr<tape_interface>> &dst_tapes,
    std::vector<std::vector<std::size_t>> &dst_runs) {
  const std::size_t K = std::max(1, cfg_.temp_tapes_count);
  for (std::size_t i = 0; i < src_tapes.size(); i += K) {
    std::vector<tape_interface *> group_tapes;
    std::vector<std::vector<std::size_t>::const_iterator> group_beg;
    std::vector<std::size_t> group_counts;
    for (std::size_t j = i; j < std::min(i + K, src_tapes.size()); ++j) {
      group_tapes.push_back(src_tapes[j].get());
      group_beg.push_back(src_runs[j].cbegin());
      group_counts.push_back(src_runs[j].size());
    }
    auto out = create_temp_tape();
    std::vector<std::size_t> out_runs;
    merge_tapes(group_tapes, group_beg, group_counts, *out, out_runs);
    dst_tapes.push_back(std::move(out));
    dst_runs.push_back(std::move(out_runs));
  }
}

void external_tape_sorter::merge_tapes(
    const std::vector<tape_interface *> &tapes,
    const std::vector<std::vector<std::size_t>::const_iterator> &runs_beg,
    const std::vector<std::size_t> &runs_count, tape_interface &out,
    std::vector<std::size_t> &out_runs) {
  const std::size_t T = tapes.size();
  std::vector<std::size_t> run_idx(T, 0);
  std::vector<std::size_t> remaining(T, 0);
  std::vector<int32_t> cur_val(T, 0);
  std::vector<bool> has_val(T, false);

  for (auto t : tapes)
    t->rewind();

  bool any_run_left = true;
  while (any_run_left) {
    any_run_left = false;
    for (std::size_t i = 0; i < T; ++i) {
      if (remaining[i] == 0 && run_idx[i] < runs_count[i]) {
        remaining[i] = *(runs_beg[i] + run_idx[i]);
        ++run_idx[i];
        if (remaining[i] > 0) {
          has_val[i] = tapes[i]->read(cur_val[i]);
          tapes[i]->moveForward();
        } else {
          has_val[i] = false;
        }
        any_run_left = true;
      } else if (run_idx[i] < runs_count[i]) {
        any_run_left = true;
      }
    }
    if (!any_run_left)
      break;

    std::size_t merged_len = 0;
    for (std::size_t i = 0; i < T; ++i)
      if (remaining[i] > 0)
        merged_len += remaining[i];

    out_runs.push_back(merged_len);

    auto cmp = [](const std::pair<int32_t, std::size_t> &a,
                  const std::pair<int32_t, std::size_t> &b) {
      return a.first > b.first;
    };
    std::priority_queue<std::pair<int32_t, std::size_t>,
                        std::vector<std::pair<int32_t, std::size_t>>,
                        decltype(cmp)>
        pq(cmp);
    for (std::size_t i = 0; i < T; ++i) {
      if (has_val[i])
        pq.emplace(cur_val[i], i);
    }

    while (!pq.empty()) {
      auto [val, i] = pq.top();
      pq.pop();
      out.write(val);
      out.moveForward();
      --remaining[i];
      if (remaining[i] > 0) {
        has_val[i] = tapes[i]->read(cur_val[i]);
        tapes[i]->moveForward();
        if (has_val[i])
          pq.emplace(cur_val[i], i);
      } else {
        has_val[i] = false;
      }
    }
  }
}

std::unique_ptr<tape_interface> external_tape_sorter::create_temp_tape() {
  std::string filepath = "tmp/tape_" + std::to_string(tape_counter_++) + ".bin";
  return std::make_unique<file_tape>(filepath, cfg_, true);
}

void external_tape_sorter::remove_temp_tape(tape_interface &tape) {
  try {
    std::filesystem::remove(tape.name());
  } catch (const std::filesystem::filesystem_error &) {
  }
}
