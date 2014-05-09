#include "BinaryPointsLoader.h"

#include <osg/Geode>
#include <osg/Point>

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
    // Parse options (format: 'pointsPerBatch dist:dec+')
    // where pointsPerBatch is the number of points for each LOD group 
    // at max LOD, and each dist:dec pair is a LOD level with distance from 
    // eye and decimation level.
    Vector<String> args = StringUtils::split(model->info->options, " ");
    int pointsPerBatch = boost::lexical_cast<int>(args[0]);
    Vector<std::pair<int, int> > lodlevels;
    for(int i = 1; i < args.size(); i++)
    {
        Vector<String> lodargs = StringUtils::split(args[i], ":");
        lodlevels.push_back(std::pair<int, int>(
            boost::lexical_cast<int>(lodargs[0]),
            boost::lexical_cast<int>(lodargs[1])
            ));
    }


    osg::ref_ptr<osg::Group> group = new osg::Group();

	osgDB::Options* options = new osgDB::Options; 
	options->setOptionString(model->info->options);
	osg::Node* node = osgDB::readNodeFile(model->info->path, options);

    if(node)
    {
    	node->getUserValue("loaderOutput", model->info->loaderOutput);
		model->nodes.push_back(node);
	    return true;
    }
    return false;
}

