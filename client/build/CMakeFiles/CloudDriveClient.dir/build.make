# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/chengxi/Cloud-Drive/client

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/chengxi/Cloud-Drive/client/build

# Include any dependencies generated for this target.
include CMakeFiles/CloudDriveClient.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/CloudDriveClient.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/CloudDriveClient.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/CloudDriveClient.dir/flags.make

CMakeFiles/CloudDriveClient.dir/client.cpp.o: CMakeFiles/CloudDriveClient.dir/flags.make
CMakeFiles/CloudDriveClient.dir/client.cpp.o: ../client.cpp
CMakeFiles/CloudDriveClient.dir/client.cpp.o: CMakeFiles/CloudDriveClient.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chengxi/Cloud-Drive/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/CloudDriveClient.dir/client.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/CloudDriveClient.dir/client.cpp.o -MF CMakeFiles/CloudDriveClient.dir/client.cpp.o.d -o CMakeFiles/CloudDriveClient.dir/client.cpp.o -c /home/chengxi/Cloud-Drive/client/client.cpp

CMakeFiles/CloudDriveClient.dir/client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/CloudDriveClient.dir/client.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/chengxi/Cloud-Drive/client/client.cpp > CMakeFiles/CloudDriveClient.dir/client.cpp.i

CMakeFiles/CloudDriveClient.dir/client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/CloudDriveClient.dir/client.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/chengxi/Cloud-Drive/client/client.cpp -o CMakeFiles/CloudDriveClient.dir/client.cpp.s

# Object files for target CloudDriveClient
CloudDriveClient_OBJECTS = \
"CMakeFiles/CloudDriveClient.dir/client.cpp.o"

# External object files for target CloudDriveClient
CloudDriveClient_EXTERNAL_OBJECTS =

CloudDriveClient: CMakeFiles/CloudDriveClient.dir/client.cpp.o
CloudDriveClient: CMakeFiles/CloudDriveClient.dir/build.make
CloudDriveClient: CMakeFiles/CloudDriveClient.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/chengxi/Cloud-Drive/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable CloudDriveClient"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/CloudDriveClient.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/CloudDriveClient.dir/build: CloudDriveClient
.PHONY : CMakeFiles/CloudDriveClient.dir/build

CMakeFiles/CloudDriveClient.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/CloudDriveClient.dir/cmake_clean.cmake
.PHONY : CMakeFiles/CloudDriveClient.dir/clean

CMakeFiles/CloudDriveClient.dir/depend:
	cd /home/chengxi/Cloud-Drive/client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chengxi/Cloud-Drive/client /home/chengxi/Cloud-Drive/client /home/chengxi/Cloud-Drive/client/build /home/chengxi/Cloud-Drive/client/build /home/chengxi/Cloud-Drive/client/build/CMakeFiles/CloudDriveClient.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/CloudDriveClient.dir/depend

