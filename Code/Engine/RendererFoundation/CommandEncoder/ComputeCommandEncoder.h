
#pragma once

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class EZ_RENDERERFOUNDATION_DLL ezGALComputeCommandEncoder : public ezGALCommandEncoder
{
public:
  ezGALComputeCommandEncoder(ezGALDevice& ref_device, ezGALCommandEncoderState& ref_state, ezGALCommandEncoderCommonPlatformInterface& ref_commonImpl, ezGALCommandEncoderComputePlatformInterface& ref_computeImpl);
  virtual ~ezGALComputeCommandEncoder();

  // Dispatch

  void Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ);
  void DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDispatchCall() { m_uiDispatchCalls++; }

  // Statistic variables
  ezUInt32 m_uiDispatchCalls = 0;

  ezGALCommandEncoderComputePlatformInterface& m_ComputeImpl;
};
