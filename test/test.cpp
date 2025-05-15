// assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
// anari-sdk includes
#include <anari/anari.h>
#include <anari/anari_cpp.hpp>
// std includes
#include <iostream>

// bridge includes
#include "../include/bridge.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./test_bridge <model_path>" << std::endl;
    return 1;
  }

  const char* modelPath = argv[1];

  anari::Device device = anari::newDevice("default");

  if (!device) {
    std::cerr << "Failed to create ANARI device." << std::endl;
    return 1;
  }
  // Set error callback
  anari::setParameter(device, device, "statusCallback", ANARI_STATUS_CALLBACK_FUNCTION, [](void*, ANARIDataType, void*, ANARIStatusSeverity severity, const char* message) {
    std::cerr << "ANARI [" << int(severity) << "]: " << message << std::endl;
  });
  anari::commitParameters(device, device);

  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(modelPath,
       aiProcess_Triangulate |
       aiProcess_GenSmoothNormals |
       aiProcess_JoinIdenticalVertices |
       aiProcess_FlipUVs);

  if (!scene || !scene->HasMeshes()) {
       std::cerr << "Failed to load model: " << importer.GetErrorString() << std::endl;
       return 1;
  }

  ANARIWorld world = assimp_anari_bridge::bridge(scene, device);

  // Commit the world (required before rendering)
  anari::commitParameters(device, world);

  std::cout << "ANARI device and world initialized successfully." << std::endl;

  // Cleanup
  anari::release(device, world);
  anari::release(device, device);

  return 0;
}
