#pragma once

#include "StaticMesh.h"

class Model
{
public:
	using Ptr = std::shared_ptr<Model>;

	void setMesh(StaticMesh::Ptr m) {mMesh = m;}
	void setTramsform(const FMatrix& m){mTransform = m;}

	const StaticMesh::Ptr& getMesh()const{return mMesh;}
	const FMatrix& getTransform()const{return mTransform;}
private:
	FMatrix mTransform;
	StaticMesh::Ptr mMesh;
};