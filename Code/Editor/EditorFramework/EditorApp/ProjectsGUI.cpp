#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

void ezQtEditorApp::GuiOpenDashboard()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDashboard", Qt::ConnectionType::QueuedConnection);
}

void ezQtEditorApp::GuiOpenDocsAndCommunity()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiOpenDocsAndCommunity", Qt::ConnectionType::QueuedConnection);
}

bool ezQtEditorApp::GuiCreateProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(true);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, true));
    return true;
  }
}

bool ezQtEditorApp::GuiOpenProject(bool bImmediate /*= false*/)
{
  if (bImmediate)
  {
    return GuiCreateOrOpenProject(false);
  }
  else
  {
    QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, false));
    return true;
  }
}

void ezQtEditorApp::SlotQueuedGuiOpenDashboard()
{
  ezQtDashboardDlg dlg(nullptr, ezQtDashboardDlg::DashboardTab::Projects);
  dlg.exec();
}

void ezQtEditorApp::SlotQueuedGuiOpenDocsAndCommunity()
{
  ezQtDashboardDlg dlg(nullptr, ezQtDashboardDlg::DashboardTab::Documentation);
  dlg.exec();
}

void ezQtEditorApp::SlotQueuedGuiCreateOrOpenProject(bool bCreate)
{
  GuiCreateOrOpenProject(bCreate);
}

bool ezQtEditorApp::GuiCreateOrOpenProject(bool bCreate)
{
  const QString sDir = QString::fromUtf8(m_sLastProjectFolder.GetData());
  ezStringBuilder sFile;

  const char* szFilter = "ezProject (ezProject)";

  if (bCreate)
    sFile = QFileDialog::getExistingDirectory(
      QApplication::activeWindow(), QLatin1String("Choose Folder for New Project"), sDir, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();
  else
    sFile = QFileDialog::getOpenFileName(
      QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter), nullptr, QFileDialog::Option::DontResolveSymlinks)
              .toUtf8()
              .data();

  if (sFile.IsEmpty())
    return false;

  if (bCreate)
    sFile.AppendPath("ezProject");

  m_sLastProjectFolder = ezPathUtils::GetFileDirectory(sFile);

  return CreateOrOpenProject(bCreate, sFile).Succeeded();
}
