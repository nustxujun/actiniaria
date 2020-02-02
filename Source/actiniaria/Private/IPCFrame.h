#include "Core.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/RenderCommand.h"

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

		rendercmd.done();
	}
private:
	void createMesh(const std::string& name, FStaticMeshRenderData & renderdata);
	void createModel(const std::string& name, const std::string& meshname, const Matrix& transform, const Matrix& normaltransform, const std::vector< std::string>& materials);
	void createMaterial(UMaterialInterface* material);

public:
	RenderCommand rendercmd;
};