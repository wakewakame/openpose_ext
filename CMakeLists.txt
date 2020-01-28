cmake_minimum_required(VERSION 3.10)

# download openpose
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/3rdparty")
if (NOT EXISTS ${LIBRARIES_PATH})
	message(STATUS "create directory : ${LIBRARIES_PATH}")
	file(MAKE_DIRECTORY "${LIBRARIES_PATH}")
endif (NOT EXISTS ${LIBRARIES_PATH})
set(OPENPOSE_URL "https://github.com/CMU-Perceptual-Computing-Lab/openpose/releases/download/v1.5.1/openpose-1.5.1-binaries-win64-gpu-python-flir-3d_recommended.zip")
message(STATUS OPENPOSE_DOWNLOAD_LOG)
get_filename_component(OPENPOSE_ZIP_PATH "${OPENPOSE_URL}" NAME)
set(OPENPOSE_ZIP_PATH "${LIBRARIES_PATH}/${OPENPOSE_ZIP_PATH}")
string(REGEX REPLACE "\\.[^.]*$" "" OPENPOSE_DIR_PATH ${OPENPOSE_ZIP_PATH})
if (NOT EXISTS ${OPENPOSE_DIR_PATH})
	if (NOT EXISTS ${OPENPOSE_ZIP_PATH})
		message(FATAL_ERROR "error : ${OPENPOSE_ZIP_PATH} was not found.\nPlease download ${OPENPOSE_URL} and copy it to ${OPENPOSE_ZIP_PATH}")
	endif (NOT EXISTS ${OPENPOSE_ZIP_PATH})
	message(STATUS "unzip : ${OPENPOSE_ZIP_PATH}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${OPENPOSE_ZIP_PATH} WORKING_DIRECTORY ${LIBRARIES_PATH}) 
	message(STATUS "download : models")
	set(OPENPOSE_MODELS_PATH "${OPENPOSE_DIR_PATH}/openpose/models")
	set(OPENPOSE_MODELS_DOWNLOADER_PATH "${OPENPOSE_MODELS_PATH}/getModels.bat")
	execute_process(COMMAND ${OPENPOSE_MODELS_DOWNLOADER_PATH} WORKING_DIRECTORY ${OPENPOSE_MODELS_PATH})
endif (NOT EXISTS ${OPENPOSE_DIR_PATH})

# create project
project(openpose_ext)

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
message(STATUS "search : all .cpp files")
file(GLOB_RECURSE ALL_CPP_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(openpose_ext "${ALL_CPP_FILES}")
target_include_directories(openpose_ext PRIVATE
	"${CMAKE_SOURCE_DIR}/include"
	"${OPENPOSE_DIR_PATH}/openpose/include"
)
file(GLOB OPENPOSE_LIB_FILES "${OPENPOSE_DIR_PATH}/openpose/lib/*.lib")
target_link_libraries(openpose_ext ${OPENPOSE_LIB_FILES})
set_target_properties(openpose_ext PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:openpose_ext>")
