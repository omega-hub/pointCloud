#include <omega.h>
#include <cyclops/cyclops.h>
#include <omegaToolkit.h>

#include "TextPointsLoader.h"
#include "BinaryPointsLoader.h"

using namespace omega;
using namespace cyclops;

///////////////////////////////////////////////////////////////////////////////
// Python wrapper code.
#ifdef OMEGA_USE_PYTHON
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_MODULE(pointCloud)
{
	PYAPI_REF_CLASS_WITH_CTOR(TextPointsLoader, ModelLoader);
	PYAPI_REF_CLASS_WITH_CTOR(BinaryPointsLoader, ModelLoader);
}
#endif
