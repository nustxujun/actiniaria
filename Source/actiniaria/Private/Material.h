#pragma once

#include "Engine/Renderer.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"

class Material
{
public:
	using Ptr = std::shared_ptr<Material>;
	Material(UMaterialInterface* material, Renderer::PipelineState::Ref pso)
	{
		pipelineState = pso;
		auto base = material->GetBaseMaterial();
		
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
				
					parameters[ name] = param->DefaultValue;
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
					auto tex = Renderer::getSingleton()->createTexture(width, height, convertFormat(format), src );
					textures[type] = (tex);
					tex->setName(*t->GetName());
					source.UnlockMip(0);
				}
			}
		}
	}

	void apply(const Renderer::ConstantBuffer::Ptr& vscbuffer, const Renderer::ConstantBuffer::Ptr& pscbuffer)
	{
		if (vscbuffer)
			pipelineState->setVSConstant("VSConstant", vscbuffer);
		if (pscbuffer)
		{
			for (auto& p : parameters)
				pscbuffer->setVariable(p.first, &p.second);
			
			pipelineState->setPSConstant("PSConstant", pscbuffer);
		}
		if (textures.find(SAMPLERTYPE_Color) != textures.end())
			pipelineState->setPSResource("albedo", textures[SAMPLERTYPE_Color]->getHandle());
	}
private:
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

public:
	std::map<TEnumAsByte<enum EMaterialSamplerType>, Renderer::Texture::Ref> textures;
	Renderer::PipelineState::Ref pipelineState;
	std::map<std::string, FVector4> parameters;
};