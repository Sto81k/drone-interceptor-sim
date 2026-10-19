cmake_minimum_required(VERSION 3.8)
project(interceptor_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Підключення базових пакетів
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(vision_msgs REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(gazebo_msgs REQUIRED)

# 2. Створення виконуваного файлу з нашого C++ коду
add_executable(interceptor_node src/interceptor_node.cpp)
ament_target_dependencies(interceptor_node 
  rclcpp 
  std_msgs 
  geometry_msgs 
  vision_msgs 
  mavros_msgs
  trajectory_msgs
  gazebo_msgs
)

# 3. Інструкція для встановлення
install(TARGETS
  interceptor_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
