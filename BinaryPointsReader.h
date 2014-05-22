#ifndef _BINARY_POINTS_READER_H_
#define _BINARY_POINTS_READER_H_

#include <omega.h>

// OSG
#include <osg/Group>
#include <osg/Vec3>
#include <osg/Uniform>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgDB/ReaderWriter>
#include <osg/ValueObject>

using namespace omega;

class BinaryPointsReader: public osgDB::ReaderWriter
{
public:
    BinaryPointsReader()
    {
        supportsExtension("xyzb", "XYZ binary");
    }

    const char* className() const { return "Binary points reader"; }

    virtual ReadResult readNode(const std::string& filename, const Options*) const;

private:
    void readXYZ(
        const String& filename,
        int readStartP, int readLengthP, int decimation,
        osg::Vec3Array* points, osg::Vec4Array* colors,
        int* numPoints,
        Vector3f* pointmin,
        Vector3f* pointmax,
        Vector4f* rgbamin,
        Vector4f* rgbamax) const;

    ReadResult readBoundsFile(const String& datafile) const;
};
#endif
