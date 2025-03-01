#pragma once

#include <Core/CoreDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;
class ezUuid;

struct ezSurfaceResourceEvent
{
  enum class Type
  {
    Created,
    Destroyed
  };

  Type m_Type;
  ezSurfaceResource* m_pSurface;
};

class EZ_CORE_DLL ezSurfaceResource : public ezResource
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSurfaceResource, ezResource);
  EZ_RESOURCE_DECLARE_COMMON_CODE(ezSurfaceResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezSurfaceResource, ezSurfaceResourceDescriptor);

public:
  ezSurfaceResource();
  ~ezSurfaceResource();

  const ezSurfaceResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  static ezEvent<const ezSurfaceResourceEvent&, ezMutex> s_Events;

  void* m_pPhysicsMaterialPhysX = nullptr;
  void* m_pPhysicsMaterialJolt = nullptr;

  /// \brief Spawns the prefab that was defined for the given interaction at the given position and using the configured orientation.
  /// Returns false, if the interaction type was not defined in this surface or any of its base surfaces
  bool InteractWithSurface(ezWorld* pWorld, ezGameObjectHandle hObject, const ezVec3& vPosition, const ezVec3& vSurfaceNormal, const ezVec3& vIncomingDirection, const ezTempHashedString& sInteraction, const ezUInt16* pOverrideTeamID, float fImpulseSqr = 0.0f) const;

  bool IsBasedOn(const ezSurfaceResource* pThisOrBaseSurface) const;

  bool IsBasedOn(const ezSurfaceResourceHandle hThisOrBaseSurface) const;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  static const ezSurfaceInteraction* FindInteraction(const ezSurfaceResource* pCurSurf, ezUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue);

  ezSurfaceResourceDescriptor m_Descriptor;

  struct SurfInt
  {
    ezUInt64 m_uiInteractionTypeHash = 0;
    const ezSurfaceInteraction* m_pInteraction;
  };

  ezDynamicArray<SurfInt> m_Interactions;
};
