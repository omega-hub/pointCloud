request_dependency(cyclops)
include_directories(${OSG_INCLUDES})

add_library(pointCloud MODULE 
	pointCloud.cpp 
	TextPointsLoader.cpp 
	TextPointsLoader.h
	BinaryPointsLoader.cpp 
	BinaryPointsLoader.h)
	
target_link_libraries(pointCloud omega cyclops)

set_target_properties(pointCloud PROPERTIES PREFIX "")
if(WIN32)
	set_target_properties(pointCloud PROPERTIES FOLDER modules SUFFIX ".pyd")
endif()
