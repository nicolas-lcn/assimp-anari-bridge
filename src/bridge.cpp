#include "bridge.h"


ANARIWorld assimp_anari_bridge::bridge(const aiScene* scene, ANARIDevice device) {
  ANARIWorld world = anariNewWorld(device);

  scene->HasMeshes();

  scene->HasMaterials();

  scene->HasCameras();

  anariCommitParameters(device, world);
  return world;
}
