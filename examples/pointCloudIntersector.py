# (C) 2014 - Arthur Nishimoto, Electronic Visualization Laboratory, University of Illinois at Chicago
#
# Based on pointCloud example by Alessandro Febretti, evl, uic
#
# Right-click on the point cloud to select the nearest point at that location.
# Uses the omegaOsg PointIntersector

from cyclops import *
from pointCloud import *

SceneNodeHitPointsFlag = 1 << 16;

scene = getSceneManager()
scene.addLoader(BinaryPointsLoader())

pointProgram = ProgramAsset()
pointProgram.name = "points"
pointProgram.vertexShaderName = "pointCloud/shaders/Sphere.vert"
pointProgram.fragmentShaderName = "pointCloud/shaders/Sphere.frag"
pointProgram.geometryShaderName = "pointCloud/shaders/Sphere.geom"
pointProgram.geometryOutVertices = 4
pointProgram.geometryInput = PrimitiveType.Points
pointProgram.geometryOutput = PrimitiveType.TriangleStrip
scene.addProgram(pointProgram)

pointScale = Uniform.create('pointScale', UniformType.Float, 1)
pointScale.setFloat(0.1)

pointCloudModel = ModelInfo()
pointCloudModel.name = 'pointCloud'
pointCloudModel.path = 'data.xyzb'
#pointCloudModel.options = "10000 100:1000000:5 20:100:4 6:20:2 0:5:1"
pointCloudModel.options = "10000 100:1000000:20 20:100:10 6:20:5 0:5:5"
#pointCloudModel.options = "10000 0:1000000:1"
scene.loadModel(pointCloudModel)

pointCloud = StaticObject.create(pointCloudModel.name)

# Set the flag to enable PointIntersector instead of standard face/line interesction in hitNode()
pointCloud.setFlag( SceneNodeHitPointsFlag );

# attach shader uniforms
mat = pointCloud.getMaterial()
mat.setProgram(pointProgram.name)
mat.attachUniform(pointScale)

rayMarker = SphereShape.create(0.5,1)
rayMarker.setScale(Vector3(0.6, 0.6, 0.6))       
rayMarker.setEffect('colored -e red')

def onEvent():
	e = getEvent()
	
	# On pointer and right click
	if(e.getServiceType() == ServiceType.Pointer and e.isButtonDown(EventFlags.Right) ):
		r = getRayFromEvent(e)
		
		# hitNode on pointCloud node with ray start r[1] and end r[2] points
		hitData = hitNode(pointCloud, r[1], r[2])
		
		if( hitData[0] ):
			rayMarker.setPosition( hitData[1] )
			print hitData[1] # print intersection position

setEventFunction(onEvent)