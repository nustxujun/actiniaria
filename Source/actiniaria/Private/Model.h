#pragma once

#include "StaticMesh.h"
#include "Material.h"

class Model
{
public:
	using Ptr = std::shared_ptr<Model>;

	void setMesh(StaticMesh::Ptr m) {mMesh = m;}
	void setTramsform(const FMatrix& m){mTransform = m;}
	void setVSCBuffer(const Renderer::ConstantBuffer::Ptr& cb){ mVSConstants = cb;}
	void setPSCBuffer(const Renderer::ConstantBuffer::Ptr& cb) { mPSConstants = cb; }
	void setMaterial(const Material::Ptr& mat){ mMaterial = mat;}

	const StaticMesh::Ptr& getMesh()const{return mMesh;}
	const FMatrix& getTransform()const{return mTransform;}
	const Renderer::ConstantBuffer::Ptr& getVSCBuffer()const{return mVSConstants;}
	const Renderer::ConstantBuffer::Ptr& getPSCBuffer()const { return mPSConstants; }
	const Material::Ptr& getMaterial()const{return mMaterial;}
private:
	FMatrix mTransform;
	StaticMesh::Ptr mMesh;
	Material::Ptr mMaterial;
	Renderer::ConstantBuffer::Ptr mVSConstants;
	Renderer::ConstantBuffer::Ptr mPSConstants;

};