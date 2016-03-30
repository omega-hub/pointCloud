module_version(1.1)
request_dependency(cyclops)
include_directories(${OSG_INCLUDES})

add_library(pointCloud MODULE 
	pointCloud.cpp 
	TextPointsLoader.cpp 
	TextPointsLoader.h
	BinaryPointsLoader.cpp 
	BinaryPointsLoader.h
	BinaryPointsReader.cpp 
	BinaryPointsReader.h
    SphereArrayFilter.h
    SphereArrayFilter.cpp)
	
target_link_libraries(pointCloud omega cyclops)

declare_native_module(pointCloud)
