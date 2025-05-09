cmake_minimum_required(VERSION 3.10)
project(templ)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find ncurses (terminal UI)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# Define the executable
add_executable(templ main.cpp)

# Link to curses (note: modern CMake uses target_include_directories/target_link_libraries)
target_link_libraries(templ PRIVATE ${CURSES_LIBRARIES})

# Install the executable to bin/
install(TARGETS templ
    RUNTIME DESTINATION bin
)

# Install the templates directory recursively
install(DIRECTORY templates/
    DESTINATION share/templ/templates
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# Define uninstall target
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -E remove_directory "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/templ"
    COMMAND ${CMAKE_COMMAND} -E remove "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/templ"
)
