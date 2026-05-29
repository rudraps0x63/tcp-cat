cmake_minimum_required(VERSION 4.0)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)

project(tcp_cat CXX C)

add_bare_module(tcp_cat)

target_include_directories(${tcp_cat} PRIVATE "include/"
                                              "util/")

target_sources(
  ${tcp_cat}
  PRIVATE
  binding.cc
)
