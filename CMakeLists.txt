cmake_minimum_required(VERSION 3.15)
project(SudokuSolver)

set(CMAKE_CXX_STANDARD 17)

# Szukamy OpenCV (to akurat działa normalnie)
find_package(OpenCV REQUIRED)

# Szukamy Tesseracta przez PkgConfig (sposób MSYS2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Tesseract REQUIRED tesseract)

add_executable(SudokuSolver src/main.cpp)

# Dodajemy nagłówki Tesseracta i linkujemy wszystko
target_include_directories(SudokuSolver PRIVATE ${Tesseract_INCLUDE_DIRS})
target_link_libraries(SudokuSolver PRIVATE 
    ${OpenCV_LIBS} 
    ${Tesseract_LIBRARIES}
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/testcase1 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/testcase2 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})