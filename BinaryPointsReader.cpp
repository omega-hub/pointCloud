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
    bool sizeOnly = false;

    if(o->getOptionString().size() > 0)
    {
        String format;

        libconfig::ArgumentHelper ah;
        ah.newString("format", "only suported format is xyzrgba", format);
        ah.newNamedInt('s', "start", "start", "start batch id", readStartP);
        ah.newNamedInt('l', "length", "length", "number of batches to read", readLengthP);
        ah.newNamedInt('d', "decimation", "decimation", "read decimation", decimation);
        ah.newNamedInt('b', "batch-size", "batch size", "batch size", batchSize);
        ah.newFlag('z', "size", "computes size only (or read from cached", sizeOnly);
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

        //ofmsg("Reading file %1% start=%2% length=%3% dec=%4%", 
        //    %actualFilename %readStartP %readLengthP %decimation);
    }
    else if(elems.size() != 2)
    {
        ofwarn("BinaryPointsReader::readNode: wrong filename format %1%", %filename);
        return ReadResult();
    }

    String path;

    if(sizeOnly)
    {
        // If we are only getting the point and data bounds for this file,
        // see if we have a result already computed and cached.
        ReadResult rr = readBoundsFile(filename);
        if(rr.status() != ReadResult::FILE_NOT_HANDLED) return rr;
    }

    if(DataManager::findFile(actualFilename, path))
    {
        osg::Vec3Array* verticesP = new osg::Vec3Array();
        osg::Vec4Array* verticesC = new osg::Vec4Array();

        size_t numPoints = 0;
        float maxf = numeric_limits<float>::max();
        float minf = -numeric_limits<float>::max();
        Vector4f rgbamin = Vector4f(maxf, maxf, maxf, maxf);
        Vector4f rgbamax = Vector4f(minf, minf, minf, minf);
        Vector3f pointmin = Vector3f(maxf, maxf, maxf);
        Vector3f pointmax = Vector3f(minf, minf, minf);

        if(useSinglePrecision)
        {
            readXYZ<float>(path,
            readStartP, readLengthP, decimation, 
            verticesP, verticesC,
            &numPoints,
            &pointmin,
            &pointmax,
            &rgbamin,
            &rgbamax);
        }
        else
        {
            readXYZ<double>(path,
                readStartP, readLengthP, decimation,
                verticesP, verticesC,
                &numPoints,
                &pointmin,
                &pointmax,
                &rgbamin,
                &rgbamax);
        }
        if(sizeOnly)
        {
            //omsg("Computing data bounds");

            // Generate the bounds file
            String boundsBasename;
            String boundsExtension;
            String boundsPath;
            StringUtils::splitFullFilename(filename, boundsBasename, boundsExtension, boundsPath);

            // make sure path exists
            String p = boundsPath + "bounds";
            //ofmsg("Creating path %1%", %p);
            DataManager::createPath(p);

            // Open bounds file and write bounds data.
            String boundsFileName = boundsPath + "bounds/" + boundsBasename + ".bounds";
            FILE* bf = fopen(boundsFileName.c_str(), "w");
            fprintf(bf, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
                pointmin[0], pointmax[0],
                pointmin[1], pointmax[1],
                pointmin[2], pointmax[2],
                rgbamin[0], rgbamax[0],
                rgbamin[1], rgbamax[1],
                rgbamin[2], rgbamax[2],
                rgbamin[3], rgbamax[3]);
            fclose(bf);

            // Read the bounds file and return the result.
            ReadResult rr = readBoundsFile(filename);
            if(rr.status() != ReadResult::FILE_NOT_HANDLED) return rr;
            ofwarn("Could not read bounds file for %1%", %filename);
            return rr;
        }

        // create geometry and geodes to hold the data
        osg::Geode* geode = new osg::Geode();
        geode->setCullingActive(true);

        // hack 
/*        batchSize = verticesP->size();

        int numBatches = 1; //verticesP->size() / batchSize;
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
*/
        // NOBATCH
        osg::Geometry* nodeGeom = new osg::Geometry();
        osg::StateSet *state = nodeGeom->getOrCreateStateSet();
        nodeGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, verticesP->size()));
        osg::VertexBufferObject* vboP = nodeGeom->getOrCreateVertexBufferObject();
        vboP->setUsage(GL_STREAM_DRAW);

        nodeGeom->setUseDisplayList(false);
        nodeGeom->setUseVertexBufferObjects(true);
        nodeGeom->setVertexArray(verticesP);
        nodeGeom->setColorArray(verticesC);
        nodeGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        // NOBATCH

        geode->addDrawable(nodeGeom);

        geode->dirtyBound();
        //grp->addChild(geode);

        //omsg(model->info->loaderOutput);

        return ReadResult(geode);
    }
    return ReadResult();
}

///////////////////////////////////////////////////////////////////////////////
osgDB::ReaderWriter::ReadResult BinaryPointsReader::readBoundsFile(const String& filename) const
{
    // Results will be in a file with same name as the one we want to open,
    // but with extension .bounds, in a directory called bounds
    // get base filename (without extension)
    String boundsBasename;
    String boundsExtension;
    String boundsPath;
    String path;
    StringUtils::splitFullFilename(filename, boundsBasename, boundsExtension, boundsPath);

    String boundsFileName = boundsPath + "bounds/" + boundsBasename + ".bounds";
    if(DataManager::findFile(boundsFileName, path))
    {
        String boundsText = DataManager::readTextFile(path);
        // Bounds text format:
        // xmin, xmax, ymin, ymax, zmin, zmax, rmin, rmax, gmin, gmax, bmin, bmax, amin, amax
        Vector<String> vals = StringUtils::split(boundsText, ",");
        float values[14];
        for(int i = 0; i < 14; i++)
        {
            StringUtils::trim(vals[i]);
            values[i] = boost::lexical_cast<float>(vals[i]);
        }
        Ref<osg::Node> n = new osg::Node();
        n->setUserValue("xmin", values[0]);
        n->setUserValue("xmax", values[1]);
        n->setUserValue("ymin", values[2]);
        n->setUserValue("ymax", values[3]);
        n->setUserValue("zmin", values[4]);
        n->setUserValue("zmax", values[5]);
        n->setUserValue("rmin", values[6]);
        n->setUserValue("rmax", values[7]);
        n->setUserValue("gmin", values[8]);
        n->setUserValue("gmax", values[9]);
        n->setUserValue("bmin", values[10]);
        n->setUserValue("bmax", values[11]);
        n->setUserValue("amin", values[12]);
        n->setUserValue("amax", values[13]);

        //ofmsg("setmin %1% %2% %3% setmax %4% %5% %6%", 
        //    %values[0] %values[2] %values[4]
        //    %values[1] %values[3] %values[5]);

        return ReadResult(n);
    }
    else
    {
        ofmsg("BinaryPointsReader::readBoundsFile bounds file not found %1% ", %boundsFileName);
    }
    return ReadResult();
}
