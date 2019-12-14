#pragma once
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/Renderer.h"
class StaticMesh
{
public:
	using Ptr = std::shared_ptr<StaticMesh>;

	StaticMesh(FStaticMeshRenderData& renderdata);

	const Renderer::Buffer::Ptr& getVertices()const {return mVertices;}
	const Renderer::Buffer::Ptr& getIndices()const{return mIndices;}
private:
	Renderer::Buffer::Ptr mVertices;
	Renderer::Buffer::Ptr mIndices;
};