# MIT License
#
# Copyright (c) 2024 Kritzel Kratzel.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in 
# the Software without restriction, including without limitation the rights to 
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all 
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# ------------------------------------------------------------------------------
# luaping DLL
add_library(luaping SHARED)
# setup lua include directory
target_include_directories(luaping PRIVATE ${LIBLUA_INCLUDEDIR})
# setup platform-specific sources, compile and linker options
if(WIN32 AND NOT MinGW)
  target_compile_options(luaping PRIVATE /D_WINDLL /D_WIN32 /D_CRT_SECURE_NO_WARNINGS)
  target_link_options(luaping PRIVATE /LIBPATH:${LIBLUA_LIBDIR}
    liblua.lib iphlpapi.lib ws2_32.lib ntdll.lib)
else()
  message(FATAL_ERROR "Not yet fully implemented.")
endif()
# plattform-independend sources
target_sources(luaping PRIVATE luaping.c)
# Install
install(TARGETS luaping RUNTIME DESTINATION ${INSTALL_TOP_CDIR})
