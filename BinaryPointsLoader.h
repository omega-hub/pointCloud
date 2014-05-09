#ifndef _BINARY_POINTS_LOADER_H_
#define _BINARY_POINTS_LOADER_H_

//#include <omega.h>
#include <cyclops/cyclops.h>

// OSG
//#include <osg/Group>
//#include <osg/Vec3>
//#include <osg/Uniform>
//#include <osgDB/ReadFile>
//#include <osgDB/FileUtils>

#include "BinaryPointsReader.h"


using namespace omega;

class BinaryPointsLoader : public cyclops::ModelLoader
{
public:
	virtual bool load(cyclops::ModelAsset* model);
	virtual bool supportsExtension(const String& ext); 

    BinaryPointsLoader();
    virtual ~BinaryPointsLoader();
    void initialize();

private:
	String format;
};
#endif
