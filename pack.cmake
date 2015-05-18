if(WIN32)
    file(INSTALL DESTINATION ${PACKAGE_DIR}/bin
        TYPE FILE
        FILES
            ${BIN_DIR}/pointCloud.pyd
        )
elseif(APPLE)
    file(INSTALL DESTINATION ${PACKAGE_DIR}/bin
        TYPE FILE
        FILES
            ${BIN_DIR}/pointCloud.so
        )
endif()

file(INSTALL DESTINATION ${PACKAGE_DIR}/modules/pointCloud
    TYPE DIRECTORY
    FILES
        ${SOURCE_DIR}/modules/pointCloud/shaders
    )

file(INSTALL DESTINATION ${PACKAGE_DIR}/examples/pointCloud
    TYPE FILE
    FILES
        ${SOURCE_DIR}/modules/pointCloud/examples/pointCloudIntersector.py 
        ${SOURCE_DIR}/modules/pointCloud/examples/pointCloudSampleGenerator.py 
        ${SOURCE_DIR}/modules/pointCloud/examples/pointCloudView.py 
    )
    