#pragma once
#include "Engine/Renderer.h"
#include "Materials/MaterialInterface.h"
#include <sstream>

class MaterialParser
{
public:
	MaterialParser();
	std::string operator()(UMaterialInterface* material);

private:
	void parse(UEdGraphPin* pin, std::stringstream& ss);

	std::map<FString, std::function<void(const TArray<UEdGraphPin*>&, const TArray<UEdGraphPin*>&, UMaterialExpression*, UEdGraphPin* pin,std::stringstream& )>> mExprs;
	std::map<FString, std::string> mBoundResources;
	std::map<FString,std::string> mDefinations;
	std::map<FString, std::string> mMacros;
};