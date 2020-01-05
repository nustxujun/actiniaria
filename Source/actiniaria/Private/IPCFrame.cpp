#include "IPCFrame.h"
#include "Engine/Common.h"
#include "Engine/D3DHelper.h"

#include "Core.h"

#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"

#include"Engine/Light.h"
#include"Engine/DirectionalLight.h"

#include "MaterialParser.h"

void IPCFrame::createMesh(const std::string& name, FStaticMeshRenderData & renderdata)
{

	auto& mesh = renderdata.LODResources[0];
	auto numVertices = mesh.GetNumVertices();
	auto& positions = mesh.VertexBuffers.PositionVertexBuffer;
	auto& vertices = mesh.VertexBuffers.StaticMeshVertexBuffer;

	auto vertexstride = (
		sizeof(FVector) +  // pos
		sizeof(FVector2D) + //uv
		sizeof(FVector) +  // normal
		sizeof(FVector) +  // tangent
		sizeof(FVector) ); // binormal
	std::vector<char> vertexData(vertexstride * numVertices);
	char* data = vertexData.data();
	auto move = [](char*& data, const auto& value)
	{
		memcpy(data, &value, sizeof(value));
		data += sizeof(value);
	};
	for (auto i = 0; i < numVertices; ++i)
	{
		FVector pos = positions.VertexPosition(i);
		FVector2D uv = vertices.GetVertexUV(i, 0);
		FVector normal = vertices.VertexTangentZ(i);
		FVector tangent = vertices.VertexTangentX(i);
		FVector binormal = vertices.VertexTangentY(i);

		move(data, pos);
		move(data, uv);
		move(data, normal);
		move(data, tangent);
		move(data, binormal);


		//memcpy(data, &pos, sizeof(FVector));
		//memcpy(data + sizeof(FVector), &uv, sizeof(FVector2D));
		//data += vertexstride;
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

	rendercmd.createMesh(name,
		vertexData.data(), vertexData.size(), numVertices, vertexstride, indexData.data(), indexData.size(), numIndices, indexstride);

}

void IPCFrame::createModel(const std::string& name, const std::string& meshname, const Matrix& transform, const std::string& material)
{
	rendercmd.createModel(name,{meshname},transform, material);
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
	case SAMPLERTYPE_Color: return "albedoMap";
	case SAMPLERTYPE_Normal: return "normalMap";
		break;
	}

	return "unknown";
}

void IPCFrame::createMaterial( UMaterialInterface* material)
{
	std::string name = U2M(*material->GetName());
	auto base = material->GetBaseMaterial();


	std::map<std::string, Vector4> parameters;

	std::set< std::string> textures;
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
				rendercmd.createTexture(U2M(*t->GetName()),width, height, convertFormat(format), src);
				//auto tex = Renderer::getSingleton()->createTexture(width, height, convertFormat(format), src);
				textures.insert(U2M(*t->GetName()));
				//tex->setName(*t->GetName());
				source.UnlockMip(0);
			}
		}
	}
	MaterialParser parser;
	rendercmd.createMaterial(name,"shaders/scene_vs.hlsl", name + "_ps", parser(material),textures);

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
	rendercmd.init(true);
}

IPCFrame::~IPCFrame()
{
}

void IPCFrame::iterateLights()
{
	for (TObjectIterator<ADirectionalLight> iter; iter; ++iter)
	{
		auto light = *iter;
		auto dir = light->GetTransform().ToMatrixNoScale().TransformFVector4(FVector4{1,0,0,0});
		auto brightness = light->GetBrightness();
		auto color = light->GetLightColor() * brightness;
		rendercmd.createLight(U2M(*light->GetName()),0,*(Color*)&color, *(Vector3*)&dir);
	}

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
		auto dir = camact->GetTransform().ToMatrixNoScale().TransformVector({1,0,0});
		auto pos = camact->GetTransform().GetLocation();
		auto view = FTranslationMatrix(-camact->GetTransform().GetLocation()) * rotmat;
		view = view.GetTransposed();

		rendercmd.createCamera("main",{pos.X, pos.Y, pos.Z}, {dir.X, dir.Y, dir.Z}, *(Matrix*)&view, *(Matrix*)&proj, { 0,0, width , height, 0.0f, 1.0f });

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


}


