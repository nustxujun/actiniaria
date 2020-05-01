#include "IPCFrame.h"

#include "Core.h"

#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"

#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"

#include"Engine/Light.h"
#include"Engine/DirectionalLight.h"
#include "Engine/ReflectionCapture.h"
#include "Components/ReflectionCaptureComponent.h"
#include "Engine/MapBuildDataRegistry.h"

#include "MaterialParser.h"
#include <string>
#include <regex>
#include <locale>
#include <dxgi.h>


static std::string convert(const std::wstring& str)
{
	std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>>
		converter(new std::codecvt<wchar_t, char, std::mbstate_t>("CHS"));
	return converter.to_bytes(str);
}

void IPCFrame::createMesh(const std::string& name, FStaticMeshRenderData & renderdata)
{

	auto& mesh = renderdata.LODResources[0];
	UINT numVertices = mesh.GetNumVertices();
	auto& positions = mesh.VertexBuffers.PositionVertexBuffer;
	auto& colors = mesh.VertexBuffers.ColorVertexBuffer;
	auto& vertices = mesh.VertexBuffers.StaticMeshVertexBuffer;

	UINT vertexstride = (
		sizeof(FVector) +  // pos
		sizeof(FVector2D) + //uv
		sizeof(FVector) +  // normal
		sizeof(FVector) +  // tangent
		sizeof(FVector) +// binormal
		sizeof(FColor) // color
	); 	
	std::vector<char> vertexData(vertexstride * numVertices);
	char* data = vertexData.data();
	auto move = [](char*& data, const auto& value)
	{
		memcpy(data, &value, sizeof(value));
		data += sizeof(value);
	};
	for (UINT i = 0; i < numVertices; ++i)
	{
		FVector pos = positions.VertexPosition(i);
		FVector2D uv = vertices.GetVertexUV(i, 0);
		FVector normal = vertices.VertexTangentZ(i);
		FVector tangent = vertices.VertexTangentX(i);
		FVector binormal = vertices.VertexTangentY(i);
		FColor color = (int)colors.GetNumVertices() > i? colors.VertexColor(i):FColor(0xffffffff);


		move(data, pos);
		move(data, uv);
		move(data, normal);
		move(data, tangent);
		move(data, binormal);
		move(data, color);


		//memcpy(data, &pos, sizeof(FVector));
		//memcpy(data + sizeof(FVector), &uv, sizeof(FVector2D));
		//data += vertexstride;
	}

	//mVertices = renderer->createBuffer(cacheData.size(), stride, D3D12_HEAP_TYPE_DEFAULT, cacheData.data(), cacheData.size());

	auto& indices = mesh.IndexBuffer;
	UINT numIndices = indices.GetNumIndices();
	std::vector<char> indexData;
	indexData.resize(indices.GetIndexDataSize());
	UINT indexstride = indices.Is32Bit() ? 4 : 2;
	data = indexData.data();
	for (UINT i = 0; i < numIndices; ++i)
	{
		auto index = indices.GetIndex(i);
		memcpy(data, &index, indexstride);
		data += indexstride;
	}

	struct SubMesh
	{
		UINT materialIndex;
		UINT startIndex;
		UINT numIndices;
	};
	std::vector<SubMesh> subs;

	for (auto& s : mesh.Sections)
	{
		subs.push_back({
			(UINT)s.MaterialIndex,
			(UINT)s.FirstIndex,
			(UINT)s.NumTriangles * 3,
		});
	}

	mIPC << "createMesh" << name ;
	UINT bytesofvertices = (UINT)vertexData.size();
	mIPC << bytesofvertices << numVertices << vertexstride;
	mIPC.send(vertexData.data(), bytesofvertices);

	UINT bytesofindices = indexData.size();
	mIPC << bytesofindices << numIndices << indexstride;
	mIPC.send(indexData.data(), bytesofindices);


	mIPC << (UINT) subs.size();

	for (auto& s: subs)
		mIPC << s;


	//rendercmd.createMesh(name,
		//vertexData.data(), vertexData.size(), numVertices, vertexstride, indexData.data(), indexData.size(), numIndices, indexstride, subs);

}


