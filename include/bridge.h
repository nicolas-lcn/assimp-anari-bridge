#ifndef _ASSIMP_ANARI_BRIDGE_H_DEFINED
#define _ASSIMP_ANARI_BRIDGE_H_DEFINED

// anari-sdk includes
#include <anari/anari.h>

// assimp includes
#include <assimp/scene.h>

namespace assimp_anari_bridge {

  /**
   * Convert a full aiScene from Assimp to an ANARIWorld using a given ANARIDevice
   * @param[in] scene Assimp scene pointer
   * @param[in] device ANARI device handler
   * @return The instance ANARIWorld built for given device
   **/
  ANARIWorld bridge(const aiScene* scene, ANARIDevice device);
  
}


#endif
