cmake_minimum_required(VERSION 3.10)
project(templ)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(templ main.cpp)

if(UNIX OR APPLE)
    find_package(Curses REQUIRED)
    include_directories(${CURSES_INCLUDE_DIR})
    target_link_libraries(templ PRIVATE ${CURSES_LIBRARIES})
elseif(WIN32)
    if(EXISTS "${CMAKE_SOURCE_DIR}/PDCurses/include/curses.h")
        include_directories(${CMAKE_SOURCE_DIR}/PDCurses/include)
        add_library(pdcurses STATIC IMPORTED)
        set_target_properties(pdcurses PROPERTIES
            IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/PDCurses/build/pdcurses.lib
        )
        target_link_libraries(templ pdcurses)
    else()
        message(FATAL_ERROR "PDCurses not found. Ensure PDCurses is correctly set up in the project.")
    endif()
endif()

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
