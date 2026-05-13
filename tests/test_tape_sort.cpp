#include "external_tape_sorter.h"
#include "file_tape.h"
#include "tape_config.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <numeric>
#include <random>
#include <vector>

class tape_sort_test : public ::testing::Test {
protected:
  void SetUp() override {
    std::filesystem::create_directories("tmp");
    cfg_.read_delay_ms = 0;
    cfg_.write_delay_ms = 0;
    cfg_.rewind_delay_ms = 0;
    cfg_.step_delay_ms = 0;
    cfg_.memory_limit_bytes = 1024 * 1024;
    cfg_.temp_tapes_count = 2;
  }
  void TearDown() override { std::filesystem::remove_all("tmp"); }

  tape_config cfg_;
  const std::string tmp_dir_ = "tmp/test_";

  void run_sort_and_verify(std::vector<int32_t> data, std::size_t memory_limit,
                           const std::string &test_name) {
    cfg_.memory_limit_bytes = memory_limit;
    std::string in_path = tmp_dir_ + test_name + "_in.bin";
    std::string out_path = tmp_dir_ + test_name + "_out.bin";

    {
      file_tape in(in_path, cfg_, true);
      for (auto v : data) {
        in.write(v);
        in.moveForward();
      }
    }

    file_tape in(in_path, cfg_, false);
    file_tape out(out_path, cfg_, true);
    external_tape_sorter sorter(cfg_);
    ASSERT_NO_THROW(sorter.sort(in, out));

    out.rewind();
    std::vector<int32_t> result;
    int32_t val;
    while (out.read(val)) {
      result.push_back(val);
      out.moveForward();
    }

    std::sort(data.begin(), data.end());
    ASSERT_EQ(result.size(), data.size()) << "Size mismatch in " << test_name;
    ASSERT_EQ(result, data) << "Content mismatch in " << test_name;
  }
};

TEST_F(tape_sort_test, handles_empty_input) {
  run_sort_and_verify({}, 1024, "empty");
}
TEST_F(tape_sort_test, sorts_small_dataset_fits_in_memory) {
  run_sort_and_verify({5, 3, 8, 1, 9, 0, -2, 4}, 1024 * 1024, "small");
}
TEST_F(tape_sort_test, sorts_large_dataset_forces_external_sort) {
  std::vector<int32_t> data;
  std::mt19937 gen(42);
  std::uniform_int_distribution<int32_t> dist(-10000, 10000);
  for (int i = 0; i < 500; ++i)
    data.push_back(dist(gen));
  run_sort_and_verify(std::move(data), 50 * sizeof(int32_t), "large_ext");
}
TEST_F(tape_sort_test, handles_duplicates_and_negatives) {
  run_sort_and_verify({1, 1, -5, -5, 0, 0, 99, 99, -10, -10},
                      32 * sizeof(int32_t), "dup_neg");
}
TEST_F(tape_sort_test, sorts_already_sorted_data) {
  std::vector<int32_t> data(200);
  std::iota(data.begin(), data.end(), -100);
  run_sort_and_verify(std::move(data), 20 * sizeof(int32_t), "already_sorted");
}
TEST_F(tape_sort_test, sorts_reverse_sorted_data) {
  std::vector<int32_t> data(200);
  std::iota(data.rbegin(), data.rend(), -100);
  run_sort_and_verify(std::move(data), 20 * sizeof(int32_t), "reverse_sorted");
}
TEST_F(tape_sort_test, sorts_all_identical_elements) {
  std::vector<int32_t> data(1000, 42);
  run_sort_and_verify(std::move(data), 12 * sizeof(int32_t), "identical");
}
TEST_F(tape_sort_test, sorts_with_memory_limit_one_element) {
  std::vector<int32_t> data(100);
  std::mt19937 gen(99);
  std::uniform_int_distribution<int32_t> dist(-1000, 1000);
  for (auto &v : data)
    v = dist(gen);
  run_sort_and_verify(std::move(data), sizeof(int32_t), "tiny_mem_1");
}
TEST_F(tape_sort_test, sorts_with_memory_limit_two_elements) {
  std::vector<int32_t> data(300);
  std::mt19937 gen(123);
  std::uniform_int_distribution<int32_t> dist(-5000, 5000);
  for (auto &v : data)
    v = dist(gen);
  run_sort_and_verify(std::move(data), 2 * sizeof(int32_t), "tiny_mem_2");
}
TEST_F(tape_sort_test, sorts_alternating_extremes) {
  std::vector<int32_t> data;
  int32_t low = -1000, high = 1000;
  for (int i = 0; i < 150; ++i) {
    data.push_back(low++);
    data.push_back(high--);
  }
  run_sort_and_verify(std::move(data), 16 * sizeof(int32_t), "alternating");
}
TEST_F(tape_sort_test, sorts_large_dataset_multiple_merge_passes) {
  std::vector<int32_t> data(2000);
  std::mt19937 gen(2023);
  std::uniform_int_distribution<int32_t> dist(-100000, 100000);
  for (auto &v : data)
    v = dist(gen);
  run_sort_and_verify(std::move(data), 10 * sizeof(int32_t), "multi_pass");
}
TEST_F(tape_sort_test, sorts_exact_memory_boundary) {
  std::vector<int32_t> data = {7, 2, 9, 1, 5, 3, 8, 4, 6, 0};
  std::size_t exact_mem = data.size() * sizeof(int32_t);
  run_sort_and_verify(std::move(data), exact_mem, "exact_boundary");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
