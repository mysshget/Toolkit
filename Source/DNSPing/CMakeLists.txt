# This code is part of Toolkit(DNSPing)
# DNSPing, a useful and powerful toolkit
# Copyright (C) 2014-2018 Chengr28
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


# CMake minimum version
cmake_minimum_required(VERSION 3.1)

# Project name and build type
project(DNSPing)
set(CMAKE_BUILD_TYPE "Release")

# All source files
set(MAIN_SRC_FILES
		"Console.h"
		"Definition.h"
		"DNSPing.h"
		"Include.h"
		"Initialization.h"
		"Platform.h"
		"Process.h"
		"Protocol.h"
		"ReadCommand.h"
		"Resolver.h"
		"Structure.h"
		"Terminal.h"
		"Type.h"
		"Console.cpp"
		"DNSPing.cpp"
		"Initialization.cpp"
		"Process.cpp"
		"Protocol.cpp"
		"ReadCommand.cpp"
		"Resolver.cpp"
		"Terminal.cpp")

# Compiling and linking flags
## C++ 14 standard definitions
set(CMAKE_CXX_STANDARD 14)
## Platform definitions
IF(PLATFORM_OPENWRT)
	add_definitions(-DPLATFORM_OPENWRT)
ELSE(PLATFORM_OPENWRT)
	add_definitions(-flto -fpic)
ENDIF(PLATFORM_OPENWRT)

# Executable file name
add_executable(DNSPing ${MAIN_SRC_FILES})
IF(PLATFORM_OPENWRT)
	INSTALL(TARGETS DNSPing RUNTIME DESTINATION sbin)
ENDIF(PLATFORM_OPENWRT)
