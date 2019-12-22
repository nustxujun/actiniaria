#include "Core.h"
#include <sstream>

#include "Engine/Framework.h"
#include "Engine/Pipeline.h"
#include "Engine/RenderContext.h"
#include "Engine/ImguiOverlay.h"

#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"

#include "Model.h"
#include "Material.h"

#include "Runtime/Json/Public/Dom/JsonObject.h"
#include "Runtime/Json/Public/Serialization/JsonReader.h"

#include <iostream>

inline FString GetPluginPath()
{
	FString path = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("actiniaria"));
	if (FPaths::DirectoryExists(path))
		return path;
	return  FPaths::Combine(FPaths::EnginePluginsDir(), TEXT("actiniaria"));
}

class ActiniariaFrame :public RenderContext, public Framework
{
public:
	std::shared_ptr<DefaultPipeline> pipeline;
	Renderer::Buffer::Ptr vertices;
	Renderer::Texture::Ref tex;
	std::vector<Model::Ptr> models;

	ActiniariaFrame()
	{
		if (!::AllocConsole())
		{
			Common::checkResult(GetLastError(), " init actiniaria");
		}
		else
		{
			freopen("CONOUT$", "w", stdout);
			std::cout << "console started." << std::endl;
		}
		FString path = GetPluginPath() + "/Source/actiniaria/Private/engine/";

		Renderer::getSingleton()->addSearchPath(*path);
		pipeline = decltype(pipeline)(new DefaultPipeline());
		Framework::resize(960, 540);

	}

	~ActiniariaFrame()
	{
	}

	void iterateObjects()
	{
		
		{
			auto camIter = TObjectIterator<ACameraActor>();
			Common::Assert(!!camIter, "need camera");
			auto camact = *camIter;
			auto camcom = camact->GetCameraComponent();
			FMinimalViewInfo info;
			camcom->GetCameraView(0,info);
			auto maincamera = getMainCamera();
			const auto& vp = maincamera->getViewport();

			//info.FOV = vp.Width / vp.Height;

			std::cout << "FOV: " << info.FOV << std::endl;
			
			FMatrix proj;
			{
				float FarZ = 1000;
				float NearZ = 0.1f;
				float halfFov = info.FOV * 0.5f;

				proj = FPerspectiveMatrix(halfFov, vp.Width, vp.Height,NearZ,FarZ).GetTransposed();
			}

			//auto proj = FMatrix::Identity;

			auto rot = camact->GetTransform().GetRotation().Rotator();
			FMatrix ViewPlanesMatrix = FMatrix(
				FPlane(0, 0, 1, 0),
				FPlane(1, 0, 0, 0),
				FPlane(0, 1, 0, 0),
				FPlane(0, 0, 0, 1));
			auto rotmat = FInverseRotationMatrix(rot) * ViewPlanesMatrix;

			auto view = FTranslationMatrix(-camact->GetTransform().GetLocation()) * rotmat;
			view = view.GetTransposed();
			maincamera->setProjectionMatrix(*(Matrix*)&proj);
			maincamera->setViewMatrix(*(Matrix*)&view);
	

		}



		std::map<FString, StaticMesh::Ptr> meshs;
		std::map<FString, Material::Ptr> materials;
		for (TObjectIterator<AStaticMeshActor> iter; iter; ++iter)
		{
			auto actor = *iter;
			auto component = actor->GetStaticMeshComponent();
			auto mesh = component->GetStaticMesh();
			if (mesh == nullptr )
				continue;

			auto material = mesh->GetMaterial(0);
			if(material == nullptr)
				continue;
			Material::Ptr mat;
			{
				auto ret = materials.find(material->GetName());
				if (ret != materials.end())
					mat = ret->second;
				else
				{
					mat = initMaterial(material);
					materials[material->GetName()] = mat;
				}
			}

			StaticMesh::Ptr dstMesh;
			{
				auto ret = meshs.find(mesh->GetName());
				if (ret != meshs.end())
					dstMesh = ret->second;
				else
				{
					dstMesh = StaticMesh::Ptr( new StaticMesh(*mesh->RenderData));
					meshs[mesh->GetName()] = dstMesh;
				}
			}
			auto model = Model::Ptr(new Model());
			model->setMaterial(mat);
			model->setMesh(dstMesh);
			model->setTramsform(actor->GetTransform().ToMatrixWithScale());
			model->setVSCBuffer(mat->pipelineState->createConstantBuffer(Renderer::Shader::ST_VERTEX,"VSConstant"));
			model->setPSCBuffer(mat->pipelineState->createConstantBuffer(Renderer::Shader::ST_PIXEL, "PSConstant"));

			models.push_back(model);
		}
	}

