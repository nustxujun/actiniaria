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

	void init();
private:
	void iterateObjects();
	void iterateLights();
	void iterateCapture();

	void createMesh(const std::string& name, FStaticMeshRenderData & renderdata);
	void createModel(const std::string& name, const std::string& meshname, const Matrix& transform, const Matrix& normaltransform, const std::vector< std::string>& materials);
	void createStaticMesh(AStaticMeshActor* actor);
	void createMaterial(UMaterialInterface* material,  std::set<std::string>& textureMap);
	void createSkySphere(const std::string& name, const std::string& meshname, const std::string& mat, const Matrix& tran);
public:
	RenderCommand rendercmd;
	std::set<FString> meshs;
	std::set<FString> materials;
	std::set<std::string> textures;
};