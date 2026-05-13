#pragma once
#include <cstddef>
#include <cstdint>

class tape_interface {
public:
  virtual ~tape_interface() = default;
  virtual bool read(int32_t &value) = 0;
  virtual bool write(int32_t value) = 0;
  virtual void rewind() = 0;
  virtual void moveForward() = 0;
  virtual void moveBackward() = 0;
  virtual std::size_t length() const = 0;
  virtual std::size_t position() const = 0;
  virtual const char *name() const = 0;
};