cmake_minimum_required(VERSION 3.10)

# download openpose
set(LIBRARIES_PATH "${CMAKE_SOURCE_DIR}/3rdparty")
if (NOT EXISTS ${LIBRARIES_PATH})
	message(STATUS "create directory : ${LIBRARIES_PATH}")
	file(MAKE_DIRECTORY "${LIBRARIES_PATH}")
endif (NOT EXISTS ${LIBRARIES_PATH})
set(OPENPOSE_URL "https://github.com/CMU-Perceptual-Computing-Lab/openpose/releases/download/v1.5.1/openpose-1.5.1-binaries-win64-gpu-python-flir-3d_recommended.zip")
get_filename_component(OPENPOSE_ZIP_PATH "${OPENPOSE_URL}" NAME)
set(OPENPOSE_ZIP_PATH "${LIBRARIES_PATH}/${OPENPOSE_ZIP_PATH}")
string(REGEX REPLACE "\\.[^.]*$" "" OPENPOSE_DIR_PATH ${OPENPOSE_ZIP_PATH})
if (NOT EXISTS ${OPENPOSE_DIR_PATH})
	if (NOT EXISTS ${OPENPOSE_ZIP_PATH})
		message(STATUS "download : ${OPENPOSE_URL}")
		message(STATUS "it takes long time...")
		file(TO_NATIVE_PATH ${OPENPOSE_ZIP_PATH} OPENPOSE_ZIP_PATH_BACKSLASH)
		execute_process(COMMAND cmd /c start /wait bitsadmin /transfer openposeDownloadJob /dynamic /download /priority FOREGROUND ${OPENPOSE_URL} ${OPENPOSE_ZIP_PATH_BACKSLASH} WORKING_DIRECTORY ${LIBRARIES_PATH})
	endif (NOT EXISTS ${OPENPOSE_ZIP_PATH})
	if (NOT EXISTS ${OPENPOSE_ZIP_PATH})
		message(FATAL_ERROR "error : download was failed")
	endif (NOT EXISTS ${OPENPOSE_ZIP_PATH})
	message(STATUS "unzip : ${OPENPOSE_ZIP_PATH}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${OPENPOSE_ZIP_PATH} WORKING_DIRECTORY ${LIBRARIES_PATH}) 
	message(STATUS "download : models")
	set(OPENPOSE_MODELS_PATH "${OPENPOSE_DIR_PATH}/openpose/models")
	set(OPENPOSE_MODELS_DOWNLOADER_PATH "${OPENPOSE_MODELS_PATH}/getModels.bat")
	execute_process(COMMAND ${OPENPOSE_MODELS_DOWNLOADER_PATH} WORKING_DIRECTORY ${OPENPOSE_MODELS_PATH})
endif (NOT EXISTS ${OPENPOSE_DIR_PATH})
if (NOT EXISTS ${OPENPOSE_DIR_PATH})
	file(REMOVE ${OPENPOSE_ZIP_PATH})
	message(FATAL_ERROR "error : unzip was failed")
endif (NOT EXISTS ${OPENPOSE_DIR_PATH})

# download sqlitecpp
set(SQLITECPP_URL "https://github.com/SRombauts/SQLiteCpp/archive/be1a8eeace02ce98dfa3da688d1011c5bb895985.zip")
get_filename_component(SQLITECPP_ZIP_PATH "${SQLITECPP_URL}" NAME)
set(SQLITECPP_ZIP_PATH "${LIBRARIES_PATH}/SQLiteCpp-${SQLITECPP_ZIP_PATH}")
string(REGEX REPLACE "\\.[^.]*$" "" SQLITECPP_DIR_PATH ${SQLITECPP_ZIP_PATH})
if (NOT EXISTS ${SQLITECPP_DIR_PATH})
	if (NOT EXISTS ${SQLITECPP_ZIP_PATH})
		message(STATUS "download : ${SQLITECPP_URL}")
		message(STATUS "it takes long time...")
		file(TO_NATIVE_PATH ${SQLITECPP_ZIP_PATH} SQLITECPP_ZIP_PATH_BACKSLASH)
		execute_process(COMMAND bitsadmin /transfer openposeDownloadJob /dynamic /download /priority FOREGROUND ${SQLITECPP_URL} ${SQLITECPP_ZIP_PATH_BACKSLASH} WORKING_DIRECTORY ${LIBRARIES_PATH})
	endif (NOT EXISTS ${SQLITECPP_ZIP_PATH})
	if (NOT EXISTS ${SQLITECPP_ZIP_PATH})
		message(FATAL_ERROR "error : download was failed")
	endif (NOT EXISTS ${SQLITECPP_ZIP_PATH})
	message(STATUS "unzip : ${SQLITECPP_ZIP_PATH}")
	execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${SQLITECPP_ZIP_PATH} WORKING_DIRECTORY ${LIBRARIES_PATH})
endif (NOT EXISTS ${SQLITECPP_DIR_PATH})
if (NOT EXISTS ${SQLITECPP_DIR_PATH})
	file(REMOVE ${SQLITECPP_ZIP_PATH})
	message(FATAL_ERROR "error : unzip was failed")
endif (NOT EXISTS ${SQLITECPP_DIR_PATH})

# create project
project(openpose_ext)

# configuration
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_CONFIGURATION_TYPES "Release")

# copy necessary files
message(STATUS "copy : dll files")
file(GLOB DLL_FILES "${OPENPOSE_DIR_PATH}/openpose/bin/*")
file(COPY ${DLL_FILES} DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "copy : model files")
file(COPY "${OPENPOSE_DIR_PATH}/openpose/models" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "copy : media files")
file(COPY "${CMAKE_SOURCE_DIR}/media" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# add SQLiteCPP
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
add_subdirectory(${SQLITECPP_DIR_PATH})

# add source files
message(STATUS "search : all .cpp .lib files")
file(GLOB_RECURSE ALL_CPP_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB OPENPOSE_LIB_FILES "${OPENPOSE_DIR_PATH}/openpose/lib/*.lib")

# add main program
add_executable(openpose_ext "${CMAKE_SOURCE_DIR}/main.cpp" "${ALL_CPP_FILES}")
target_include_directories(openpose_ext PRIVATE
	"${CMAKE_SOURCE_DIR}/include"
	"${OPENPOSE_DIR_PATH}/openpose/include"
)
target_link_libraries(openpose_ext ${OPENPOSE_LIB_FILES})
target_link_libraries(openpose_ext SQLiteCpp)
set_target_properties(openpose_ext PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:openpose_ext>")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT openpose_ext)

# add example programs
file(GLOB_RECURSE ALL_EXAMPLE_FILES "${CMAKE_SOURCE_DIR}/examples/*.cpp")
foreach(EXAMPLE_FILE IN ITEMS ${ALL_EXAMPLE_FILES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_FILE} NAME_WE)
	add_executable("${EXAMPLE_NAME}" "${EXAMPLE_FILE}" "${ALL_CPP_FILES}")
	target_include_directories("${EXAMPLE_NAME}" PRIVATE
		"${CMAKE_SOURCE_DIR}/include"
		"${OPENPOSE_DIR_PATH}/openpose/include"
	)
	target_link_libraries("${EXAMPLE_NAME}" ${OPENPOSE_LIB_FILES})
	target_link_libraries("${EXAMPLE_NAME}" SQLiteCpp)
	set_target_properties("${EXAMPLE_NAME}" PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXAMPLE_NAME}>")
endforeach()