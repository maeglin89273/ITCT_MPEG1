cmake_minimum_required(VERSION 3.7)
project(ITCT_MPEG1)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp BitBuffer.cpp BitBuffer.h TypeDefinition.h MpegPlayer.cpp MpegPlayer.h MpegDecoder.cpp MpegDecoder.h Picture.cpp Picture.h Block.cpp Block.h Sequence.cpp Sequence.h HuffmanTable.cpp HuffmanTable.h FIDCT.cpp FIDCT.h Renderer.h)
find_package( OpenCV REQUIRED )
include_directories( ${OPENCV_INCLUDE_DIR} )
link_directories(${OPENCV_LIB_DIR})
add_executable(ITCT_MPEG1 ${SOURCE_FILES})
target_link_libraries(ITCT_MPEG1 ${OpenCV_LIBS})