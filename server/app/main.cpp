/*
 *  Copyright (c) 2025 Sean Kent. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <uv.h>

#include <iostream>
#include <mutex>
#include <ostream>
#include <span>
#include <thread>
#include <unordered_map>
#include <vector>

struct client_data {
  uv_tcp_t client;
  char* buffer;
};

void on_close(uv_handle_t* handle) {
  delete reinterpret_cast<uv_tcp_t*>(handle);
}

void alloc_buffer(uv_handle_t* handle, size_t size, uv_buf_t* buf) {
  auto client = static_cast<client_data*>(handle->data);
  buf->base = client->buffer = new char[size];
  buf->len = size;
}

void on_write(uv_write_t* req, int status) {
  if (status < 0) {
    std::cout << "Failed to write to client: " << uv_strerror(status) << std::endl;
  }

  delete req;
}

void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
  if (nread < 0) {
    if (nread != UV_EOF) {
      std::cout << "Failed to read from client: " << uv_strerror(nread) << std::endl;
    } else {
      std::cout << "Client disconnected" << std::endl;
    }
    uv_close(reinterpret_cast<uv_handle_t*>(client), on_close);
  }

  if (nread > 0) {
    auto write_req = new uv_write_t{};
    auto wrbuf = uv_buf_init(buf->base, nread);
    uv_write(write_req, client, &wrbuf, 1, on_write);
    std::cout << "Read " << nread << " bytes." << std::endl;
  }
}

void on_connected_ipv4(uv_stream_t* server, int status) {
  if (status < 0) {
    std::cout << "Error: " << uv_strerror(status) << std::endl;
    return;
  }

  auto client = new uv_tcp_t{};
  uv_tcp_init(server->loop, client);

  if (int err = uv_accept(server, reinterpret_cast<uv_stream_t*>(client)); err == 0) {
    if (err = uv_read_start(reinterpret_cast<uv_stream_t*>(client), alloc_buffer, on_read); err < 0) {
      std::cout << "Failed to start reading from client: " << uv_strerror(err) << std::endl;
      uv_close(reinterpret_cast<uv_handle_t*>(client), on_close);
    }
  } else {
    std::cout << "Failed to accept connection: " << uv_strerror(err) << std::endl;
    uv_close(reinterpret_cast<uv_handle_t*>(client), on_close);
  }
}

namespace ew {
void f();
}

int main() {
  ew::f();
  uv_tcp_t server;
  auto loop = uv_default_loop();

  sockaddr_in addr{};
  uv_ip4_addr("0.0.0.0", 8000, &addr);

  uv_tcp_init(loop, &server);
  uv_tcp_bind(&server, reinterpret_cast<sockaddr*>(&addr), 0);

  int r = uv_listen(reinterpret_cast<uv_stream_t*>(&server), 10, on_connected_ipv4);

  if (r) {
    std::cout << "Error: " << uv_strerror(r) << std::endl;
  }

  return uv_run(loop, UV_RUN_DEFAULT);
}