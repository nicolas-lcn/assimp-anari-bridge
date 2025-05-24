#include "bridge.h"

#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/material.h>
#include <assimp/pbrmaterial.h>

#include <limits>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
// Use C++99
ANARIWorld assimp_anari_bridge::bridge(const aiScene* scene, ANARIDevice device) {
  // check if device supports quad, triangle (KHR_GEOMETRY_QUAD, KHR_GEOMETRY_TRIANGLE)
  ANARIWorld world = anariNewWorld(device);

  std::map<unsigned int, ANARIGeometry> geometriesByMeshId;
  std::map<unsigned int, ANARIMaterial> materialsByMaterialId;
  std::map<unsigned int, ANARISurface> surfacesByMeshId;
  std::map<unsigned int, ANARIInstance> instancesByNodeId;
  std::map<unsigned int, ANARIGroup> groupsByNodeId;

  // Limits
  uint64_t geometryMaxIndex = 0;

  std::cerr << "getProperty = " << geometryMaxIndex << std::endl;
  if (anariGetProperty(device, device, "geometryMaxIndex", ANARI_UINT64, &geometryMaxIndex, sizeof(uint64_t), ANARI_WAIT)) {
    std::cerr << "geometryMaxIndex = " << geometryMaxIndex << std::endl;
  } else {
    geometryMaxIndex = std::numeric_limits<uint32_t>::max();
    std::cerr << "geometryMaxIndex (not returned) = " << geometryMaxIndex << std::endl;
  }

  if (scene->HasMaterials()) {
    for (unsigned int index = 0; index < scene->mNumMaterials; ++index) {

      // KHR_MATERIAL_PHYSICALLY_BASED or KHR_MATERIAL_MATTE
      const aiMaterial* aiMaterial = scene->mMaterials[index];
      const char* materialType;
      ANARIMaterial material = anariNewMaterial(device, "physicallyBased");

      materialsByMaterialId[index] = material;
    }
  }

  if (scene->HasMeshes()) {
    for (size_t index = 0; index < scene->mNumMeshes; ++index) {
      std::cerr << "mesh = " << (index + 1) << "/" << scene->mNumMeshes << std::endl;

      const aiMesh* mesh = scene->mMeshes[index];
      mesh->mPrimitiveTypes;//aiPrimitiveType
      if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
        // We ignore mesh that are not triangle at the moment
        continue;
      }

      if (mesh->mFaces != nullptr && mesh->mNumVertices > geometryMaxIndex) {
        // We ignore mesh with a number of vertices superior to device limit if we have index-based mesh
        continue;
      }

      std::cerr << "create geometry associated with mesh = " << index << std::endl;

      ANARIGeometry geometry = anariNewGeometry(device, "triangle");

      ANARIArray1D array;

      std::cerr << "create vertices = " << mesh->mNumVertices << std::endl;
      array = anariNewArray1D(device, (float*)mesh->mVertices, 0, 0, ANARI_FLOAT32_VEC3, mesh->mNumVertices);
      anariCommitParameters(device, array);
      anariSetParameter(device, geometry, "vertex.position", ANARI_ARRAY1D, &array);
      anariRelease(device, array); // we are done using this handle

      if (mesh->mNormals) {
        std::cerr << "create normals = " << mesh->mNumVertices << std::endl;
        array = anariNewArray1D(device, (float*)mesh->mNormals, 0, 0, ANARI_FLOAT32_VEC3, mesh->mNumVertices);
        anariCommitParameters(device, array);
        anariSetParameter(device, geometry, "vertex.normal", ANARI_ARRAY1D, &array);
        anariRelease(device, array); // we are done using this handle
      }

      if (mesh->mTangents) {
        std::cerr << "create tangents = " << mesh->mNumVertices << std::endl;
        array = anariNewArray1D(device, (float*)mesh->mTangents, 0, 0, ANARI_FLOAT32_VEC3, mesh->mNumVertices);
        anariCommitParameters(device, array);
        anariSetParameter(device, geometry, "vertex.tangent", ANARI_ARRAY1D, &array);
        anariRelease(device, array); // we are done using this handle
      }

      // TODO should be given in selected attribute / deactivate / or handeness pushed in tangents
      if (mesh->mBitangents) {
        std::cerr << "create bitangents = " << mesh->mNumVertices << std::endl;
        array = anariNewArray1D(device, (float*)mesh->mBitangents, 0, 0, ANARI_FLOAT32_VEC3, mesh->mNumVertices);
        anariCommitParameters(device, array);
        anariSetParameter(device, geometry, "vertex.attribute0", ANARI_ARRAY1D, &array);
        anariRelease(device, array); // we are done using this handle
      }

      // TODO should be given in selected attribute / deactivate /
      if (mesh->mColors[0]) {
        std::cerr << "create colors  = " << mesh->mNumVertices << std::endl;
        array = anariNewArray1D(device, (float*)mesh->mColors[0], 0, 0, ANARI_FLOAT32_VEC4, mesh->mNumVertices);
        anariCommitParameters(device, array);
        anariSetParameter(device, geometry, "vertex.color", ANARI_ARRAY1D, &array);
        anariRelease(device, array); // we are done using this handle
      }

      int addedUvs = 0;
      for (unsigned int indexUV = 0; indexUV < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++indexUV) {
        if (mesh->mTextureCoords[indexUV] == nullptr) {
          continue;
        }
        std::cerr << "create uvs  = " << indexUV << std::endl;
        if (addedUvs >= 3) {
          break;
        }
        std::vector<float> uvs(mesh->mNumVertices * 2);
        for (unsigned int indexVertex = 0; indexVertex < mesh->mNumVertices; ++indexVertex) {
          uvs[2 * indexVertex]     = mesh->mTextureCoords[indexUV][indexVertex][0];
          uvs[2 * indexVertex + 1] = mesh->mTextureCoords[indexUV][indexVertex][1];
        }
        std::stringstream builder;
        builder << "vertex.attribute" << (1 + addedUvs);
        std::string text = builder.str();
        std::cerr << "create uvs  = " << indexUV << " in " << text << std::endl;
        array = anariNewArray1D(device, uvs.data(), 0, 0, ANARI_FLOAT32_VEC2, mesh->mNumVertices);
        anariCommitParameters(device, array);
        anariSetParameter(device, geometry, text.c_str(), ANARI_ARRAY1D, &array);
        anariRelease(device, array); // we are done using this handle
        addedUvs++;
      }

      mesh->mColors;//aiColor4D*[AI_MAX_NUMBER_OF_COLOR_SETS]|nullptr
      mesh->mTextureCoords;//aiVector3D*[AI_MAX_NUMBER_OF_TEXTURECOORDS]|nullptr
      mesh->mTextureCoordsNames;//aiString**
      mesh->mNumUVComponents;//uint[AI_MAX_NUMBER_OF_TEXTURECOORDS]

      if (mesh->mFaces) {
        std::vector<uint32_t> faces(mesh->mNumFaces * 3);
        std::cerr << "create faces  = " << mesh->mNumFaces << std::endl;
        for (unsigned int indexFace = 0; indexFace < mesh->mNumFaces; ++indexFace) {
          faces[3 * indexFace]     = mesh->mFaces[indexFace].mIndices[0];
          faces[3 * indexFace + 1] = mesh->mFaces[indexFace].mIndices[1];
          faces[3 * indexFace + 2] = mesh->mFaces[indexFace].mIndices[2];
        }
        array = anariNewArray1D(device, faces.data(), 0, 0, ANARI_UINT32_VEC3, mesh->mNumFaces);
        anariCommitParameters(device, array);
        anariSetParameter(device, geometry, "primitive.index", ANARI_ARRAY1D, &array);
        anariRelease(device, array); // we are done using this handle
      }
      std::cerr<< "after all" << std::endl;

      mesh->mName;//aiString

      mesh->mMaterialIndex;//uint

      mesh->mBones;//aiBone**

      mesh->mNumBones;//uint

      mesh->mNumAnimMeshes;//uint

      mesh->mAnimMeshes;//aiAnimMesh** mAnimMeshes;

      mesh->mMethod;//aiMorphingMethod associated to aiANimMesh

      anariCommitParameters(device, geometry);
      geometriesByMeshId[index] = geometry;
      // TODO map geometry and mesh index
    }
  }

  scene->HasMaterials();

  scene->HasCameras();


  anariCommitParameters(device, world);

  for (auto& pair: geometriesByMeshId) {
    anariRelease(device, pair.second);
  }

  for (auto& pair: materialsByMaterialId) {
    anariRelease(device, pair.second);
  }

  return world;
}
