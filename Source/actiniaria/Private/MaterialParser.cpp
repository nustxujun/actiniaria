#include "MaterialParser.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"

#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionSphereMask.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionCrossProduct.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionObjectPositionWS.h"




#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphSchema.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "MaterialGraph/MaterialGraphNode_Base.h"
#include "MaterialGraph/MaterialGraphNode_Root.h"

#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include <regex>
#include "Windows/MinWindows.h"


static std::string format()
{
	return {};
}

template<class T, class ... Args>
static std::string format(const T& v, Args&& ... args)
{
	std::stringstream ss;
	ss << v << format(args...);
	return ss.str();
}

static void Assert(bool v, const std::string& msg)
{
	if (!v)
	{
		::MessageBoxA(NULL,msg.c_str(), NULL,NULL);
	}
}

static std::string convertToMulti(const std::wstring& str)
{
	std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>>
		converter(new std::codecvt<wchar_t, char, std::mbstate_t>("CHS"));
	return converter.to_bytes(str);
}


static std::string tostring(const FVector4& v)
{
	std::stringstream ss;
	ss << "half4(" << v.X << "," << v.Y << "," << v.Z << "," << v.W << ")";
	return ss.str();
}

MaterialParser::MaterialParser()
{
	mExprs["MaterialExpressionVectorParameter"] = [&, &overrides = mOverrideVectorParameters](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto vector = Cast<UMaterialExpressionVectorParameter>(expr);

		auto defaultvalue = vector->DefaultValue;
		auto ret = overrides.find(vector->ParameterName);
		if (ret != overrides.end())
		{
			defaultvalue = ret->second->ParameterValue;
		}

		if (pin->PinName == "R")
		{
			ss << defaultvalue.R;
		}
		else if (pin->PinName == "G")
		{ 
			ss << defaultvalue.G;
		}
		else if (pin->PinName == "B")
		{
			ss << defaultvalue.B;
		}
		else if (pin->PinName == "A")
		{
			ss << defaultvalue.A;
		}
		else
			ss <<  tostring(defaultvalue) ;
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

	mExprs["MaterialExpressionSubtract"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto sub = Cast<UMaterialExpressionSubtract>(expr);

		if (inputs[0]->LinkedTo.Num() == 0)
		{
			ss << sub->ConstA;
		}
		else
		{
			parse(inputs[0]->LinkedTo[0], ss);
		}

		ss << " - ";

		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << sub->ConstB;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}

	};

	mExprs["MaterialExpressionDivide"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto div = Cast<UMaterialExpressionDivide>(expr);

		if (inputs[0]->LinkedTo.Num() == 0)
		{
			ss << div->ConstA;
		}
		else
		{
			parse(inputs[0]->LinkedTo[0], ss);
		}

		ss << " / ";

		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << div->ConstB;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}

	};

	

	mExprs["MaterialExpressionScalarParameter"] = [&overrides = mOverrideScalarParameters](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto scalar = Cast<UMaterialExpressionScalarParameter>(expr);
		auto defaultvalue = scalar->DefaultValue;
		auto ret = overrides.find(scalar->ParameterName);
		if (ret != overrides.end())
		{
			defaultvalue = ret->second->ParameterValue;
		}
		ss << defaultvalue;
	};

	mExprs["MaterialExpressionTextureSample"] = [&, &res = mBoundResources, &def = mDefinations](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto sampler = Cast<UMaterialExpressionTextureSample>(expr);
		const auto& name = sampler->GetName();
		const auto& texture = sampler->Texture->GetName();

		auto toVariable = [](const std::string & str)
		{
			std::regex r("[^0-9a-zA-Z_]");
			return "_" + std::regex_replace(str, r, "_");
		};

		// bind resource
		if (res.find(texture) == res.end())
		{
			res[texture] = "Texture2D " + toVariable(convertToMulti(*texture));
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

			def[name] = "half4 " + convertToMulti(*name) + " = " + toVariable(convertToMulti(*texture)) + ".Sample(anisotropicSampler," + uv + ")";
		}

		ss << convertToMulti(*name);

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
	mExprs["MaterialExpressionClamp"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto clamp = Cast<UMaterialExpressionClamp>(expr);
		ss << "clamp(";
		parse(inputs[0]->LinkedTo[0], ss);
		ss << ", ";
		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << clamp->MinDefault;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}
		ss << ", ";
		if (inputs[2]->LinkedTo.Num() == 0)
		{
			ss << clamp->MinDefault;
		}
		else
		{
			parse(inputs[2]->LinkedTo[0], ss);
		}

		ss << ")";
	};
	mExprs["MaterialExpressionDivide"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto div = Cast<UMaterialExpressionDivide>(expr);
		if (inputs[0]->LinkedTo.Num() == 0)
		{
			ss << div->ConstA;
		}
		else
		{
			parse(inputs[0]->LinkedTo[0], ss);
		}
		ss << "/";
		if (inputs[1]->LinkedTo.Num() == 0)
		{
			ss << div->ConstB;
		}
		else
		{
			parse(inputs[1]->LinkedTo[0], ss);
		}

	};
	mExprs["MaterialExpressionMaterialFunctionCall"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream&  ss)
	{
		auto fc = Cast<UMaterialExpressionMaterialFunctionCall>(expr);
		Assert(0, "no impl");
	};
	mExprs["MaterialExpressionVertexColor"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto vc = Cast<UMaterialExpressionVertexColor>(expr);
		ss << "input.color" ;

		if (pin->PinName == "Output2")
		{
			ss << ".r";
		}
		else if (pin->PinName == "Output3")
		{
			ss << ".g";
		}
		else if (pin->PinName == "Output4")
		{
			ss << ".b";
		}
		else if (pin->PinName == "Output5")
		{
			ss << ".a";
		}
	};
	mExprs["MaterialExpressionPanner"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto panner = Cast<UMaterialExpressionPanner>(expr);
		
		std::string uv = "input.uv";
		if (inputs[0]->LinkedTo.Num() != 0)
		{
			std::stringstream uvparser;
			parse(inputs[0]->LinkedTo[0], uvparser);
			uv = uvparser.str();
		}

		std::string time = "deltatime";
		if (inputs[1]->LinkedTo.Num() != 0)
		{
			std::stringstream timeparser;
			parse(inputs[1]->LinkedTo[0], timeparser);
			time = timeparser.str();
		}

		std::string speed = format("half2(", panner->SpeedX, ",",panner->SpeedY,")");
		if (inputs[2]->LinkedTo.Num() != 0)
		{
			std::stringstream parser;
			parse(inputs[2]->LinkedTo[0], parser);
			speed = parser.str();
		}

		ss << format(uv, " + ", speed, " * ", time);

	};

	mExprs["MaterialExpressionComponentMask"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto mask = Cast<UMaterialExpressionComponentMask>(expr);

		parse(inputs[0]->LinkedTo[0], ss);

		ss << ".";

		if (mask->R)
			ss << "r";
		if (mask->G)
			ss << "g";
		if (mask->B)
			ss << "b";
		if (mask->A)
			ss << "a";
	};
	mExprs["MaterialExpressionOneMinus"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto om = Cast<UMaterialExpressionOneMinus>(expr);
		ss << "1 - ";
		parse(inputs[0]->LinkedTo[0], ss);

	};

	mExprs["MaterialExpressionPower"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto power = Cast<UMaterialExpressionPower>(expr);
		ss << "power(";
		parse(inputs[0]->LinkedTo[0], ss);

		ss << ",";

		if (inputs[1]->LinkedTo.Num() > 0)
			parse(inputs[1]->LinkedTo[0], ss);
		else
			ss << power->ConstExponent;
		
		ss << ")";
	};
	mExprs["MaterialExpressionSphereMask"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto sm = Cast<UMaterialExpressionSphereMask>(expr);
		
		ss << "power(clamp(";
		
		std::string checkpoint;
		{
			std::stringstream cp;
			parse(inputs[0]->LinkedTo[0], cp);
			checkpoint = cp.str();
		}

		std::string origin;
		{
			std::stringstream op;
			parse(inputs[1]->LinkedTo[0], op);
			origin = op.str();
		}

		std::string radius = format(sm->AttenuationRadius);
		if (inputs[2]->LinkedTo.Num() > 0)
		{
			std::stringstream rp;
			parse(inputs[2]->LinkedTo[0], rp);
			radius = rp.str();
		}

		std::string hardness = format(sm->HardnessPercent * 100.0f + 1.0f);
		if (inputs[3]->LinkedTo.Num() > 0)
		{
			std::stringstream hp;
			parse(inputs[3]->LinkedTo[0], hp);
			hardness = hp.str();
		}
		ss << format("length(", checkpoint, " - ", origin,")/", radius) << ",0,1), " << hardness << ")";

	};
	mExprs["MaterialExpressionNormalize"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto n = Cast<UMaterialExpressionNormalize>(expr);

		ss << "normalize(";
		parse(inputs[0]->LinkedTo[0], ss);
		ss << ")";
	};
	mExprs["MaterialExpressionCrossProduct"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto c = Cast<UMaterialExpressionCrossProduct>(expr);

		ss << "cross(";
		parse(inputs[0]->LinkedTo[0], ss);
		ss <<",";
		parse(inputs[1]->LinkedTo[0], ss);
		ss << ")";
	};

	mExprs["MaterialExpressionDotProduct"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto d = Cast<UMaterialExpressionDotProduct>(expr);

		ss << "dot(";
		parse(inputs[0]->LinkedTo[0], ss);
		ss << ",";
		parse(inputs[1]->LinkedTo[0], ss);
		ss << ")";
	};
	mExprs["MaterialExpressionCameraVectorWS"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	{
		auto d = Cast<UMaterialExpressionCameraVectorWS>(expr);

		ss << "V";
	};
	//mExprs["MaterialExpressionObjectPositionWS"] = [&](const TArray<UEdGraphPin*>& inputs, const TArray<UEdGraphPin*>& outputs, UMaterialExpression* expr, UEdGraphPin* pin, std::stringstream& ss)
	//{
	//	auto d = Cast<UMaterialExpressionObjectPositionWS>(expr);

	//	ss << "boundscenter";
	//};
	
}

