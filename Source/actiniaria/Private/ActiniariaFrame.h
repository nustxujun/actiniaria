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


inline FMatrix convertMatrix(const FMatrix& m)
{
	FMatrix o = FMatrix::Identity;
	o.M[0][0] = m.M[1][1]; o.M[0][1] = m.M[1][2]; o.M[0][2] = m.M[1][0];
	o.M[1][0] = m.M[2][1]; o.M[1][1] = m.M[2][2]; o.M[1][2] = m.M[2][0];
	o.M[2][0] = m.M[0][1]; o.M[2][1] = m.M[0][2]; o.M[2][2] = m.M[0][0];
	o.M[3][0] = m.M[3][1]; o.M[3][1] = m.M[3][2]; o.M[3][2] = m.M[3][0];
	return o;
}

class ActiniariaFrame :public RenderContext, public Framework
{
public:
	std::shared_ptr<DefaultPipeline> pipeline;
	Renderer::PipelineState::Ref pso;
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

			info.FOV = vp.Width / vp.Height;

			std::cout << "FOV: " << info.FOV << std::endl;
			
			FMatrix proj;
			{
				float FarZ = 1000;
				float NearZ = 0.1f;
				float halfFov = info.FOV * 0.5f;
				float SinFov = std::sin(halfFov);
				float CosFov = std::cos(halfFov);

				float Height = CosFov / SinFov;
				float Width = 1.0f / info.AspectRatio;
				float fRange = FarZ / (FarZ - NearZ);

				FMatrix M;
				M.M[0][0] = Width;
				M.M[0][1] = 0.0f;
				M.M[0][2] = 0.0f;
				M.M[0][3] = 0.0f;

				M.M[1][0] = 0.0f;
				M.M[1][1] = Height;
				M.M[1][2] = 0.0f;
				M.M[1][3] = 0.0f;

				M.M[2][0] = 0.0f;
				M.M[2][1] = 0.0f;
				M.M[2][2] = fRange;
				M.M[2][3] = 1.0f;

				M.M[3][0] = 0.0f;
				M.M[3][1] = 0.0f;
				M.M[3][2] = -fRange * NearZ;
				M.M[3][3] = 0.0f;
		
				proj = M.GetTransposed();
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
			//view = convertMatrix(view);
			view = view.GetTransposed();
			maincamera->setProjectionMatrix(*(Matrix*)&proj);
			maincamera->setViewMatrix(*(Matrix*)&view);
	

		}


		std::map<FString, StaticMesh::Ptr> meshs;
		for (TObjectIterator<AStaticMeshActor> iter; iter; ++iter)
		{
			auto actor = *iter;
			auto component = actor->GetStaticMeshComponent();
			auto mesh = component->GetStaticMesh();
			if (mesh == nullptr )
				continue;
			StaticMesh::Ptr dstMesh;
			auto ret = meshs.find(mesh->GetName());
			if (ret != meshs.end())
				dstMesh = ret->second;
			else
			{
				dstMesh = StaticMesh::Ptr( new StaticMesh(*mesh->RenderData));
				meshs[mesh->GetName()] = dstMesh;
			}
			auto model = Model::Ptr(new Model());
			model->setMesh(dstMesh);
			model->setTramsform(actor->GetTransform().ToMatrixWithScale());
			model->setCBuffer(pso->createConstantBuffer(Renderer::Shader::ST_VERTEX,"Constant"));
			models.push_back(model);
		}
	}

	void init()
	{
		auto renderer = Renderer::getSingleton();

		auto vs = renderer->compileShader(L"shaders/scene_vs.hlsl", L"vs", L"vs_5_0");
		auto ps = renderer->compileShader(L"shaders/scene_vs.hlsl", L"ps", L"ps_5_0");
		std::vector<Renderer::Shader::Ptr> shaders = { vs, ps };

		Renderer::RenderState rs = Renderer::RenderState::Default;
		rs.setInputLayout({
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			});

		pso = renderer->createPipelineState(shaders, rs);


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
		cmdlist->setPipelineState(pso);


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

			auto mat = m->getTransform().GetTransposed();
			//auto mat = FMatrix::Identity;
			transform.world = *(Matrix*)&mat;
			auto consts = m->getCBuffer();
			consts->blit(&transform);
			pso->setVSConstant("Constant",consts);
			//pso->setVSConstant("Constant", &transform);

			auto numIndices = indices->getSize() / indices->getStride();
			cmdlist->drawIndexedInstanced(numIndices,1);
		}


	}
} ;


