cmake_minimum_required(VERSION 3.8)
project(gcs_core)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# --- Блок компіляції та встановлення ---
add_executable(gcs_node src/gcs_node.cpp)
ament_target_dependencies(gcs_node rclcpp geometry_msgs std_srvs)

install(TARGETS
  gcs_node
  DESTINATION lib/${PROJECT_NAME}
)
# ---------------------------------------

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()