#include "StaticMesh.h"

StaticMesh::StaticMesh(FStaticMeshRenderData & renderdata)
{
	//auto renderer = Renderer::getSingleton();

	//auto& mesh = renderdata.LODResources[0];
	//auto numVertices = mesh.GetNumVertices();
	//auto& positions = mesh.VertexBuffers.PositionVertexBuffer;
	//auto& vertices = mesh.VertexBuffers.StaticMeshVertexBuffer;

	//auto stride = (sizeof(FVector) + sizeof(FVector2D));
	//std::vector<char> cacheData(stride * numVertices );
	//char* data = cacheData.data();
	//for (auto i = 0; i < numVertices; ++i)
	//{
	//	const auto& pos = positions.VertexPosition(i);
	//	const auto& uv = vertices.GetVertexUV(i,0);
	//	
	//	memcpy(data, &pos, sizeof(FVector));
	//	memcpy(data + sizeof(FVector), &uv, sizeof(FVector2D));
	//	data += stride;
	//}

	//mVertices = renderer->createBuffer(cacheData.size(), stride, D3D12_HEAP_TYPE_DEFAULT, cacheData.data(), cacheData.size());
	//
	//auto& indices = mesh.IndexBuffer;
	//auto numIndices = indices.GetNumIndices();
	//cacheData.resize(indices.GetIndexDataSize());
	//stride = indices.Is32Bit()? 4: 2;
	//data = cacheData.data();
	//for (auto i = 0; i < numIndices; ++i)
	//{
	//	auto index = indices.GetIndex(i);
	//	memcpy(data, &index, stride);
	//	data += stride;
	//}
	//
	//mIndices = renderer->createBuffer(cacheData.size(), stride,D3D12_HEAP_TYPE_DEFAULT,cacheData.data(), cacheData.size());

}
