#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

ezResult ezTexConvProcessor::Assemble3DTexture(ezImage& dst) const
{
  EZ_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return ezImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}


EZ_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture3D);
