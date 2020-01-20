#include "MaterialParser.h"

#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"


#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Base.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"

std::string tostring(const FVector4& v)
{
	std::stringstream ss;
	ss << "half4(" << v.X << "," << v.Y << "," << v.Z << "," << v.W << ")";
	return ss.str();
}

MaterialParser::MaterialParser()
{
	mExprs["MaterialExpressionVectorParameter"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto vector = Cast<UMaterialExpressionVectorParameter>(expr);
		if (pin->PinName == "R")
		{
			ss << vector->DefaultValue.R;
		}
		else if (pin->PinName == "G")
		{ 
			ss << vector->DefaultValue.G;
		}
		else if (pin->PinName == "B")
		{
			ss << vector->DefaultValue.B;
		}
		else if (pin->PinName == "A")
		{
			ss << vector->DefaultValue.A;
		}
		else
			ss <<  tostring(vector->DefaultValue) ;
	};

	mExprs["MaterialExpressionLinearInterpolate"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto interpolate = Cast<UMaterialExpressionLinearInterpolate>(expr);
		
		ss << "lerp(";

		// A
		if (inputs[0]->LinkedTo.Num() == 0)
		{
			ss << interpolate->ConstA ; 
		}
		else
		{
			parse(inputs[0]->LinkedTo[0],ss);
		}
		ss << ",";

		// B
		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << interpolate->ConstB;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}
		ss << ",";

		if (inputs[2]->LinkedTo.Num() == 0)
		{
			ss << interpolate->ConstAlpha ;
		}
		else
		{
			parse(inputs[2]->LinkedTo[0], ss);
		}
		ss << ")";


	};

	mExprs["MaterialExpressionMultiply"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto mul = Cast<UMaterialExpressionMultiply>(expr);
		
		if (inputs[0]->LinkedTo.Num() == 0)
		{
			ss << mul->ConstA;
		}
		else
		{
			parse(inputs[0]->LinkedTo[0], ss);
		}

		ss << " * ";

		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << mul->ConstB;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}


	};

	mExprs["MaterialExpressionScalarParameter"] = [](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto scalar = Cast<UMaterialExpressionScalarParameter>(expr);
		
		ss << scalar->DefaultValue;
	};

	mExprs["MaterialExpressionTextureSample"] = [&, &res = mBoundResources, &def = mDefinations](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto sampler = Cast<UMaterialExpressionTextureSample>(expr);
		const auto& name = sampler->GetName();
		const auto& texture = sampler->Texture->GetName();

		// bind resource
		if (res.find(texture) == res.end())
		{
			res[texture] = "Texture2D " + U2M(*texture) + ";";
		}


		// define variable and sample texture
		if (def.find(name) == def.end())
		{
			std::string uv = "input.uv";
			if (inputs[0]->LinkedTo.Num() != 0)
			{
				std::stringstream uvparser;
				parse(inputs[0]->LinkedTo[0], uvparser);
				uv = uvparser.str();
			}

			def[name] = "half4 " + U2M(*name) + " = " + U2M(*texture) + ".Sample(linearSampler," + uv + ")";
		}

		ss << U2M(*name);

		if (pin->PinName == "R")
		{
			ss << ".r";
		}
		else if (pin->PinName == "G")
		{
			ss << ".g";
		}
		else if (pin->PinName == "B")
		{
			ss << ".b";
		}
		else if (pin->PinName == "A")
		{
			ss << ".a";
		}


	};
	mExprs["MaterialExpressionConstant"] = [](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto constant = Cast<UMaterialExpressionConstant>(expr);

		ss << constant->R;
	};
	mExprs["MaterialExpressionFresnel"] = [](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto fresnel= Cast<UMaterialExpressionFresnel>(expr);

		ss << 0.5f;
	};
	mExprs["MaterialExpressionConstant3Vector"] = [](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto constant = Cast<UMaterialExpressionConstant3Vector>(expr);
		const auto& var = constant->Constant;
		ss << "half4(" << var.R << "," << var.G << "," << var.B << "," << var.B << ")";
	};
	mExprs["MaterialExpressionAdd"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto add = Cast<UMaterialExpressionAdd>(expr);

		if (inputs[0]->LinkedTo.Num() == 0)
		{
			ss << add->ConstA;
		}
		else
		{
			parse(inputs[0]->LinkedTo[0], ss);
		}

		ss << " + ";

		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << add->ConstB;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}

	};
	mExprs["MaterialExpressionTextureCoordinate"] = [](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto tc = Cast<UMaterialExpressionTextureCoordinate>(expr);
		ss << "input.uv";
		if (tc->CoordinateIndex > 0)
		{
			ss << tc->CoordinateIndex;
		}

		ss << " * half2(" << tc->UTiling << "," << tc->VTiling<< ")";
	};

	
}

