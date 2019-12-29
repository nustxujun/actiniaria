#include "Engine/Common.h"
#include "Engine/D3DHelper.h"
#include "IPCFrame.h"

#include "Engine/RenderCommand.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"

void createMesh(const std::string& name, FStaticMeshRenderData & renderdata)
{

	auto& mesh = renderdata.LODResources[0];
	auto numVertices = mesh.GetNumVertices();
	auto& positions = mesh.VertexBuffers.PositionVertexBuffer;
	auto& vertices = mesh.VertexBuffers.StaticMeshVertexBuffer;

	auto vertexstride = (sizeof(FVector) + sizeof(FVector2D));
	std::vector<char> vertexData(vertexstride * numVertices);
	char* data = vertexData.data();
	for (auto i = 0; i < numVertices; ++i)
	{
		const auto& pos = positions.VertexPosition(i);
		const auto& uv = vertices.GetVertexUV(i, 0);

		memcpy(data, &pos, sizeof(FVector));
		memcpy(data + sizeof(FVector), &uv, sizeof(FVector2D));
		data += vertexstride;
	}

	//mVertices = renderer->createBuffer(cacheData.size(), stride, D3D12_HEAP_TYPE_DEFAULT, cacheData.data(), cacheData.size());

	auto& indices = mesh.IndexBuffer;
	auto numIndices = indices.GetNumIndices();
	std::vector<char> indexData;
	indexData.resize(indices.GetIndexDataSize());
	auto indexstride = indices.Is32Bit() ? 4 : 2;
	data = indexData.data();
	for (auto i = 0; i < numIndices; ++i)
	{
		auto index = indices.GetIndex(i);
		memcpy(data, &index, indexstride);
		data += indexstride;
	}

	//mIndices = renderer->createBuffer(cacheData.size(), stride, D3D12_HEAP_TYPE_DEFAULT, cacheData.data(), cacheData.size());

	RenderCommand::getSingleton()->createMesh(name,
		vertexData.data(), vertexData.size(), numVertices, vertexstride, indexData.data(), indexData.size(), numIndices, indexstride);

}

void createModel(const std::string& name, const std::string& meshname, const Matrix& transform, const std::string& material)
{
	RenderCommand::getSingleton()->createModel(name,{meshname},transform, material);
}


DXGI_FORMAT convertFormat(ETextureSourceFormat f)
{
	switch (f)
	{
	case TSF_G8:return DXGI_FORMAT_R8_UNORM;
	case TSF_BGRA8:return DXGI_FORMAT_B8G8R8A8_UNORM;
	case TSF_RGBA16:return DXGI_FORMAT_R16G16B16A16_UNORM;
	case TSF_RGBA16F:return DXGI_FORMAT_R16G16B16A16_FLOAT;
	default:
		Common::Assert(false, "unsupported ue4 texture format");
		return DXGI_FORMAT_UNKNOWN;
	}
}

std::string mapMaterial(const std::string& name)
{
	if (name == "M_Chair")
		return "shaders/customization/chair_ps.hlsl";
	else if (name == ("M_TableRound"))
		return "shaders/customization/table_ps.hlsl";
	else
		return "shaders/scene_ps.hlsl";
}

std::string mapTextureType(TEnumAsByte<enum EMaterialSamplerType> type)
{
	switch (type)
	{
	case SAMPLERTYPE_Color: return "albedo";
	case SAMPLERTYPE_Normal: return "normal";
		break;
	}

	return "unknown";
}

void createMaterial( UMaterialInterface* material)
{
	std::string name = U2M(*material->GetName());
	auto base = material->GetBaseMaterial();
	std::map<std::string, Vector4> parameters;

	std::map<std::string, std::string> textures;
	for (auto& expr : base->Expressions)
	{
		{
			const UMaterialExpressionVectorParameter* param = Cast<const UMaterialExpressionVectorParameter>(expr);
			if (param)
			{
				TArray<FMaterialParameterInfo> info;
				TArray<FGuid> id;
				param->GetAllParameterInfo(info, id, nullptr);
				std::string name = TCHAR_TO_ANSI(*info[0].Name.ToString());
				parameters[name] = *(Vector4*)&param->DefaultValue;
			}
		}
		{
			const UMaterialExpressionTextureSample* param = Cast<const UMaterialExpressionTextureSample>(expr);
			if (param)
			{
				auto type = param->SamplerType;
				auto t = param->Texture;

				uint32 width = (uint32)t->GetSurfaceWidth();
				uint32 height = (uint32)t->GetSurfaceHeight();
				auto& source = t->Source;
				auto format = source.GetFormat();

				uint32 BytesPerPixel = source.GetBytesPerPixel();
				auto src = source.LockMip(0);
				RenderCommand::getSingleton()->createTexture(U2M(*t->GetName()),width, height, convertFormat(format), src);
				//auto tex = Renderer::getSingleton()->createTexture(width, height, convertFormat(format), src);
				textures[mapTextureType(type)] = U2M(*t->GetName());
				//tex->setName(*t->GetName());
				source.UnlockMip(0);
			}
		}
	}

	RenderCommand::getSingleton()->createMaterial(name,"shaders/scene_vs.hlsl", mapMaterial(name),parameters,textures);

}

