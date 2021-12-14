set(CMAKE_CXX_STANDARD 11)

add_executable(test_nsg_index test_nsg_index.cpp)
# target_link_directories()
if(APPLE)
    target_link_libraries(test_nsg_index PRIVATE ${PROJECT_NAME} tcmalloc)
else()
    target_link_libraries(test_nsg_index ${PROJECT_NAME} -ltcmalloc)
endif()


add_executable(test_nsg_search test_nsg_search.cpp)
target_link_libraries(test_nsg_search PRIVATE ${PROJECT_NAME})


add_executable(test_nsg_optimized_search test_nsg_optimized_search.cpp)
if(APPLE)
    target_link_libraries(test_nsg_optimized_search PRIVATE ${PROJECT_NAME} tcmalloc)
else()
    target_link_libraries(test_nsg_optimized_search ${PROJECT_NAME} -ltcmalloc)
endif()

# message(STATUS ${CMAKE_BINARY_DIR}/src)
add_library(test_gidsne_index SHARED test_gidsne_index.cpp)
add_executable(test_gidsne_index_exe test_gidsne_index.cpp)
# target_link_directories(test_gidsne_index PUBLIC ${CMAKE_BINARY_DIR}/src)
target_link_libraries(test_gidsne_index PRIVATE ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}.a)
target_link_libraries(test_gidsne_index_exe PRIVATE ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}.a)
