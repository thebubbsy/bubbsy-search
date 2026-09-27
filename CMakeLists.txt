# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)
project(BubbsySearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
  message(FATAL_ERROR "Bubbsy Search targets Windows only.")
endif()

# ---- MinHook (inline hooking; replaces the old Windhawk runtime) ----
include(FetchContent)
FetchContent_Declare(
  minhook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(minhook)

add_library(bubbsy_common STATIC
  src/common/config.cpp
  src/common/everything_ipc.cpp)
target_link_libraries(bubbsy_common PUBLIC shlwapi propsys ole32 shell32)

# ---- bubbsy-hook.dll (GPLv3, injected into explorer.exe) ----
add_library(bubbsy-hook SHARED
  src/hookdll/dllmain.cpp
  src/hookdll/symbols.cpp)
target_link_libraries(bubbsy-hook PRIVATE bubbsy_common minhook dbghelp shlwapi propsys)
set_target_properties(bubbsy-hook PROPERTIES OUTPUT_NAME "bubbsy-hook" PREFIX "")

# ---- bubbsy-host.exe (tray + supervisor + injector) ----
add_executable(bubbsy-host WIN32
  src/host/main.cpp
  src/host/injector.cpp
  src/host/everything_manager.cpp
  src/host/autostart.cpp
  assets/host.rc)
target_link_libraries(bubbsy-host PRIVATE bubbsy_common shell32 advapi32)
set_target_properties(bubbsy-host PROPERTIES OUTPUT_NAME "bubbsy-host")
