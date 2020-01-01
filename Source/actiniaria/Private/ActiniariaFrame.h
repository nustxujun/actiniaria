#include "Core.h"
#include <sstream>

#include "engine/DefaultFrame.h"

#include <iostream>

inline FString GetPluginPath()
{
	FString path = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("actiniaria"));
	if (FPaths::DirectoryExists(path))
		return path;
	return  FPaths::Combine(FPaths::EnginePluginsDir(), TEXT("actiniaria"));
}

class ActiniariaFrame :public DefaultFrame
{
public:
	ActiniariaFrame()
	{
		FString path = GetPluginPath() + "/Source/actiniaria/Private/engine/";
		Renderer::getSingleton()->addSearchPath(*path);
	}
} ;


