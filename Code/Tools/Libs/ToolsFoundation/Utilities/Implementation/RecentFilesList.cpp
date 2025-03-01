#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

void ezRecentFilesList::Insert(ezStringView sFile, ezInt32 iContainerWindow)
{
  ezStringBuilder sCleanPath = sFile;
  sCleanPath.MakeCleanPath();

  ezString s = sCleanPath;

  for (ezUInt32 i = 0; i < m_Files.GetCount(); i++)
  {
    if (m_Files[i].m_File == s)
    {
      m_Files.RemoveAtAndCopy(i);
      break;
    }
  }
  m_Files.PushFront(RecentFile(s, iContainerWindow));

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void ezRecentFilesList::Save(ezStringView sFile)
{
  ezDeferredFileWriter File;
  File.SetOutput(sFile);

  for (const RecentFile& file : m_Files)
  {
    ezStringBuilder sTemp;
    sTemp.Format("{0}|{1}", file.m_File, file.m_iContainerWindow);
    File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    File.WriteBytes("\n", sizeof(char)).IgnoreResult();
  }

  if (File.Close().Failed())
    ezLog::Error("Unable to open file '{0}' for writing!", sFile);
}

void ezRecentFilesList::Load(ezStringView sFile)
{
  m_Files.Clear();

  ezFileReader File;
  if (File.Open(sFile).Failed())
    return;

  ezStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  ezHybridArray<ezStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  ezStringBuilder sTemp, sTemp2;

  for (const ezStringView& sv : Lines)
  {
    sTemp = sv;
    ezHybridArray<ezStringView, 2> Parts;
    sTemp.Split(false, Parts, "|");

    if (!ezOSFile::ExistsFile(Parts[0].GetData(sTemp2)))
      continue;

    if (Parts.GetCount() == 1)
    {
      m_Files.PushBack(RecentFile(Parts[0], 0));
    }
    else if (Parts.GetCount() == 2)
    {
      ezStringBuilder sContainer = Parts[1];
      ezInt32 iContainerWindow = 0;
      ezConversionUtils::StringToInt(sContainer, iContainerWindow).IgnoreResult();
      m_Files.PushBack(RecentFile(Parts[0], iContainerWindow));
    }
  }
}
