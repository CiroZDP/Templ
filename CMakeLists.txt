# Probably overkill for a simple project, but I'd rather this than a "compile.sh" script :/
cmake_minimum_required(VERSION 3.10)
project(templ)
set(CMAKE_CXX_STANDARD 20)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(templ main.cpp)
target_link_libraries(templ ${CURSES_LIBRARIES})

# Install the executable
install(TARGETS templ
    RUNTIME DESTINATION bin
)

# Install templates directory with its contents
install(DIRECTORY templates/
    DESTINATION share/templ/templates
    PATTERN "*"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

# Uninstall templ
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_INSTALL_PREFIX}/share/templ
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_INSTALL_PREFIX}/bin/templ
)