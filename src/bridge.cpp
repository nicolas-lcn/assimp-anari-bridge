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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool loadTexture(const aiScene* scene, ANARIDevice device, const aiMaterial* aiMaterial, const aiTextureType type, const unsigned int index, ANARISampler sampler)
{
  aiString path;

  if(aiMaterial->GetTexture(type, index, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
  {
    int imageWidth, imageHeight, imageBPP;
    imageWidth = imageHeight = imageBPP = 0;
    const aiTexture* aiTexture = scene->GetEmbeddedTexture(path.C_Str());
    if(aiTexture)
    {
      stbi_set_flip_vertically_on_load(1);

      void* pImageData = stbi_load_from_memory((const stbi_uc*)aiTexture->pcData, aiTexture->mWidth, &imageWidth, &imageHeight, &imageBPP, 0);
      anariSetParameter(device, sampler, "image", ANARI_ARRAY2D, pImageData);
      anariSetParameter(device, sampler, "inAttribute", ANARI_STRING, "attribute0");
      anariSetParameter(device, sampler, "filter", ANARI_STRING, "linear");
      anariSetParameter(device, sampler, "wrapMode1", ANARI_STRING, "repeat");
      anariSetParameter(device, sampler, "wrapMode2", ANARI_STRING, "repeat");

      anariCommitParameters(device, sampler);
      stbi_image_free(pImageData);
      std::cerr<<"loaded image texture dims : "<<imageWidth<<","<<imageHeight<<" bits per pixel : "<<imageBPP<<std::endl;
      return true;
    }
  }
  return false;
}

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
    }
  }

  if (scene->HasMaterials()) {
    for (unsigned int index = 0; index < scene->mNumMaterials; ++index) {

      // KHR_MATERIAL_PHYSICALLY_BASED or KHR_MATERIAL_MATTE
      const aiMaterial* aiMaterial = scene->mMaterials[index];
      const char* materialType;
      ANARIMaterial material = anariNewMaterial(device, "physicallyBased");

      // base color
      if(aiMaterial->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        // Assuming every textures are embedded in the scene
        if(loadTexture(scene, device, aiMaterial, aiTextureType_BASE_COLOR, 0, sampler))
          anariSetParameter(device, material, "baseColor", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }
      float opacity = 1.0f;
      if (aiMaterial->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
      {
        anariSetParameter(device, material, "opacity", ANARI_FLOAT32, (void*)&opacity);
      }
      // metallic roughness
      // According to ANARI Spec : (https://registry.khronos.org/ANARI/specs/1.0/ANARI-1.0.html)
      // To use the glTF metallicRoughnessTexture create two samplers with 
      // the same image array and a "swizzle" outTransform for roughness.
      /* Refs: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#metallic-roughness-material
       *           "textures for metalness and roughness properties are packed together in a single
       *           texture called metallicRoughnessTexture. Its green channel contains roughness
       *           values and its blue channel contains metalness values..."
       *       https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_material_pbrmetallicroughness_metallicroughnesstexture
       *           "The metalness values are sampled from the B channel. The roughness values are
       *           sampled from the G channel..."
       */
      if(aiMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
      {
        
        ANARISampler metallic = anariNewSampler(device, "image2D");
        // Assuming every textures are embedded in the scene
        if(loadTexture(scene, device, aiMaterial, aiTextureType_DIFFUSE_ROUGHNESS, 0, metallic))
        {
          //According to gltf spec, metallness is encoded in blue channel
          float swizzle[16] = {
                               0.0, 0.0, 1.0, 0.0,
                               0.0, 0.0, 1.0, 0.0,
                               0.0, 0.0, 1.0, 0.0,
                               0.0, 0.0, 1.0, 0.0
                              };
          anariSetParameter(device, metallic, "outTransform", ANARI_FLOAT32_MAT4, swizzle);
        }
        ANARISampler roughness = anariNewSampler(device, "image2D");
        // Assuming every textures are embedded in the scene
        if(loadTexture(scene, device, aiMaterial, aiTextureType_DIFFUSE_ROUGHNESS, 0, roughness))
        {
          //According to gltf spec, roughness is encoded in green channel
          float swizzle[16] = {
                               0.0, 1.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0
                              }; 
          anariSetParameter(device, roughness, "outTransform", ANARI_FLOAT32_MAT4, swizzle);
        }
        aiUVTransform t;
        if(aiMaterial->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_DIFFUSE_ROUGHNESS, 0), t) == AI_SUCCESS)
        {
          aiMatrix3x3 translate = aiMatrix3x3(); 
          aiMatrix3x3::Translation(t.mTranslation, translate);
          aiMatrix3x3 scale = aiMatrix3x3(t.mScaling[0],           0.0, 0.0,
                                                    0.0, t.mScaling[1], 0.0,
                                                    0.0,           0.0, 1.0);
          aiMatrix3x3 rotation  = aiMatrix3x3();
          aiMatrix3x3::RotationZ(t.mRotation, rotation);
          aiMatrix3x3 res =  translate * rotation * scale;
          float transform[16] = { res.a1, res.a2, res.a3, 0.0,
                                  res.b1, res.b2, res.b3, 0.0,
                                  res.c1, res.c2, res.c3, 0.0,
                                     0.0,    0.0,    0.0, 1.0};
          anariSetParameter(device, metallic, "inTransform", ANARI_FLOAT32_MAT4, transform);
          anariSetParameter(device, roughness, "inTransform", ANARI_FLOAT32_MAT4, transform);
        }
        anariCommitParameters(device, metallic);
        anariCommitParameters(device, roughness);
        anariSetParameter(device, material, "metallic", ANARI_SAMPLER, metallic);
        anariSetParameter(device, material, "roughness", ANARI_SAMPLER, roughness);

        anariRelease(device, metallic);
        anariRelease(device, roughness);
      }
      // normal map
      // In GLTF 2.0 Spec, a normal scale is defined, but it's not defined in assimp.
      // See Closed Issue : https://github.com/assimp/assimp/issues/4853
      // assimp/material.h did not kept the commit in the issue.   
      if(aiMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        // Assuming every textures are embedded in the scene
        if(loadTexture(scene, device, aiMaterial, aiTextureType_NORMALS, 0, sampler))
          anariSetParameter(device, material, "normals", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }
      // Emissive
      // ANARI Spec : 
      /*
       *  https://registry.khronos.org/ANARI/specs/1.0/ANARI-1.0.html
       *     glTF parameters emissiveStrength and emissiveFactor should be factored into emissive 
       *     (or outTransform if emissive is an ANARI_SAMPLER).
       *
      */
      // But Assimp does not implement those two factors, only : 
      // AI_MATKEY_EMISSIVE_INTENSITY


      float emissive;
      if(aiMaterial->GetTextureCount(aiTextureType_EMISSIVE) > 0)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        // Assuming every textures are embedded in the scene
        if(loadTexture(scene, device, aiMaterial, aiTextureType_EMISSIVE, 0, sampler))
        {
          if(aiMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissive) == AI_SUCCESS)
          {
              float factor[16] = {
                                 emissive,      0.0,      0.0,      0.0,
                                      0.0, emissive,      0.0,      0.0,
                                      0.0,      0.0, emissive,      0.0,
                                      0.0,      0.0,      0.0, emissive
                                }; 
              anariSetParameter(device, sampler, "outTransform", ANARI_FLOAT32_MAT4, factor);
          }
          aiUVTransform t;
          if(aiMaterial->Get(AI_MATKEY_UVTRANSFORM(aiTextureType_EMISSIVE, 0), t) == AI_SUCCESS)
          {
            aiMatrix3x3 translate = aiMatrix3x3(); 
            aiMatrix3x3::Translation(t.mTranslation, translate);
            aiMatrix3x3 scale = aiMatrix3x3(t.mScaling[0],           0.0, 0.0,
                                                      0.0, t.mScaling[1], 0.0,
                                                      0.0,           0.0, 1.0);
            aiMatrix3x3 rotation  = aiMatrix3x3();
            aiMatrix3x3::RotationZ(t.mRotation, rotation);
            aiMatrix3x3 res =  translate * rotation * scale;
            float transform[16] = { res.a1, res.a2, res.a3, 0.0,
                                    res.b1, res.b2, res.b3, 0.0,
                                    res.c1, res.c2, res.c3, 0.0,
                                       0.0,    0.0,    0.0, 1.0};
            anariSetParameter(device, sampler, "inTransform", ANARI_FLOAT32_MAT4, transform);
          }
          anariCommitParameters(device, sampler);
          anariSetParameter(device, material, "normals", ANARI_SAMPLER, sampler);
        }
        anariRelease(device, sampler);
      }else if(aiMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissive) == AI_SUCCESS)
      {
          anariSetParameter(device, material, "emissive", ANARI_FLOAT32, &emissive);
      }

      // Ambient occlusion

      if(aiMaterial->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        if(loadTexture(scene, device, aiMaterial, aiTextureType_AMBIENT_OCCLUSION, 0, sampler))
          anariSetParameter(device, material, "occlusion", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }

      // Alpha mode.
      /* Since assimp does not implement it as it can be received in ANARI, 
       * work around with blend function and trasnparency factor
      */
      float transparency = 1.0f;
      if (aiMaterial->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparency) == AI_SUCCESS)
      {
        anariSetParameter(device, material, "alphaCutOff", ANARI_FLOAT32, &transparency);
      }
      aiBlendMode alphaMode;
      if (aiMaterial->Get(AI_MATKEY_BLEND_FUNC, alphaMode) == AI_SUCCESS)
      {
        if(alphaMode == aiBlendMode_Default)
          anariSetParameter(device, material, "alphaMode", ANARI_STRING, "mask");
        else if(alphaMode == aiBlendMode_Additive)
          anariSetParameter(device, material, "alphaMode", ANARI_STRING, "blend");
        else
          anariSetParameter(device, material, "alphaMode", ANARI_STRING, "opaque");

      }
      // Specular/Glossiness
      if(aiMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        if(loadTexture(scene, device, aiMaterial, aiTextureType_SPECULAR, 0, sampler))
          anariSetParameter(device, material, "specular", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }
      aiColor4D specularColor(0.0, 0.0, 0.0, 0.0);
      if(aiMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS)
      {
        float color[3] = {specularColor.r, specularColor.g, specularColor.b};
        anariSetParameter(device, material, "specularColor", ANARI_FLOAT32_VEC3, color);
      }

      // CLEAR COAT
      if(aiMaterial->GetTextureCount(aiTextureType_CLEARCOAT) > 0)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        if(loadTexture(scene, device, aiMaterial, aiTextureType_CLEARCOAT, 0, sampler))
          anariSetParameter(device, material, "clearcoat", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }
      float clearcoatRoughnessFactor = 1.0f;
      if(aiMaterial->GetTextureCount(aiTextureType_CLEARCOAT) > 1)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        if(loadTexture(scene, device, aiMaterial, AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE, sampler))
          anariSetParameter(device, material, "clearcoatRoughness", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }else if (aiMaterial->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, clearcoatRoughnessFactor) == AI_SUCCESS)
      {
        anariSetParameter(device, material, "clearcoatRoughness", ANARI_FLOAT32, &clearcoatRoughnessFactor);
      }
      if(aiMaterial->GetTextureCount(aiTextureType_CLEARCOAT) > 2)
      {
        ANARISampler sampler = anariNewSampler(device, "image2D");
        if(loadTexture(scene, device, aiMaterial, AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE, sampler))
          anariSetParameter(device, material, "clearcoatRoughness", ANARI_SAMPLER, sampler);
        anariRelease(device, sampler);
      }
      
      
      anariCommitParameters(device, material);
      materialsByMaterialId[index] = material;
      anariRelease(device, material);
    }
  }

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
