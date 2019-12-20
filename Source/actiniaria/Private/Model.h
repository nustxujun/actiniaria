#pragma once

#include "StaticMesh.h"

class Model
{
public:
	using Ptr = std::shared_ptr<Model>;

	void setMesh(StaticMesh::Ptr m) {mMesh = m;}
	void setTramsform(const FMatrix& m){mTransform = m;}
	void setCBuffer(const Renderer::ConstantBuffer::Ptr& cb){mConstants = cb;}

	const StaticMesh::Ptr& getMesh()const{return mMesh;}
	const FMatrix& getTransform()const{return mTransform;}
	const Renderer::ConstantBuffer::Ptr& getCBuffer()const{return mConstants;}
private:
	FMatrix mTransform;
	StaticMesh::Ptr mMesh;
	Renderer::ConstantBuffer::Ptr mConstants;
};