void IPCFrame::createStaticMesh(AStaticMeshActor * actor)
{
	auto component = actor->GetStaticMeshComponent();
	if (component == nullptr)
		return;
	auto mesh = component->GetStaticMesh();
	if (mesh == nullptr)
		return;

	//auto material = mesh->GetMaterial(0);
	auto numMaterials = component->GetNumMaterials();

	std::vector<std::string> mats;
	for (int i = 0; i < numMaterials; ++i)
	{
		auto material = component->GetMaterial(i);
		if (material == nullptr)
			continue;

		auto ret = materials.find(material->GetName());
		if (ret != materials.end())
		{
		}
		else
		{
			createMaterial(material, textures);
			materials.insert(material->GetName());
		}

		mats.push_back(convert(*material->GetName()));

	}

	if (mats.size() != numMaterials)
		return;



	{
		auto ret = meshs.find(mesh->GetName());
		if (ret != meshs.end())
		{
		}
		else
		{
			//dstMesh = StaticMesh::Ptr(new StaticMesh(*mesh->RenderData));
			createMesh(convert(*mesh->GetName()), *mesh->RenderData);
			meshs.insert(mesh->GetName());
		}
	}

	auto world = actor->GetTransform().ToMatrixWithScale().GetTransposed();
	auto nworld = actor->GetTransform().Inverse().ToMatrixWithScale(); // world -> inverse -> transpose -> normal world

	FVector center;
	FVector extent;
	actor->GetActorBounds(false,center, extent);

	mIPC << "createModel" << convert(*actor->GetName());
	mIPC << (UINT)1U << convert(*mesh->GetName()) << world << nworld << center << extent;
	mIPC << (UINT) mats.size();
	for (auto& m: mats)
		mIPC << m;

	//rendercmd.createModel(convert(*actor->GetName()), { convert(*mesh->GetName()) }, *(Matrix*)&world, *(Matrix*)&nworld, mats);

}


static DXGI_FORMAT convertFormat(ETextureSourceFormat f)
{
	switch (f)
	{
	case TSF_G8:return DXGI_FORMAT_R8_UNORM;
	case TSF_BGRA8:return DXGI_FORMAT_B8G8R8A8_UNORM;
	case TSF_RGBA16:return DXGI_FORMAT_R16G16B16A16_UNORM;
	case TSF_RGBA16F:return DXGI_FORMAT_R16G16B16A16_FLOAT;
	default:
		assert(0 && "unsupported ue4 texture format");
		return DXGI_FORMAT_UNKNOWN;
	}
}

static UINT sizeof_format(ETextureSourceFormat f)
{
	switch (f)
	{
	case TSF_G8:return 1U;
	case TSF_BGRA8:return 4U;
	case TSF_RGBA16:return 8U;
	case TSF_RGBA16F:return 8U;
	default:
		assert(0 && "unsupported ue4 texture format");
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

void IPCFrame::createMaterial( UMaterialInterface* material, std::set<std::string>& textureMap)
{
	std::string name = convert(*material->GetName());
	auto base = material->GetBaseMaterial();
	auto toVariable = [](const std::string & str)
	{
		std::regex r("[^0-9a-zA-Z_]");
		return "_" + std::regex_replace(str, r, "_");
	};
	//std::map<std::string, Vector4> parameters;

	std::set< std::string> textures;
	for (auto& expr : base->Expressions)
	{
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

				std::string texturename = toVariable(convert(*t->GetName()));
				if (textureMap.find(texturename) == textureMap.end())
				{
					auto src = source.LockMip(0);
					auto size = sizeof_format(format) * width * height;
					mIPC << "createTexture" << texturename << width << height << convertFormat(format) << (bool)t->SRGB << size;
					mIPC.send(src, size);
					
					//rendercmd.createTexture(texturename, width, height, convertFormat(format), (bool)t->SRGB,src);
					source.UnlockMip(0);
				}
				textureMap.insert(texturename);
				textures.insert(texturename);
			}
		}
	}
	MaterialParser parser;
	mIPC << "createMaterial" << name << "shaders/scene_vs.hlsl" << name + "_ps" << parser(material);
	mIPC << (UINT) textures.size();
	for (auto& t: textures)
		mIPC << t;
	//rendercmd.createMaterial(name,"shaders/scene_vs.hlsl", name + "_ps", parser(material),textures);

}

void IPCFrame::createSkySphere(const std::string & name, const std::string & meshname, const std::string & mat, const FMatrix& tran)
{
	mIPC << "createSky" << name << meshname << mat << tran;
	//rendercmd.createSky(name, meshname, mat, tran);
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
	mIPC.listen("renderstation");
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

		mIPC << "createLight" << convert(*light->GetName()) << UINT(0) << color << FVector(dir);
		//rendercmd.createLight(convert(*light->GetName()),0,*(Color*)&color, *(Vector3*)&dir);

	}

}

