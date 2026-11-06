cmake_minimum_required(VERSION 3.16)

project(student_demo CXX)

set(CMAKE_CXX_STANDARD 17)

# 把 student.cpp 编成静态库 libStudent.a
add_library(Student STATIC
    tools/student.cpp
)

# 头文件在 includes/ 下面
target_include_directories(Student PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)

add_executable(main
    src/main.cpp
)

# main 链接静态库
target_link_libraries(main PRIVATE Student)
