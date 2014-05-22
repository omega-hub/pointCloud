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
    template<typename T>
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

///////////////////////////////////////////////////////////////////////////////
template<typename T>
void BinaryPointsReader::readXYZ(
    const String& filename,
    int readStartP, int readLengthP, int decimation,
    osg::Vec3Array* points, osg::Vec4Array* colors,
    int* numPoints,
    Vector4f* rgbamin,
    Vector4f* rgbamax) const
{
    osg::Vec3f point;
    osg::Vec4f color(1.0f, 1.0f, 1.0f, 1.0f);

    // Default record size = 7 doubles (X,Y,Z,R,G,B,A)
    int numFields = 7;
    size_t recordSize = sizeof(T)* numFields;

    FILE* fin = fopen(filename.c_str(), "rb");

    // How many records are in the file?
    fseek(fin, 0, SEEK_END);
    long endpos = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    long numRecords = endpos / recordSize;
    long readStart = numRecords * readStartP / 100;
    long readLength = numRecords * readLengthP / 100;

    if(decimation <= 0) decimation = 1;
    if(readStart != 0)
    {
        fseek(fin, readStart * recordSize, SEEK_SET);
    }

    // Adjust read length.
    if(readLength == 0 || readStart + readLength > numRecords)
    {
        readLength = numRecords - readStart;
    }

    ofmsg("BinaryPointsLoader: reading records %1% - %2% of %3% (decimation %4%) of %5%",
        %readStart % (readStart + readLength) % numRecords %decimation %filename);

    // Read in data
    T* buffer = (T*)malloc(recordSize * readLength / decimation);
    if(buffer == NULL)
    {
        oferror("BinaryPointsLoader::readXYZ: could not allocate %1% bytes",
            % (recordSize * readLength / decimation));
        return;
    }

    int ne = readLength / decimation;

    // Read data
    // If data is not decimated, read it in one go.
    if(decimation == 1)
    {
        size_t size = fread(buffer, recordSize, readLength, fin);
    }
    else
    {
        int j = 0;
        for(int i = 0; i < ne; i++)
        {
            // Read one record
            size_t size = fread(&buffer[j], recordSize, 1, fin);
            // Skip ahead decimation - 1 records.
            fseek(fin, recordSize * (decimation - 1), SEEK_CUR);
            j += numFields;
        }
    }

    points->reserve(ne);
    colors->reserve(ne);

    int j = 0;
    for(int i = 0; i < ne; i++)
    {
        point[0] = buffer[i * numFields];
        point[1] = buffer[i * numFields + 1];
        point[2] = buffer[i * numFields + 2];

        color[0] = buffer[i * numFields + 3];
        color[1] = buffer[i * numFields + 4];
        color[2] = buffer[i * numFields + 5];
        color[3] = buffer[i * numFields + 6];

        points->push_back(point);
        colors->push_back(color);

        // Update data bounds and number of points
        *numPoints = *numPoints + 1;
        for(int j = 0; j < 4; j++)
        {
            if(color[j] < (*rgbamin)[j]) (*rgbamin)[j] = color[j];
            if(color[j] > (*rgbamax)[j]) (*rgbamax)[j] = color[j];
        }
    }

    fclose(fin);
    free(buffer);
}
#endif
