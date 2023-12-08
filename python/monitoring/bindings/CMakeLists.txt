find_package(pybind11 REQUIRED)

list(APPEND monitoring_python_files monitoring_python.cc)
pybind11_add_module(monitoring_python ${monitoring_python_files})
target_link_libraries(
  monitoring_python PRIVATE ${Boost_LIBRARIES} pybind11::pybind11 Python::Module
                         Python::NumPy gnuradio-monitoring)
target_include_directories(monitoring_python
   PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../../include
)

# copy in bindings .so file for use in QA test module
# add_custom_target(
#   copy_monitoring_bindings_for_tests ALL
#   COMMAND
#     ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/*.so"
#     ${CMAKE_BINARY_DIR}/test_modules/gnuradio/monitoring/
#   DEPENDS monitoring_python)

install(TARGETS monitoring_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/monitoring COMPONENT pythonapi)
