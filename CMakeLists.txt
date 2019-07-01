cmake_minimum_required(VERSION 3.2)

if ($ENV{TRAVIS_BUILD_NUMBER})
    project(bulkmt VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})
else($ENV{TRAVIS_BUILD_NUMBER}) 
    project(bulkmt VERSION 0.0.1)
endif($ENV{TRAVIS_BUILD_NUMBER})

find_package(Threads)


add_library(definitions        bulkmt_processor.cpp
                               bulkmt.hpp)

add_executable(${PROJECT_NAME} bulkmt.cpp)

set_target_properties(${PROJECT_NAME}  PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
    
)

target_link_libraries(${PROJECT_NAME}
                      ${CMAKE_THREAD_LIBS_INIT}
                      definitions
)

install(TARGETS definitions             DESTINATION bin)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)



set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_CONTACT arcadii_ru@mail.ru)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
include(CPack)
