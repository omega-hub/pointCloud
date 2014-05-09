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

