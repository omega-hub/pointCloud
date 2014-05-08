#include "BinaryPointsReader.h"

#include <osg/Geode>
#include <osg/Point>

#include <limits>

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
osgDB::ReaderWriter::ReadResult BinaryPointsReader::readNode(const std::string& filename, const osgDB::ReaderWriter::Options* o) const
{
    String path;
    if(DataManager::findFile(filename, path))
    {
        osg::Vec3Array* verticesP = new osg::Vec3Array();
        osg::Vec4Array* verticesC = new osg::Vec4Array();

        int numPoints = 0;
        float maxf = numeric_limits<float>::max();
        float minf = -numeric_limits<float>::max();
        Vector4f rgbamin = Vector4f(maxf, maxf, maxf, maxf);
        Vector4f rgbamax = Vector4f(minf, minf, minf, minf);

        // Check the options to see if we should read a subsection of the file
        // and / or use decimation.
        int readStartP = 0;
        int readLengthP = 0;
        int decimation = 0;
        int batchSize = 1000;

        if(o->getOptionString().size() > 0)
        {
            String format;

            libconfig::ArgumentHelper ah;
            ah.newString("format", "only suported format is xyzrgba", format);
            ah.newNamedInt('s', "start", "start", "start record %", readStartP);
            ah.newNamedInt('l', "length", "length", "number of records to read %", readLengthP);
            ah.newNamedInt('d', "decimation", "decimation", "read decimation", decimation);
            ah.newNamedInt('b', "batch-size", "batch size", "batch size", batchSize);
            ah.process(o->getOptionString().c_str());
        }

        readXYZ(path,
            readStartP, readLengthP, decimation,
            verticesP, verticesC,
            &numPoints,
            &rgbamin,
            &rgbamax);

        // create geometry and geodes to hold the data
        osg::Geode* geode = new osg::Geode();
        geode->setCullingActive(true);

        int numBatches = verticesP->size() / batchSize;
        ofmsg("%1%: creating %2% batches of %3% points", %filename %numBatches %batchSize);
        int batchStart = 0;
        for(int batchId = 0; batchId < numBatches; batchId++)
        {

            int prims = batchSize;
            if(batchStart + prims > verticesP->size())
            {
                prims = verticesP->size() - batchStart;
            }
            osg::Geometry* nodeGeom = new osg::Geometry();
            osg::StateSet *state = nodeGeom->getOrCreateStateSet();
            nodeGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, batchStart, prims));
            osg::VertexBufferObject* vboP = nodeGeom->getOrCreateVertexBufferObject();
            vboP->setUsage(GL_STREAM_DRAW);

            nodeGeom->setUseDisplayList(false);
            nodeGeom->setUseVertexBufferObjects(true);
            nodeGeom->setVertexArray(verticesP);
            nodeGeom->setColorArray(verticesC);
            nodeGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

            geode->addDrawable(nodeGeom);
            batchStart += prims;

        }

        geode->dirtyBound();
        //grp->addChild(geode);

        //omsg(model->info->loaderOutput);

        // Save loade results in the model info
        string output =
            ostr("{ 'numPoints': %d, "
            "'minR': %d, 'maxR': %d, "
            "'minG': %d, 'maxG': %d, "
            "'minB': %d, 'maxB': %d, "
            "'minA': %d, 'maxA': %d }",
            %numPoints
            %rgbamin[0] % rgbamax[0]
            % rgbamin[1] % rgbamax[1]
            % rgbamin[2] % rgbamax[2]
            % rgbamin[3] % rgbamax[3]
            );

        geode->setUserValue("loaderOutput", output);

        return ReadResult(geode);
    }
    return ReadResult();
}

///////////////////////////////////////////////////////////////////////////////
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
    size_t recordSize = sizeof(double)* numFields;

    FILE* fin = fopen(filename.c_str(), "rb");

    // How many records are in the file?
    fseek(fin, 0, SEEK_END);
    long endpos = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    int numRecords = endpos / recordSize;
    int readStart = numRecords * readStartP / 100;
    int readLength = numRecords * readLengthP / 100;

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
    double* buffer = (double*)malloc(recordSize * readLength / decimation);
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

/*
///////////////////////////////////////////////////////////////////////////////
bool BinaryPointsLoader::load(ModelAsset* model)
{
    osg::ref_ptr<osg::Group> group = new osg::Group();

	bool result = loadFile(model, group);

    if(result)
    {
		//osg::Geode* points;
	    //points = group->getChild(0)->asGeode();
	    
		model->nodes.push_back(group->asGroup());

	    //group->removeChild(0, 1);
    }
    return result;
}
*/