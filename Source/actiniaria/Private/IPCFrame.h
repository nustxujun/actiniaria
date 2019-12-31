#include "Core.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

class IPCFrame
{
public:
	IPCFrame();
	~IPCFrame();
	void iterateObjects();
	void iterateLights();
	void init()
	{
		iterateObjects();
		iterateLights();
	}
};