#include "BinaryPointsLoader.h"

#include <osg/Geode>
#include <osg/Point>
#include <osg/PagedLOD>

#include <limits>

using namespace omega;
using namespace cyclops;

///////////////////////////////////////////////////////////////////////////////
// Used by load method
struct LODLevel
{
    LODLevel(int dmin, int dmax, int dc) :
        distmin(dmin),
        distmax(dmax),
        dec(dc) {}
    int distmin;
    int distmax;
    int dec;
};

///////////////////////////////////////////////////////////////////////////////
BinaryPointsLoader::BinaryPointsLoader(): ModelLoader("points-binary")
{
    osgDB::Registry* reg = osgDB::Registry::instance();
    reg->addReaderWriter(new BinaryPointsReader());
}

///////////////////////////////////////////////////////////////////////////////
BinaryPointsLoader::~BinaryPointsLoader()
{
}

///////////////////////////////////////////////////////////////////////////////
bool BinaryPointsLoader::supportsExtension(const String& ext) 
{ 
	if(StringUtils::endsWith(ext, "xyzb")) return true;
	return false; 
}

///////////////////////////////////////////////////////////////////////////////
bool BinaryPointsLoader::load(ModelAsset* model)
{
    // Compute max records (points) from source file
    String path;
    if(!DataManager::findFile(model->info->path, path))
    {
        ofwarn("BinaryPointsLoader::load: could not find %1%", %model->info->path);
        return false;
    }

    // Default record size = 7 doubles (X,Y,Z,R,G,B,A)
    int numFields = 7;
    size_t recordSize = sizeof(double)* numFields;
    FILE* fin = fopen(path.c_str(), "rb");
    // How many records are in the file?
    fseek(fin, 0, SEEK_END);
    size_t endpos = ftell(fin);
    size_t numRecords = endpos / recordSize;
    fclose(fin);

    // Parse options (format: 'pointsPerBatch dist:dec+')
    // where pointsPerBatch is the number of points for each LOD group 
    // at max LOD, and each distmin:distmax:dec pair is a LOD level with distance from 
    // eye and decimation level.
    Vector<String> args = StringUtils::split(model->info->options, " ");
    size_t pointsPerBatch = boost::lexical_cast<size_t>(args[0]);

    // Convert points per batch to batch length as file size percentage.
    size_t lengthP = pointsPerBatch * BINARY_POINTS_MAX_BATCHES / numRecords;

    ofmsg("Total Points: %1%   Points per batch: %2%   Batch length(%%): %3%", 
    	%numRecords
    	%pointsPerBatch
    	%lengthP);

    if(lengthP < 1)
    {
    	lengthP = 1;
        pointsPerBatch = numRecords / BINARY_POINTS_MAX_BATCHES;
        ofwarn("Can't have more than %1% batches. Adjusting batch size to %2%", %BINARY_POINTS_MAX_BATCHES %pointsPerBatch);
    }

    int mindec = 1000000;
    Vector<LODLevel> lodlevels;
    for(int i = 1; i < args.size(); i++)
    {
        Vector<String> lodargs = StringUtils::split(args[i], ":");
        LODLevel ll(
            boost::lexical_cast<int>(lodargs[0]),
            boost::lexical_cast<int>(lodargs[1]),
            boost::lexical_cast<int>(lodargs[2])
            );
		lodlevels.push_back(ll);
        if(ll.dec < mindec) mindec = ll.dec;
    }

    ofmsg("LOD levels: %1%", %lodlevels.size());

    // Create root group for this point cloud
    Ref<osg::Group> group = new osg::Group();

    // get base filename (without extension)
    String basename;
    String extension;
    StringUtils::splitBaseFilename(model->info->path, basename, extension);

    float maxf = numeric_limits<float>::max();
    float minf = -numeric_limits<float>::max();
    Vector4f rgbamin = Vector4f(maxf, maxf, maxf, maxf);
    Vector4f rgbamax = Vector4f(minf, minf, minf, minf);

    // Iterate for each batch
    for(int startP = 0; startP <= BINARY_POINTS_MAX_BATCHES; startP += lengthP)
    {
        int childid = 0;
        osg::PagedLOD* plod = new osg::PagedLOD();
        plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        plod->setNumChildrenThatCannotBeExpired(2);
        group->addChild(plod);

        osgDB::Options* options = new osgDB::Options; 
        options->setOptionString(ostr("xyzrgba -b %1%", %(pointsPerBatch / mindec)));

        plod->setDatabaseOptions(options);
        //plod->setCenterMode(osg::LOD::USE_BOUNDING_SPHERE_CENTER);

        // Create LOD groups for each batch
		String filename;
        foreach(LODLevel ll, lodlevels)
        {
            filename = ostr("%1%.%2%-%3%-%4%.xyzb",
                %basename
                %startP %lengthP %ll.dec);

            plod->setFileName(childid, filename);
            plod->setRange(childid, ll.distmin, ll.distmax);

            // Minimum expiration time and frames.
            plod->setMinimumExpiryFrames(childid, 60);
            plod->setMinimumExpiryTime(childid, 5);

            if(childid == 0)
        	{
		        // Load or compute bounds
		        Ref<osgDB::Options> boundoptions = new osgDB::Options; 
		        boundoptions->setOptionString(ostr("xyzrgba -b %1% -z", %(pointsPerBatch / mindec)));
				Ref<osg::Node> n = osgDB::readNodeFile(filename, boundoptions);

				// The node only stores user data. read it back.
		        float brgbamin[4];
		        float brgbamax[4];
		        float bpointmin[3];
		        float bpointmax[3];
		        n->getUserValue("xmin", bpointmin[0]);
		        n->getUserValue("xmax", bpointmax[0]);
		        n->getUserValue("ymin", bpointmin[1]);
		        n->getUserValue("ymax", bpointmax[1]);
		        n->getUserValue("zmin", bpointmin[2]);
		        n->getUserValue("zmax", bpointmax[2]);
		        n->getUserValue("rmin", brgbamin[0]);
		        n->getUserValue("rmax", brgbamax[0]);
		        n->getUserValue("gmin", brgbamin[1]);
		        n->getUserValue("gmax", brgbamax[1]);
		        n->getUserValue("bmin", brgbamin[2]);
		        n->getUserValue("bmax", brgbamax[2]);
		        n->getUserValue("amin", brgbamin[3]);
		        n->getUserValue("amax", brgbamax[3]);

		        // Use this batch bounds to upate the point cloud bounds
		        for(int j = 0; j < 4; j++)
		        {
		            if(brgbamin[j] < (rgbamin)[j]) (rgbamin)[j] = brgbamin[j];
		            if(brgbamax[j] > (rgbamax)[j]) (rgbamax)[j] = brgbamax[j];
		        }

		        // Compute batch center
		        Vector3f bm(bpointmin[0], bpointmin[1], bpointmin[2]);
		        Vector3f bM(bpointmax[0], bpointmax[1], bpointmax[2]);
		        Vector3f bcenter = (bm + bM)/2;
		        //ofmsg("batch center: %1%", %bcenter);
				plod->setCenter(osg::Vec3d(bcenter[0], bcenter[1], bcenter[2]));
        	}
            childid++;
        }
    }

    // Save loaded results in the model info
    string output =
        ostr("{ "
        "'minR': %d, 'maxR': %d, "
        "'minG': %d, 'maxG': %d, "
        "'minB': %d, 'maxB': %d, "
        "'minA': %d, 'maxA': %d }",
        %rgbamin[0] %rgbamax[0]
        %rgbamin[1] %rgbamax[1]
        %rgbamin[2] %rgbamax[2]
        %rgbamin[3] %rgbamax[3]
        );

    model->info->loaderOutput = output;

    model->nodes.push_back(group);

  //  if(node)
  //  {
  //  	node->getUserValue("loaderOutput", model->info->loaderOutput);
		//model->nodes.push_back(node);
	 //   return true;
  //  }
    return true;
}

