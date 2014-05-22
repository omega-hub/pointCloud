#include "BinaryPointsReader.h"

#include <osg/Geode>
#include <osg/Point>
#include <osgDB/FileNameUtils>

#include <limits>

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
osgDB::ReaderWriter::ReadResult BinaryPointsReader::readNode(const std::string& filename, const osgDB::ReaderWriter::Options* o) const
{
    std::string ext(osgDB::getLowerCaseFileExtension(filename));
    if (!acceptsExtension(ext)) return ReadResult::FILE_NOT_HANDLED;

    String actualFilename = filename;

    bool useSinglePrecision = false;

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
        ah.newFlag('F', "float", "Use single precision floating point", useSinglePrecision);
        ah.process(o->getOptionString().c_str());
    }

    Vector<String> elems = StringUtils::split(filename, ".");
    if(elems.size() == 3)
    {
        // The filename format is [filepath].[options].xyzb
        // where options are startP-lengthP-decimation. This filename format is
        // used for paged LOD loading.
        Vector<String> options = StringUtils::split(elems[1], "-");

        if(options.size() != 3)
        {
            ofwarn("BinaryPointsReader::readNode: wrong filename format %1%", %filename);
            return ReadResult();
        }

        readStartP = boost::lexical_cast<int>(options[0]);
        readLengthP = boost::lexical_cast<int>(options[1]);
        decimation = boost::lexical_cast<int>(options[2]);

        actualFilename = elems[0] + "." + elems[2];

        ofmsg("Reading file %1% start=%2% length=%3% dec=%4%", 
            %actualFilename %readStartP %readLengthP %decimation);
    }
    else if(elems.size() != 2)
    {
        ofwarn("BinaryPointsReader::readNode: wrong filename format %1%", %filename);
        return ReadResult();
    }

    String path;
    if(DataManager::findFile(actualFilename, path))
    {
        osg::Vec3Array* verticesP = new osg::Vec3Array();
        osg::Vec4Array* verticesC = new osg::Vec4Array();

        int numPoints = 0;
        float maxf = numeric_limits<float>::max();
        float minf = -numeric_limits<float>::max();
        Vector4f rgbamin = Vector4f(maxf, maxf, maxf, maxf);
        Vector4f rgbamax = Vector4f(minf, minf, minf, minf);

        if(useSinglePrecision)
        {
            readXYZ<float>(path,
                readStartP, readLengthP, decimation,
                verticesP, verticesC,
                &numPoints,
                &rgbamin,
                &rgbamax);
        }
        else
        {
            readXYZ<double>(path,
                readStartP, readLengthP, decimation,
                verticesP, verticesC,
                &numPoints,
                &rgbamin,
                &rgbamax);
        }
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

