# CMake generated Testfile for 
# Source directory: /Users/shayaansiddique/Bhole
# Build directory: /Users/shayaansiddique/Bhole/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[physics_tests]=] "/Users/shayaansiddique/Bhole/build/bh_tests")
set_tests_properties([=[physics_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/shayaansiddique/Bhole/CMakeLists.txt;35;add_test;/Users/shayaansiddique/Bhole/CMakeLists.txt;0;")
subdirs("_deps/glm-build")
subdirs("_deps/doctest-build")
subdirs("_deps/glfw-build")
