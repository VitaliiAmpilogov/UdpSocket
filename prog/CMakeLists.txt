cmake_minimum_required( VERSION 3.10.3 )

SET( SOURCES UdpSocket.cpp )
PROJECT( UdpSocket )
ADD_LIBRARY( ${PROJECT_NAME} STATIC ${SOURCES} )