IPCFrame::IPCFrame()
{
	//if (!::AllocConsole())
	//{
	//	Common::checkResult(GetLastError(), " init actiniaria");
	//}
	//else
	//{
	//	freopen("CONOUT$", "w", stdout);
	//	std::cout << "console started." << std::endl;
	//}
	//FString path = GetPluginPath() + "/Source/actiniaria/Private/engine/";

	RenderCommand::init(true);
}

IPCFrame::~IPCFrame()
{
}

void IPCFrame::iterateObjects()
{

	{
		auto camIter = TObjectIterator<ACameraActor>();
		Common::Assert(!!camIter, "need camera");
		auto camact = *camIter;
		auto camcom = camact->GetCameraComponent();
		FMinimalViewInfo info;
		camcom->GetCameraView(0, info);

		FMatrix proj;
		float FarZ = 1000;
		float NearZ = 0.1f;
		float halfFov = info.FOV * 0.5f;
		float width = 800;
		float height = 600;

		proj = FPerspectiveMatrix(halfFov, width, height, NearZ, FarZ).GetTransposed();


		auto rot = camact->GetTransform().GetRotation().Rotator();
		FMatrix ViewPlanesMatrix = FMatrix(
			FPlane(0, 0, 1, 0),
			FPlane(1, 0, 0, 0),
			FPlane(0, 1, 0, 0),
			FPlane(0, 0, 0, 1));
		auto rotmat = FInverseRotationMatrix(rot) * ViewPlanesMatrix;

		auto view = FTranslationMatrix(-camact->GetTransform().GetLocation()) * rotmat;
		view = view.GetTransposed();

		RenderCommand::getSingleton()->createCamera("main", *(Matrix*)&view, *(Matrix*)&proj, { 0,0, width , height, 0.0f, 1.0f });

	}

	std::set<FString> meshs;
	std::set<FString> materials;
	for (TObjectIterator<AStaticMeshActor> iter; iter; ++iter)
	{
		auto actor = *iter;
		auto component = actor->GetStaticMeshComponent();
		auto mesh = component->GetStaticMesh();
		if (mesh == nullptr)
			continue;

		auto material = mesh->GetMaterial(0);
		if (material == nullptr)
			continue;
		{
			auto ret = materials.find(material->GetName());
			if (ret != materials.end())
			{ }
			else
			{
				createMaterial(material);
				materials.insert(material->GetName());
			}
		}

		{
			auto ret = meshs.find(mesh->GetName());
			if (ret != meshs.end())
			{
			}
			else
			{
				//dstMesh = StaticMesh::Ptr(new StaticMesh(*mesh->RenderData));
				createMesh(U2M(*mesh->GetName()), *mesh->RenderData);
				meshs.insert(mesh->GetName());
			}
		}

		auto world = actor->GetTransform().ToMatrixWithScale().GetTransposed();
		createModel(U2M(*actor->GetName()), U2M(*mesh->GetName()), *(Matrix*)&world, U2M(*material->GetName()));
		//auto model = Model::Ptr(new Model());
		//model->setMaterial(mat);
		//model->setMesh(dstMesh);
		//model->setTramsform(actor->GetTransform().ToMatrixWithScale());
		//model->setVSCBuffer(mat->pipelineState->createConstantBuffer(Renderer::Shader::ST_VERTEX, "VSConstant"));
		//model->setPSCBuffer(mat->pipelineState->createConstantBuffer(Renderer::Shader::ST_PIXEL, "PSConstant"));

		//models.push_back(model);
	}


	RenderCommand::getSingleton()->done();
}


