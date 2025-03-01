#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCleanGraph;
};

class ezVisualShaderAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualShaderAction, ezButtonAction);

public:
  ezVisualShaderAction(const ezActionContext& context, const char* szName);
  ~ezVisualShaderAction();

  virtual void Execute(const ezVariant& value) override;
};
