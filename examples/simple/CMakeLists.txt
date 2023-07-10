add_executable(simple main.cpp)

target_link_libraries(
  simple
  PRIVATE
    PowerSystem
    systemc-ams
    systemc
    spdlog::spdlog
  )
