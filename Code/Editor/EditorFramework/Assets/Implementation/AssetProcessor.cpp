#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

EZ_IMPLEMENT_SINGLETON(ezAssetProcessor);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetProcessor)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "AssetCurator"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezAssetProcessor);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezAssetProcessor* pDummy = ezAssetProcessor::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezCuratorLog
////////////////////////////////////////////////////////////////////////

void ezAssetProcessorLog::HandleLogMessage(const ezLoggingEventData& le)
{
  m_LoggingEvent.Broadcast(le);
}

void ezAssetProcessorLog::AddLogWriter(ezLoggingEvent::Handler handler)
{
  m_LoggingEvent.AddEventHandler(handler);
}

void ezAssetProcessorLog::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  m_LoggingEvent.RemoveEventHandler(handler);
}


////////////////////////////////////////////////////////////////////////
// ezAssetProcessor
////////////////////////////////////////////////////////////////////////

ezAssetProcessor::ezAssetProcessor()
  : m_SingletonRegistrar(this)
{
}


ezAssetProcessor::~ezAssetProcessor()
{
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }
  EZ_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Call StopProcessTask first before destroying the ezAssetProcessor.");
}

void ezAssetProcessor::StartProcessTask()
{
  EZ_LOCK(m_ProcessorMutex);
  if (m_ProcessTaskState != ProcessTaskState::Stopped)
  {
    return;
  }

  // Join old thread.
  if (m_pThread)
  {
    m_pThread->Join();
    m_pThread.Clear();
  }

  m_ProcessTaskState = ProcessTaskState::Running;

  const ezUInt32 uiWorkerCount = ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks);
  m_ProcessRunning.SetCount(uiWorkerCount, false);
  m_ProcessTasks.SetCount(uiWorkerCount);

  for (ezUInt32 idx = 0; idx < uiWorkerCount; ++idx)
  {
    m_ProcessTasks[idx].m_uiProcessorID = idx;
  }

  m_pThread = EZ_DEFAULT_NEW(ezProcessThread);
  m_pThread->Start();

  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}

void ezAssetProcessor::StopProcessTask(bool bForce)
{
  {
    EZ_LOCK(m_ProcessorMutex);
    switch (m_ProcessTaskState)
    {
      case ProcessTaskState::Running:
      {
        m_ProcessTaskState = ProcessTaskState::Stopping;
        {
          ezAssetProcessorEvent e;
          e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
          m_Events.Broadcast(e);
        }
      }
      break;
      case ProcessTaskState::Stopping:
        if (!bForce)
          return;
        break;
      default:
      case ProcessTaskState::Stopped:
        return;
    }
  }

  if (bForce)
  {
    m_bForceStop = true;
    m_pThread->Join();
    m_pThread.Clear();
    EZ_ASSERT_DEV(m_ProcessTaskState == ProcessTaskState::Stopped, "Process task shoul have set the state to stopped.");
  }
}

void ezAssetProcessor::AddLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.AddLogWriter(handler);
}

void ezAssetProcessor::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  m_CuratorLog.RemoveLogWriter(handler);
}

void ezAssetProcessor::Run()
{
  while (m_ProcessTaskState == ProcessTaskState::Running)
  {
    for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_ProcessRunning[i])
      {
        m_ProcessRunning[i] = !m_ProcessTasks[i].FinishExecute();
      }
      else
      {
        m_ProcessRunning[i] = m_ProcessTasks[i].BeginExecute();
      }
    }
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
  }

  while (true)
  {
    bool bAnyRunning = false;

    for (ezUInt32 i = 0; i < m_ProcessTasks.GetCount(); i++)
    {
      if (m_ProcessRunning[i])
      {
        if (m_bForceStop)
          m_ProcessTasks[i].ShutdownProcess();

        m_ProcessRunning[i] = !m_ProcessTasks[i].FinishExecute();
        bAnyRunning |= m_ProcessRunning[i];
      }
    }

    if (bAnyRunning)
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    else
      break;
  }

  EZ_LOCK(m_ProcessorMutex);
  m_ProcessRunning.Clear();
  m_ProcessTasks.Clear();
  m_ProcessTaskState = ProcessTaskState::Stopped;
  m_bForceStop = false;
  {
    ezAssetProcessorEvent e;
    e.m_Type = ezAssetProcessorEvent::Type::ProcessTaskStateChanged;
    m_Events.Broadcast(e);
  }
}


////////////////////////////////////////////////////////////////////////
// ezProcessTask
////////////////////////////////////////////////////////////////////////

