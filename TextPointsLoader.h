#ifndef _TEXT_POINTS_LOADER_H_
#define _TEXT_POINTS_LOADER_H_

#include <omega.h>
#include <cyclops/cyclops.h>

// OSG
#include <osg/Group>
#include <osg/Vec3>
#include <osg/Uniform>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

using namespace omega;

class TextPointsLoader : public cyclops::ModelLoader
{
public:
	virtual bool load(cyclops::ModelAsset* model);
	virtual bool supportsExtension(const String& ext); 

    TextPointsLoader();
    virtual ~TextPointsLoader();
    void initialize();

private:
    bool loadFile(const String& file, const String& options, osg::Group * grp);
    void readXYZ(const String& filename, const String& options, osg::Vec3Array* points, osg::Vec4Array* colors);
};
#endif
