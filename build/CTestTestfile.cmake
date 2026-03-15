# CMake generated Testfile for 
# Source directory: C:/WetQuill_project/TheChlorophyllProtocol
# Build directory: C:/WetQuill_project/TheChlorophyllProtocol/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[DeterminismSmoke]=] "C:/WetQuill_project/TheChlorophyllProtocol/build/Debug/determinism_smoke_test.exe")
  set_tests_properties([=[DeterminismSmoke]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;80;add_test;C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[DeterminismSmoke]=] "C:/WetQuill_project/TheChlorophyllProtocol/build/Release/determinism_smoke_test.exe")
  set_tests_properties([=[DeterminismSmoke]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;80;add_test;C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[DeterminismSmoke]=] "C:/WetQuill_project/TheChlorophyllProtocol/build/MinSizeRel/determinism_smoke_test.exe")
  set_tests_properties([=[DeterminismSmoke]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;80;add_test;C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[DeterminismSmoke]=] "C:/WetQuill_project/TheChlorophyllProtocol/build/RelWithDebInfo/determinism_smoke_test.exe")
  set_tests_properties([=[DeterminismSmoke]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;80;add_test;C:/WetQuill_project/TheChlorophyllProtocol/CMakeLists.txt;0;")
else()
  add_test([=[DeterminismSmoke]=] NOT_AVAILABLE)
endif()