ezProcessTask::ezProcessTask()
  : m_Status(EZ_SUCCESS)
{
  m_pIPC = EZ_DEFAULT_NEW(ezEditorProcessCommunicationChannel);
  m_pIPC->m_Events.AddEventHandler(ezMakeDelegate(&ezProcessTask::EventHandlerIPC, this));
}

ezProcessTask::~ezProcessTask()
{
  ShutdownProcess();
  m_pIPC->m_Events.RemoveEventHandler(ezMakeDelegate(&ezProcessTask::EventHandlerIPC, this));
  EZ_DEFAULT_DELETE(m_pIPC);
}


void ezProcessTask::StartProcess()
{
  const ezRTTI* pFirstAllowedMessageType = nullptr;
  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed = false;

  ezStringBuilder tmp;

  QStringList args;
  args << "-appname";
  args << ezApplication::GetApplicationInstance()->GetApplicationName().GetData();
  args << "-appid";
  args << QString::number(m_uiProcessorID);
  args << "-project";
  args << ezToolsProject::GetSingleton()->GetProjectFile().GetData();
  args << "-renderer";
  args << ezGameApplication::GetActiveRenderer().GetData(tmp);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  const char* EditorProcessorExecutable = "EditorProcessor.exe";
#else
  const char* EditorProcessorExecutable = "EditorProcessor";
#endif

  if (m_pIPC->StartClientProcess(EditorProcessorExecutable, args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
}

void ezProcessTask::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  m_bProcessShouldBeRunning = false;
  m_pIPC->CloseConnection();
}

void ezProcessTask::EventHandlerIPC(const ezProcessCommunicationChannel::Event& e)
{
  if (const ezProcessAssetResponseMsg* pMsg = ezDynamicCast<const ezProcessAssetResponseMsg*>(e.m_pMessage))
  {
    m_Status = pMsg->m_Status;
    m_bWaiting = false;
    m_LogEntries.Swap(pMsg->m_LogEntries);
  }
}

bool ezProcessTask::GetNextAssetToProcess(ezAssetInfo* pInfo, ezUuid& out_guid, ezDataDirPath& out_path)
{
  bool bComplete = true;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(pInfo->m_Path, false, pTypeDesc).Succeeded())
  {
    auto flags = static_cast<const ezAssetDocumentTypeDescriptor*>(pTypeDesc)->m_AssetDocumentFlags;

    if (flags.IsAnySet(ezAssetDocumentFlags::OnlyTransformManually | ezAssetDocumentFlags::DisableTransform))
      return false;
  }

  auto TestFunc = [this, &bComplete](const ezSet<ezString>& files) -> ezAssetInfo* {
    for (const auto& sFile : files)
    {
      if (ezAssetInfo* pFileInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(sFile))
      {
        switch (pFileInfo->m_TransformState)
        {
          case ezAssetInfo::TransformState::Unknown:
          case ezAssetInfo::TransformState::TransformError:
          case ezAssetInfo::TransformState::MissingTransformDependency:
          case ezAssetInfo::TransformState::MissingThumbnailDependency:
          case ezAssetInfo::TransformState::CircularDependency:
          {
            bComplete = false;
            continue;
          }
          case ezAssetInfo::TransformState::NeedsTransform:
          case ezAssetInfo::TransformState::NeedsThumbnail:
          {
            bComplete = false;
            return pFileInfo;
          }
          case ezAssetInfo::TransformState::UpToDate:
            continue;

          case ezAssetInfo::TransformState::NeedsImport:
            // the main processor has to do this itself
            continue;

            EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
        }
      }
    }
    return nullptr;
  };

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_TransformDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path);
  }

  if (ezAssetInfo* pDepInfo = TestFunc(pInfo->m_Info->m_ThumbnailDependencies))
  {
    return GetNextAssetToProcess(pDepInfo, out_guid, out_path);
  }

  // not needed to go through package dependencies here

  if (bComplete && !ezAssetCurator::GetSingleton()->m_Updating.Contains(pInfo->m_Info->m_DocumentID) &&
      !ezAssetCurator::GetSingleton()->m_TransformStateStale.Contains(pInfo->m_Info->m_DocumentID))
  {
    ezAssetCurator::GetSingleton()->m_Updating.Insert(pInfo->m_Info->m_DocumentID);
    out_guid = pInfo->m_Info->m_DocumentID;
    out_path = pInfo->m_Path;
    return true;
  }

  return false;
}

