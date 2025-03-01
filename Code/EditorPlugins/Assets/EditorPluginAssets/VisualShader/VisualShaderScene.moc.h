#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtNodeView;

class ezQtVisualShaderScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualShaderScene(QObject* pParent = nullptr);
  ~ezQtVisualShaderScene();
};

class ezQtVisualShaderPin : public ezQtPin
{
public:
  ezQtVisualShaderPin();

  virtual void SetPin(const ezPin& pin) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
};

class ezQtVisualShaderNode : public ezQtNode
{
public:
  ezQtVisualShaderNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

