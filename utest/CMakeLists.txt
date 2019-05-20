cmake_minimum_required( VERSION 3.10.3 )

SET( SOURCES test.cpp )
INCLUDE_DIRECTORIES( ../prog )
PROJECT( unit_test )
ADD_EXECUTABLE( ${PROJECT_NAME} ${SOURCES} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} PUBLIC gtest_main UdpSocket )
