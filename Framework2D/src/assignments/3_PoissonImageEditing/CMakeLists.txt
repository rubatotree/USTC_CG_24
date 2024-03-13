project(3_PoissonImageEditing)
file(GLOB source
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h" 
)
add_executable(${PROJECT_NAME} ${source} "image_clone_algorithms/point_set.h" "image_clone_algorithms/point_set.cpp" "image_clone_algorithms/image_clone.h" "image_clone_algorithms/paste.h" "image_clone_algorithms/paste.cpp" "image_clone_algorithms/seamless.h" "image_clone_algorithms/seamless.cpp" "image_clone_algorithms/mixed_gradients.h" "image_clone_algorithms/mixed_gradients.cpp" "image_clone_algorithms/image_clone.cpp")
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(${PROJECT_NAME} PROPERTIES 
  DEBUG_POSTFIX "_d"
  RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${LIBRARY_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${LIBRARY_DIR}") 
target_link_libraries(${PROJECT_NAME} PUBLIC view) 
target_compile_definitions(${PROJECT_NAME} PRIVATE -DDATA_PATH="${FRAMEWORK2D_DIR}/../Homeworks/3_poisson_image_editing/data")