void IPCFrame::iterateCapture()
{
	for (TObjectIterator< AReflectionCapture> iter; iter; ++iter)
	{
		auto actor = *iter;
		auto comp = actor->GetCaptureComponent();
		if (comp == nullptr)
			continue;
		auto data = comp->GetMapBuildData();
		if (data == nullptr)
			continue;
		auto transfrom = comp->GetComponentTransform();
		auto mat = transfrom.ToMatrixWithScale().GetTransposed();


		mIPC
			<< "createReflectionProbe" 
			<< convert(*actor->GetName()) 
			<< mat 
			<< comp->GetInfluenceBoundingRadius() 
			<< data->Brightness 
			<< (UINT)data->CubemapSize;
		UINT size = data->FullHDRCapturedData.Num();
		mIPC << size;
		mIPC.send(data->FullHDRCapturedData.GetData(), size);
		//rendercmd.createReflectionProbe(
		//	convert(*actor->GetName()),
		//	*(Matrix*)&mat,
		//	comp->GetInfluenceBoundingRadius(),
		//	data->Brightness,data->CubemapSize, 
		//	data->FullHDRCapturedData.GetData(), 
		//	data->FullHDRCapturedData.Num()
		//);
	}
}

void IPCFrame::init()
{
	iterateObjects();
	iterateLights();
	iterateCapture();
	mIPC << "done";
}

void IPCFrame::iterateObjects()
{

	{
		auto camIter = TObjectIterator<ACameraActor>();
		assert(!!camIter && "need camera");
		auto camact = *camIter;
		auto camcom = camact->GetCameraComponent();
		FMinimalViewInfo info;
		camcom->GetCameraView(0, info);

		FMatrix proj;
		float FarZ = GNearClippingPlane;
		float NearZ = GNearClippingPlane;
		float halfFov = info.FOV * 0.5f * PI / 180.0f;
		float height = 600;
		float width = info.AspectRatio * height;

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

		mIPC 
			<< "createCamera" 
			<< "main" 
			<< FVector(pos)
			<< FVector(dir)
			<< view 
			<< proj 
			<< 0.0f 
			<< 0.0f 
			<< width 
			<< height 
			<< 0.0f
			<< 1.0f;

		//rendercmd.createCamera("main",{pos.X, pos.Y, pos.Z}, {dir.X, dir.Y, dir.Z}, *(Matrix*)&view, *(Matrix*)&proj, { 0,0, width , height, 0.0f, 1.0f });

	}

	std::set<FString> meshs;
	std::set<FString> materials;
	std::set<std::string> textures;
	for (TObjectIterator<AStaticMeshActor> iter; iter; ++iter)
	{
		auto actor = *iter;
		createStaticMesh(actor);
	
	}

	for (TObjectIterator<AActor> iter; iter; ++iter)
	{
		auto& actor = iter;
		auto cls = actor->GetClass();
		auto clsname = cls->GetName();
		if (clsname != (L"BP_Sky_Sphere_C"))
		{
			continue;
		}
		OutputDebugString(*clsname);
		OutputDebugString(L"\n");

		auto comp = actor->GetComponentByClass(UStaticMeshComponent::StaticClass());

		if (comp == nullptr)
			continue;

		auto mesh = Cast<UStaticMeshComponent>(comp)->GetStaticMesh();
		if (mesh == nullptr)
			continue;

		auto material = Cast<UStaticMeshComponent>(comp)->GetMaterial(0);
		if (material == nullptr)
			continue;

		auto ret = materials.find(material->GetName());
		if (ret != materials.end())
		{
		}
		else
		{
			createMaterial(material, textures);
			materials.insert(material->GetName());
		}


		{
			auto ret = meshs.find(mesh->GetName());
			if (ret != meshs.end())
			{
			}
			else
			{
				//dstMesh = StaticMesh::Ptr(new StaticMesh(*mesh->RenderData));
				createMesh(convert(*mesh->GetName()), *mesh->RenderData);
				meshs.insert(mesh->GetName());
			}
		}

		auto world = actor->GetTransform().ToMatrixWithScale().GetTransposed();
		FVector center;
		FVector extent;
		actor->GetActorBounds(false, center, extent);

		mIPC << "createSky" << convert(*actor->GetName()) << convert(*mesh->GetName()) << convert(*material->GetName()) << world << center << extent;
	}

}


