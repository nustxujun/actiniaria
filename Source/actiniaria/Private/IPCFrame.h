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
	void createModel(const std::string& name, const std::string& meshname, const Matrix& transform, const std::string& material);
	void createMaterial(UMaterialInterface* material);


	RenderCommand rendercmd;
};