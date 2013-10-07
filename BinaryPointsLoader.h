#ifndef _BINARY_POINTS_LOADER_H_
#define _BINARY_POINTS_LOADER_H_

#include <omega.h>
#include <cyclops/cyclops.h>

// OSG
#include <osg/Group>
#include <osg/Vec3>
#include <osg/Uniform>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

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
    bool loadFile(cyclops::ModelAsset* model, osg::Group * grp);
    void readXYZ(const String& filename, 
		const String& options, osg::Vec3Array* points, osg::Vec4Array* colors,
		int* numPoints,
		Vector4f* rgbamin,
		Vector4f* rgbamax);

private:
	String format;
	int readStartP;
	int readLengthP;
	int decimation;
};
#endif