bool ezProcessTask::GetNextAssetToProcess(ezUuid& out_guid, ezDataDirPath& out_path)
{
  EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsTransform].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path);
      if (bRes)
        return true;
    }
  }

  for (auto it = ezAssetCurator::GetSingleton()->m_TransformState[ezAssetInfo::TransformState::NeedsThumbnail].GetIterator(); it.IsValid(); ++it)
  {
    ezAssetInfo* pInfo = ezAssetCurator::GetSingleton()->GetAssetInfo(it.Key());
    if (pInfo)
    {
      bool bRes = GetNextAssetToProcess(pInfo, out_guid, out_path);
      if (bRes)
        return true;
    }
  }

  return false;
}


void ezProcessTask::OnProcessCrashed()
{
  m_Status = ezStatus("Asset processor crashed");
  ezLogEntryDelegate logger([this](ezLogEntry& ref_entry) { m_LogEntries.PushBack(std::move(ref_entry)); });
  ezLog::Error(&logger, "AssetProcessor crashed!");
  ezLog::Error(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "AssetProcessor crashed!");
}

bool ezProcessTask::BeginExecute()
{
  m_LogEntries.Clear();
  m_TransitiveHull.Clear();
  m_Status = ezStatus(EZ_SUCCESS);
  {
    EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);

    if (!GetNextAssetToProcess(m_AssetGuid, m_AssetPath))
    {
      m_AssetGuid = ezUuid();
      m_AssetPath.Clear();
      m_bDidWork = false;
      return false;
    }

    m_bDidWork = true;
    ezAssetInfo::TransformState state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(m_AssetGuid, nullptr, nullptr, m_uiAssetHash, m_uiThumbHash);
    EZ_ASSERT_DEV(state == ezAssetInfo::TransformState::NeedsTransform || state == ezAssetInfo::TransformState::NeedsThumbnail, "An asset was selected that is already up to date.");

    ezSet<ezString> dependencies;

    ezStringBuilder sTemp;
    ezAssetCurator::GetSingleton()->GenerateTransitiveHull(ezConversionUtils::ToString(m_AssetGuid, sTemp), dependencies, true, true);

    m_TransitiveHull.Reserve(dependencies.GetCount());
    for (const ezString& str : dependencies)
    {
      m_TransitiveHull.PushBack(str);
    }
  }

  if (!m_bProcessShouldBeRunning)
  {
    StartProcess();
  }

  if (m_bProcessCrashed)
  {
    OnProcessCrashed();
    return false;
  }
  else
  {
    ezLog::Info(&ezAssetProcessor::GetSingleton()->m_CuratorLog, "Processing '{0}'", m_AssetPath.GetDataDirRelativePath());
    // Send and wait
    ezProcessAssetMsg msg;
    msg.m_AssetGuid = m_AssetGuid;
    msg.m_AssetHash = m_uiAssetHash;
    msg.m_ThumbHash = m_uiThumbHash;
    msg.m_sAssetPath = m_AssetPath;
    msg.m_DepRefHull.Swap(m_TransitiveHull);
    msg.m_sPlatform = ezAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pIPC->SendMessage(&msg);
    m_bWaiting = true;
    return true;
  }
}

bool ezProcessTask::FinishExecute()
{
  if (m_bWaiting)
  {
    m_pIPC->ProcessMessages();
    if (!m_pIPC->IsClientAlive())
    {
      m_bProcessCrashed = true;
    }

    if (m_bProcessCrashed)
    {
      m_bWaiting = false;
      OnProcessCrashed();
    }
    if (m_bWaiting)
      return false;
  }

  if (m_Status.Succeeded())
  {
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(m_AssetGuid);
    ezAssetCurator::GetSingleton()->NeedsReloadResources(m_AssetGuid);
  }
  else
  {
    if (m_Status.m_Result == ezTransformResult::NeedsImport)
    {
      ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, ezAssetInfo::TransformState::NeedsImport);
    }
    else
    {
      ezAssetCurator::GetSingleton()->UpdateAssetTransformLog(m_AssetGuid, m_LogEntries);
      ezAssetCurator::GetSingleton()->UpdateAssetTransformState(m_AssetGuid, ezAssetInfo::TransformState::TransformError);
    }
  }

  EZ_LOCK(ezAssetCurator::GetSingleton()->m_CuratorMutex);
  ezAssetCurator::GetSingleton()->m_Updating.Remove(m_AssetGuid);
  return true;
}

ezUInt32 ezProcessThread::Run()
{
  ezAssetProcessor::GetSingleton()->Run();
  return 0;
}