	std::wstring mapMaterial(const FString& name)
	{
		if (name == TEXT("M_Chair"))
			return L"customization/chair_ps.hlsl";
		else if (name == TEXT("M_TableRound"))
			return L"customization/table_ps.hlsl";
		else
			return L"scene_ps.hlsl";
	}

	Material::Ptr initMaterial(UMaterialInterface* material)
	{
		auto renderer = Renderer::getSingleton();

		auto vs = renderer->compileShader(L"shaders/scene_vs.hlsl", L"vs", L"vs_5_0");
		auto ps = renderer->compileShader(L"shaders/" + mapMaterial(material->GetName()), L"ps", L"ps_5_0");
		std::vector<Renderer::Shader::Ptr> shaders = { vs, ps };
		ps->registerStaticSampler({
				D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				0,0,
				D3D12_COMPARISON_FUNC_NEVER,
				D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
				0,
				D3D12_FLOAT32_MAX,
				0,0,
				D3D12_SHADER_VISIBILITY_PIXEL
			});
		Renderer::RenderState rs = Renderer::RenderState::GeneralSolid;
		rs.setInputLayout({
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			});

		auto pso = renderer->createPipelineState(shaders, rs);

		Material::Ptr ret = Material::Ptr(new Material(material, pso));
		return ret;
	}

	void init()
	{



		iterateObjects();


	}

	void renderScreen()
	{
	}

	void updateImpl()
	{
		static auto lastTime = GetTickCount64();
		static auto framecount = 0;
		auto cur = GetTickCount64();
		auto delta = cur - lastTime;
		framecount++;
		if (delta > 0)
		{
			lastTime = cur;
			float time = (float)framecount * 1000.0f / (float)delta;
			framecount = 0;
			static float history = time;
			history = history * 0.99f + time * 0.01f;

			std::stringstream ss;
			ss.precision(4);
			ss << history << "(" << 1000.0f / history << "ms)";
			::SetWindowTextA(Renderer::getSingleton()->getWindow(), ss.str().c_str());
		}

		pipeline->update();
	}

	void renderScene(Camera::Ptr cam, UINT, UINT)
	{
		auto cmdlist = Renderer::getSingleton()->getCommandList();

		const auto& vp = cam->getViewport();
		cmdlist->setViewport(vp);
		cmdlist->setScissorRect({0,0, (LONG)vp.Width, (LONG)vp.Height});
		cmdlist->setPrimitiveType();

		struct 
		{
			Matrix world;
			Matrix view;
			Matrix proj;
		}transform;
		transform.view = cam->getViewMatrix();
		transform.proj = cam->getProjectionMatrix();



		for (auto& m : models)
		{
			const auto& mesh = m->getMesh();
			const auto& indices = mesh->getIndices();
			const auto& vertices = mesh->getVertices();

			cmdlist->setVertexBuffer(vertices);
			cmdlist->setIndexBuffer(indices);

			auto world = m->getTransform().GetTransposed();
			transform.world = *(Matrix*)&world;
			auto consts = m->getVSCBuffer();
			consts->blit(&transform);

			auto mat = m->getMaterial();
			cmdlist->setPipelineState(mat->pipelineState);
			mat->apply(consts, m->getPSCBuffer());

			auto numIndices = indices->getSize() / indices->getStride();
			cmdlist->drawIndexedInstanced(numIndices,1);
		}


	}
} ;