std::string MaterialParser::operator()(UMaterialInterface* material)
{
	mBoundResources.clear();
	mDefinations.clear();
	mMacros.clear();
	auto base = material->GetBaseMaterial();
	if (!base->MaterialGraph)
	{
		base->MaterialGraph = CastChecked<UMaterialGraph>(FBlueprintEditorUtils::CreateNewGraph(base, NAME_None, UMaterialGraph::StaticClass(), UMaterialGraphSchema::StaticClass()));
	}
	base->MaterialGraph->Material = base;
	base->MaterialGraph->MaterialFunction = nullptr;
	base->MaterialGraph->OriginalMaterialFullName = base->GetName();
	base->MaterialGraph->RebuildGraph();
	//base->MaterialGraph->RealtimeDelegate.BindSP(this, &FMaterialEditor::IsToggleRealTimeExpressionsChecked);
	//base->MaterialGraph->MaterialDirtyDelegate.BindSP(this, &FMaterialEditor::SetMaterialDirty);
	//base->MaterialGraph->ToggleCollapsedDelegate.BindSP(this, &FMaterialEditor::ToggleCollapsed);

	auto graph = base->MaterialGraph;
	
	TArray<UEdGraphPin*> InputPins;
	graph->RootNode->GetInputPins(InputPins);
	TArray<UEdGraphNode*> NodesToCheck;

	std::stringstream ss;

	for (int32 Index = 0; Index < InputPins.Num(); ++Index)
	{
		if (graph->MaterialInputs[Index].IsVisiblePin(graph->Material)
			&& InputPins[Index]->LinkedTo.Num() > 0 && InputPins[Index]->LinkedTo[0])
		{
			auto type = graph->RootNode->GetInputType(InputPins[Index]);
			auto semantic = graph->MaterialInputs[Index].GetProperty();
			switch (semantic)
			{
			case MP_Normal: mMacros[L"HAS_NORMALMAP"] = "HAS_NORMALMAP";break;
			}

			FString var = "	";
			switch (type)
			{
			case MCT_Float:
			case MCT_Float1: var += "half ";break;
			case MCT_Float2: var += "half2 "; break;
			case MCT_Float3: var += "half3 "; break;
			case MCT_Float4: var += "half4 "; break;
			default: 
				Common::Assert(false, "unsupported type");
			}

			auto name = graph->MaterialInputs[Index].GetName().ToString();
			var += name.Replace(L" ", L"_");
			ss << U2M(*var) << " = ";
			parse(InputPins[Index]->LinkedTo[0], ss);
			ss << ";\n";
		}
	}

	std::string shader;
	shader += "#ifndef __SHADER_CONTENT__\n";
	shader += "#error need shader content\n";
	shader += "#endif\n";
	for (auto& m: mMacros)
		shader += "#define " + m.second + "\n";
	std::vector<std::string> headers = {
		"common.hlsl",
		"pbr.hlsl",
	};
	for (auto& h: headers)
		shader += std::string("#include ") + "\"" + h + "\"\n";

	
	for (auto& r: mBoundResources)
		shader += r.second + ";\n";

	shader += "sampler linearSampler:register(s0);\n";

	shader += "half4 ps(PSInput input):SV_TARGET \n{\n";

	for(auto& d: mDefinations)
		shader += "	" + d.second + ";\n";

	shader+= ss.str();

	shader += "#ifdef HAS_NORMALMAP\n";
	shader += "	half3 _normal = calNormal(Normal.xyz, input.normal.xyz, input.tangent.xyz, input.binormal.xyz);\n";
	shader += "#else\n";
	shader += "	half3 _normal = input.normal.xyz;\n";
	shader += "#endif\n";
	//shader += "	half3 _final = directBRDF(Roughness, Metallic, F0_DEFAULT, Base_Color.rgb, _normal.xyz,-sundir, campos - input.worldPos);\n";
	//shader += "	return half4(_final ,1) * suncolor;\n";
	//shader += "	return half4(_normal * 0.5 + 0.5,1);";

	shader += "	__SHADER_CONTENT__";
	shader += "\n}";
	return shader;
}

void MaterialParser::parse(UEdGraphPin * pin, std::stringstream & ss)
{
	
	UMaterialGraphNode* graphnode = Cast<UMaterialGraphNode>(pin->GetOwningNode());
	TArray<UEdGraphPin*> inputs;
	graphnode->GetInputPins(inputs);

	TArray<UEdGraphPin*> outputs;
	graphnode->GetOutputPins(outputs);


	auto expr = graphnode->MaterialExpression;
	auto name = expr->GetFName();
	
	auto cal = mExprs[name.GetPlainNameString()];
	if (cal)
	{
		ss << "(";
		cal(inputs, outputs, expr,pin, ss);
		ss << ")";
	}
	else
		Common::Assert(false, "cannot parse expression: " + (U2M(*name.ToString())));

}
