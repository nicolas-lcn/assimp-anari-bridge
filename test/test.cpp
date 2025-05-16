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

static void statusFunc(const void *userData,
    ANARIDevice device,
    ANARIObject source,
    ANARIDataType sourceType,
    ANARIStatusSeverity severity,
    ANARIStatusCode code,
    const char *message)
{
  const bool verbose = userData ? *(const bool *)userData : false;
  if (severity == ANARI_SEVERITY_FATAL_ERROR) {
    fprintf(stderr, "[FATAL][%p] %s\n", source, message);
    std::exit(1);
  } else if (severity == ANARI_SEVERITY_ERROR) {
    fprintf(stderr, "[ERROR][%p] %s\n", source, message);
  } else if (severity == ANARI_SEVERITY_WARNING) {
    fprintf(stderr, "[WARN ][%p] %s\n", source, message);
  } else if (verbose && severity == ANARI_SEVERITY_PERFORMANCE_WARNING) {
    fprintf(stderr, "[PERF ][%p] %s\n", source, message);
  } else if (verbose && severity == ANARI_SEVERITY_INFO) {
    fprintf(stderr, "[INFO ][%p] %s\n", source, message);
  } else if (verbose && severity == ANARI_SEVERITY_DEBUG) {
    fprintf(stderr, "[DEBUG][%p] %s\n", source, message);
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: ./test_bridge <model_path>" << std::endl;
    return 1;
  }

  const char* modelPath = argv[1];
  bool verbose = false;
  anari::Library library = anariLoadLibrary("helide", statusFunc, &verbose);

  if (!library) {
    std::cerr << "Failed to load ANARI library" << std::endl;
    return 1;
  }
  anari::Device device = anari::newDevice(library, "default");

  if (!device) {
    std::cerr << "Failed to create ANARI device." << std::endl;
    return 1;
  }
  std::cerr << "anari device is built (closing library)" << std::endl;
  anari::unloadLibrary(library);

  // Set error callback
  // anari::setParameter(device, device, "statusCallback", ANARI_STATUS_CALLBACK_FUNCTION, [](void*, ANARIDataType, void*, ANARIStatusSeverity severity, const char* message) {
  //   std::cerr << "ANARI [" << int(severity) << "]: " << message << std::endl;
  // });
  // anari::commitParameters(device, device);

  Assimp::Importer importer;

  std::cerr << "Start importing model: " << modelPath << std::endl;
  const aiScene* scene = importer.ReadFile(modelPath,
       aiProcess_Triangulate |
       aiProcess_GenSmoothNormals |
       aiProcess_JoinIdenticalVertices |
       aiProcess_FlipUVs);
  std::cerr << "Post import" << std::endl;
  if (!scene || !scene->HasMeshes()) {
       std::cerr << "Failed to load model: " << importer.GetErrorString() << std::endl;
       return 1;
  }

  std::cerr << "Run AssimpXAnari bridge" << std::endl;
  ANARIWorld world = assimp_anari_bridge::bridge(scene, device);
  if (!world) {
    std::cerr << "Failed to build Anari world" << std::endl;
    return 1;
  }
  // Commit the world (required before rendering)
  anari::commitParameters(device, world);

  std::cout << "ANARI device and world initialized successfully." << std::endl;

  // Cleanup
  anari::release(device, world);
  anari::release(device, device);

  return 0;
}
