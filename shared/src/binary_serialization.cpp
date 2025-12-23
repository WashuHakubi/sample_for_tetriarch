/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <algorithm>


#include <shared/serialization.h>

namespace ew {
struct binary_writer final : writer {
  binary_writer() : buffer_(&internal_buffer_) {
  }

  binary_writer(std::vector<char>& buffer) : buffer_(&buffer) {
  }

  void begin_object(std::string_view name) override {
    // Binary layouts are flattened, so we don't preserve object notations.
  }

  void end_object() override {
  }

  void begin_array(std::string_view name, size_t count) override {
    // write the number of items in the array as a 7-bit encoded integer
    write_7bit_int(count);
  }

  void end_array() override {
  }

  void write(std::string_view name, bool value) override { write_primitive(static_cast<uint8_t>(value)); }

  void write(std::string_view name, uint8_t value) override { write_primitive(value); }
  void write(std::string_view name, uint16_t value) override { write_primitive(value); }
  void write(std::string_view name, uint32_t value) override { write_primitive(value); }
  void write(std::string_view name, uint64_t value) override { write_primitive(value); }

  void write(std::string_view name, int8_t value) override { write_primitive(value); }
  void write(std::string_view name, int16_t value) override { write_primitive(value); }
  void write(std::string_view name, int32_t value) override { write_primitive(value); }
  void write(std::string_view name, int64_t value) override { write_primitive(value); }

  void write_compressed(std::string_view name, uint16_t value) override { write_7bit_int(value); }

  void write_compressed(std::string_view name, uint32_t value) override { write_7bit_int(value); }

  void write_compressed(std::string_view name, uint64_t value) override { write_7bit_int(value); }

  void write(std::string_view name, float value) override { write_primitive(value); }

  void write(std::string_view name, double value) override { write_primitive(value); }

  void write(std::string_view name, std::string_view value) override {
    write_7bit_int(value.size());
    buffer_->reserve(buffer_->size() + value.size());
    buffer_->insert(buffer_->end(), value.begin(), value.end());
  }

  void to_buffer(std::vector<char>& buffer) const override {
    buffer.reserve(buffer.size() + buffer_->size());
    buffer.insert(buffer.end(), buffer_->begin(), buffer_->end());
  }

private:
  template <class T>
    requires(std::is_unsigned_v<T>)
  void write_7bit_int(T value) {
    // Write a 7-bit encoded integer to the buffer. A 7 bit encoded integer uses the high bit to indicate if there are
    // following bytes.
    while (value > 0) {
      // Get the last 7 bits to write
      auto byte = static_cast<uint8_t>(value & 0x7F);
      // Remove the last 7 bits from the number
      value >>= 7;

      // If we are still not 0 then set the high bit of the byte to indicate that there will be a following byte.
      if (value > 0) {
        byte |= 0x80;
      }

      // Write the byte out.
      buffer_->push_back(byte);
    }
  }

  template <class T>
  void write_primitive(T value) {
    auto bytes = reinterpret_cast<char*>(&value);
    buffer_->reserve(buffer_->size() + sizeof(T));
    buffer_->insert(buffer_->end(), bytes, bytes + sizeof(T));
  }

  std::vector<char> internal_buffer_;
  std::vector<char>* buffer_;
};

struct binary_reader final : reader {
  binary_reader(std::span<char const> buffer) : buffer_(buffer) {
  }

  void begin_object(std::string_view name) override {
  }

  void end_object() override {
  }

  void begin_array(std::string_view name, size_t& count) override {
    read_7bit_int(count);
  }

  void end_array() override {
  }

  void read(std::string_view name, bool& value) override {
    // sizeof(bool) may not be 1, so we encode it as a uint8_t to ensure that it is.
    uint8_t tmp;
    read_primitive(tmp);
    value = tmp != 0;
  }

  void read(std::string_view name, uint8_t& value) override { read_primitive(value); }
  void read(std::string_view name, uint16_t& value) override { read_primitive(value); }
  void read(std::string_view name, uint32_t& value) override { read_primitive(value); }
  void read(std::string_view name, uint64_t& value) override { read_primitive(value); }

  void read(std::string_view name, int8_t& value) override { read_primitive(value); }
  void read(std::string_view name, int16_t& value) override { read_primitive(value); }
  void read(std::string_view name, int32_t& value) override { read_primitive(value); }
  void read(std::string_view name, int64_t& value) override { read_primitive(value); }

  void read_compressed(std::string_view name, uint16_t& value) override { read_7bit_int(value); }
  void read_compressed(std::string_view name, uint32_t& value) override { read_7bit_int(value); }
  void read_compressed(std::string_view name, uint64_t& value) override { read_7bit_int(value); }

  void read(std::string_view name, float& value) override { read_primitive(value); }
  void read(std::string_view name, double& value) override { read_primitive(value); }

  void read(std::string_view name, std::string& value) override {
    size_t count;
    read_7bit_int(count);
    value.reserve(value.size() + count);
    value.insert(value.end(), buffer_.begin(), buffer_.begin() + count);
    buffer_ = buffer_.subspan(count);
  }

private:
  template <class T>
    requires(std::is_unsigned_v<T>)
  void read_7bit_int(T& value) {
    T v{};

    // buffer to store our full 7-bit integer in
    char decode_bytes[sizeof(T) + 1] = {};
    size_t pos = 0;

    // Read out the first byte of the integer
    assert(buffer_.size() >= 1);
    auto byte = buffer_[pos];

    // Store the 7 bits of the number
    decode_bytes[pos++] = byte & 0x7f;

    while (byte & 0x80) {
      // While the high bit is set we need to read more bytes.
      assert(buffer_.size() >= pos + 1);
      byte = buffer_[pos];
      decode_bytes[pos++] = byte & 0x7f;
    }

    // Re-encode the bytes back into the integer. The bytes are encoded from low to high, so we iterate backwards.
    for (auto i = pos; i > 0; --i) {
      v <<= 7;
      v |= decode_bytes[i - 1];
    }

    value = v;
    buffer_ = buffer_.subspan(pos);
  }

  template <class T>
  void read_primitive(T& value) {
    assert(buffer_.size() >= sizeof(T));

    auto bytes = reinterpret_cast<char*>(&value);

    // Copy the bytes out of the buffer into the destination value
    std::copy_n(buffer_.begin(), sizeof(T), bytes);

    // Remove the read portion
    buffer_ = buffer_.subspan(sizeof(T));
  }

  std::span<char const> buffer_;
};

std::shared_ptr<writer> create_binary_writer() {
  return std::make_shared<binary_writer>();
}

std::shared_ptr<writer> create_binary_writer(std::vector<char>& buffer) {
  return std::make_shared<binary_writer>(buffer);
}

std::shared_ptr<reader> create_binary_reader(std::span<char const> buffer) {
  return std::make_shared<binary_reader>(buffer);
}
}
