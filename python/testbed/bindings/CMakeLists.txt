find_package(pybind11 REQUIRED)

list(APPEND testbed_python_files
   phy_converge_python.cc
   packet_validator_python.cc
   testbed_python.cc
   log_python.cc)

pybind11_add_module(testbed_python ${testbed_python_files})

target_link_libraries(
   testbed_python PRIVATE ${Boost_LIBRARIES} pybind11::pybind11 Python::Module
                         Python::NumPy dtl-testbed)
target_include_directories(testbed_python
   PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

install(TARGETS testbed_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/testbed COMPONENT pythonapi)
