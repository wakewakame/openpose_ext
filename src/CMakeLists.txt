# configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# copy necessary files
message(STATUS "copy : dll files")
file(GLOB DLL_FILES "${OPENPOSE_DIR_PATH}/openpose/bin/*")
file(COPY ${DLL_FILES} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "copy : model files")
file(COPY "${OPENPOSE_DIR_PATH}/openpose/models" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# add source files
add_executable(openpose_ext main.cpp MinOpenPose.cpp)
target_include_directories(openpose_ext PRIVATE
	"${CMAKE_SOURCE_DIR}/include"
	"${OPENPOSE_DIR_PATH}/openpose/include"
)
file(GLOB OPENPOSE_LIB_FILES "${OPENPOSE_DIR_PATH}/openpose/lib/*.lib")
target_link_libraries(openpose_ext ${OPENPOSE_LIB_FILES})
set_target_properties(openpose_ext PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:openpose_ext>")
