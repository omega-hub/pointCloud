#include "BinaryPointsLoader.h"

#include <osg/Geode>
#include <osg/Point>
#include <osg/PagedLOD>

#include <limits>

using namespace omega;
using namespace cyclops;

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
    long endpos = ftell(fin);
    long numRecords = endpos / recordSize;
    fclose(fin);

    // Parse options (format: 'pointsPerBatch dist:dec+')
    // where pointsPerBatch is the number of points for each LOD group 
    // at max LOD, and each distmin:distmax:dec pair is a LOD level with distance from 
    // eye and decimation level.
    Vector<String> args = StringUtils::split(model->info->options, " ");
    long pointsPerBatch = boost::lexical_cast<long>(args[0]);

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

    Vector<LODLevel> lodlevels;
    for(int i = 1; i < args.size(); i++)
    {
        Vector<String> lodargs = StringUtils::split(args[i], ":");
        lodlevels.push_back(LODLevel(
            boost::lexical_cast<int>(lodargs[0]),
            boost::lexical_cast<int>(lodargs[1]),
            boost::lexical_cast<int>(lodargs[2])
            ));
    }

    // Convert points per batch to batch length as file size percentage.
    int lengthP = pointsPerBatch * 100 / numRecords;

    // Create root group for this point cloud
    Ref<osg::Group> group = new osg::Group();

    // get base filename (without extension)
    String basename;
    String extension;
    StringUtils::splitBaseFilename(model->info->path, basename, extension);

    // Iterate for each batch
    for(int startP = 0; startP <= 100; startP += lengthP)
    {
        int childid = 0;
        osg::PagedLOD* plod = new osg::PagedLOD();
        plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        plod->setNumChildrenThatCannotBeExpired(1);
        group->addChild(plod);

        osgDB::Options* options = new osgDB::Options; 
        options->setOptionString(ostr("-b %1%", %pointsPerBatch));

        plod->setDatabaseOptions(options);

        // Create LOD groups for each batch
        foreach(LODLevel ll, lodlevels)
        {
            String filename = ostr("%1%.%2%-%3%-%4%.xyzb",
                %basename
                %startP %lengthP %ll.dec);
            plod->setFileName(childid, filename);
            plod->setRange(childid, ll.distmin, ll.distmax);

            // Minimum expiration time and frames.
            plod->setMinimumExpiryFrames(childid, 60);
            plod->setMinimumExpiryTime(childid, 5);
            childid++;
        }
    }

    model->nodes.push_back(group);

	//osgDB::Options* options = new osgDB::Options; 
	//options->setOptionString(model->info->options);
	//osg::Node* node = osgDB::readNodeFile(model->info->path, options);

  //  if(node)
  //  {
  //  	node->getUserValue("loaderOutput", model->info->loaderOutput);
		//model->nodes.push_back(node);
	 //   return true;
  //  }
    return true;
}