std::string MaterialParser::operator()(UMaterialInterface* material)
{
	mBoundResources.clear();
	mDefinations.clear();
	mOverrideVectorParameters.clear();
	mOverrideScalarParameters.clear();
	mMacros.clear();
	auto instance = Cast<UMaterialInstance>(material);
	if (instance)
	{
		for (auto& vp : instance->VectorParameterValues)
		{
			mOverrideVectorParameters[vp.ParameterName_DEPRECATED] = &vp;
		}

		for (auto& vp : instance->ScalarParameterValues)
		{
			mOverrideScalarParameters[vp.ParameterName_DEPRECATED] = &vp;
		}
	}

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

	struct Requirement
	{
		bool exist = false;
		std::string type ;
		std::string value;
	};
	std::map<FString, Requirement> required;
	
	required["Roughness"] = {false, "half", format(base->Roughness.Constant)};
	required["Metallic"] = { false, "half", format(base->Metallic.Constant) };
	required["Emissive Color"] = { false, "half3", "0.0f" };


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
			FString trailer ;
			switch (type)
			{
			case MCT_Float:
			case MCT_Float1: var += "half ";break;
			case MCT_Float2: var += "half2 "; trailer = ".rg"; break;
			case MCT_Float3: var += "half3 "; trailer = ".rgb"; break;
			case MCT_Float4: var += "half4 "; trailer = ".rgba"; break;
			default: 
				Assert(false, "unsupported type");
			}

			auto name = graph->MaterialInputs[Index].GetName().ToString();
			var += name.Replace(L" ", L"_");

			if (required.find(name) != required.end())
			{
				required[name].exist = true;
			}

			ss << convertToMulti(*var) << " = ";
			parse(InputPins[Index]->LinkedTo[0], ss);
			ss <<  ";\n";
		}
	}

	for (auto& r : required)
	{
		if (!r.second.exist)
		{
			ss << "	" << r.second.type << " " << convertToMulti(*r.first.Replace(L" ", L"_")) << " = " << r.second.value << ";\n";
		}
	}

	std::string shader;
	//shader += "#ifndef __SHADER_CONTENT__\n";
	//shader += "#error need shader content\n";
	//shader += "#endif\n";
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

	shader += "__BOUND_RESOURCE__  \n";

	shader += "sampler pointSampler:register(s0);\n";
	shader += "sampler linearSampler:register(s1);\n";
	shader += "sampler linearClamp:register(s2);\n";
	shader += "sampler anisotropicSampler:register(s3);\n";

	shader += "half4 ps(PSInput input):SV_TARGET \n{\n";

	for(auto& d: mDefinations)
		shader += "	" + d.second + ";\n";

	shader+= ss.str();

	shader += "	half3 V = normalize(campos.xyz - input.worldPos.xyz);\n";

	shader += "#ifdef HAS_NORMALMAP\n";
	shader += "	half3 _normal = calNormal(Normal.xyz, input.normal.xyz, input.tangent.xyz, input.binormal.xyz);\n";
	shader += "#else\n";
	shader += "	half3 _normal = input.normal.xyz;\n";
	shader += "#endif\n";
	//shader += "	half3 _final = directBRDF(Roughness, Metallic, F0_DEFAULT, Base_Color.rgb, _normal.xyz,-sundir, campos - input.worldPos);\n";
	//shader += "	return half4(_final ,1) * suncolor;\n";
	//shader += "	return half4(_normal * 0.5 + 0.5,1);";

	shader += "	__SHADER_CONTENT__";
	shader += "\n\n}\n\n";
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
		Assert(false, "cannot parse expression: " + (convertToMulti(*name.ToString())));

}
