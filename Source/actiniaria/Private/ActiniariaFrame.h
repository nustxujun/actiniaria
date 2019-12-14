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

#define SEARCH_PATH L"D:/workspace/ue4/test/Plugins/actiniaria/Source/actiniaria/Private/engine/"


class ActiniariaFrame :public Framework, public RenderContext
{
public:
	std::shared_ptr<ForwardPipeline> pipeline;
	Renderer::PipelineState::Ref pso;
	Renderer::Buffer::Ptr vertices;
	Renderer::Texture::Ref tex;
	ImguiText* fps;
	std::vector<Model::Ptr> models;

	ActiniariaFrame()
	{
		Renderer::getSingleton()->addSearchPath(SEARCH_PATH);
		pipeline = decltype(pipeline)(new ForwardPipeline());
	}

	void iterateObjects()
	{
		{
			auto camIter = TObjectIterator<ACameraActor>();
			Common::Assert(!!camIter, L"need camera");
			auto camact = *camIter;
			auto camcom = camact->GetCameraComponent();
			FMinimalViewInfo info;
			camcom->GetCameraView(0,info);
			auto maincamera = getMainCamera();
			const auto& vp = maincamera->getViewport();

			info.FOV = vp.Width / vp.Height;
			auto proj = info.CalculateProjectionMatrix();
			auto view = camact->GetTransform().Inverse().ToMatrixNoScale();

			maincamera->setProjectionMatrix(*(Matrix*)&proj);
			maincamera->setViewMatrix(*(Matrix*)&view);
			

		}


		std::map<FString, StaticMesh::Ptr> meshs;
		for (TObjectIterator<AStaticMeshActor> iter; iter; ++iter)
		{
			auto actor = *iter;
			auto component = actor->GetStaticMeshComponent();
			auto mesh = component->GetStaticMesh();

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
			models.push_back(model);
		}
	}

	void init()
	{
		auto renderer = Renderer::getSingleton();
		iterateObjects();

		auto vs = renderer->compileShader(L"shaders/scene_vs.hlsl", L"vs", L"vs_5_0");
		auto ps = renderer->compileShader(L"shaders/scene_vs.hlsl", L"ps", L"ps_5_0");
		std::vector<Renderer::Shader::Ptr> shaders = { vs, ps };
		std::vector<Renderer::RootParameter> rootparams = { D3D12_SHADER_VISIBILITY_PIXEL };
		rootparams[0].cbv32(0,0,16 * 3);


		Renderer::RenderState rs = Renderer::RenderState::Default;
		rs.setInputLayout({
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			});

		pso = renderer->createPipelineState(shaders, rs, rootparams);

		auto mainbar = ImguiObject::root()->createChild<ImguiMenuBar>(true);
		fps = mainbar->createChild<ImguiText>("test");
	}

	void renderScreen()
	{
	}

	void updateImpl()
	{
		static auto lastTime = GetTickCount64();
		auto cur = GetTickCount64();
		auto delta = cur - lastTime;
		if (delta > 0)
		{
			lastTime = cur;

			float time = 1000.0f / delta;
			static float history = time;

			history = history * 0.99f + time * 0.01f;

			std::stringstream ss;
			ss.precision(4);
			ss << history;
			fps->text = ss.str();
		}

		pipeline->update();
	}

	void renderScene(Camera::Ptr cam, UINT, UINT)
	{
		auto cmdlist = Renderer::getSingleton()->getCommandList();

		cmdlist->setPipelineState(pso);
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

			transform.world = *(Matrix*)&m->getTransform();
			cmdlist->set32BitConstants(0,16  * 3,&transform,0);

			auto numIndices = indices->getSize() / indices->getStride();
			cmdlist->drawIndexedInstanced(numIndices,1);
		}


	}
} ;


