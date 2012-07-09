/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "client.h"
#include "xbmc_pvr_dll.h"
#include <stdlib.h>
#include "N7Data.h"

using namespace std;
using namespace ADDON;

bool         m_bCreated  = false;
ADDON_STATUS m_CurStatus = ADDON_STATUS_UNKNOWN;
int          g_iClientId = -1;

/* User adjustable settings are saved here.
 * Default values are defined inside client.h
 * and exported to the other source files.
 */
std::string g_strHostname             = DEFAULT_HOST;
int         g_iConnectTimeout         = DEFAULT_CONNECT_TIMEOUT;
int         g_iPortStream             = DEFAULT_STREAM_PORT;
int         g_iPortWeb                = DEFAULT_WEB_PORT;
int         g_iUpdateInterval         = DEFAULT_UPDATE_INTERVAL;
std::string g_strUsername             = "";
std::string g_strRecordingPath        = "";
std::string g_strPassword             = "";
std::string g_szUserPath              = "";
bool        g_bCheckForChannelUpdates = true;
bool        g_bSetPowerstate          = false;
bool        g_bOnlyOneGroup           = false;
std::string g_szClientPath            = "";
std::string g_strChannelDataPath      = "/tmp/";

CHelper_libXBMC_addon *XBMC           = NULL;
CHelper_libXBMC_pvr   *PVR            = NULL;
N7                *N7Data             = NULL;

extern "C" {

void ADDON_ReadSettings(void)
{
  /* read setting "host" from settings.xml */
  char * buffer;
  buffer = (char*) malloc (1024);
  buffer[0] = 0; /* Set the end of string */

  if (XBMC->GetSetting("host", buffer))
    g_strHostname = buffer;
  else
    g_strHostname = DEFAULT_HOST;
  buffer[0] = 0; /* Set the end of string */

  /* read setting "user" from settings.xml */
  if (XBMC->GetSetting("user", buffer))
    g_strUsername = buffer;
  else
    g_strUsername = "";
  buffer[0] = 0; /* Set the end of string */
  
  /* read setting "pass" from settings.xml */
  if (XBMC->GetSetting("pass", buffer))
    g_strPassword = buffer;
  else
    g_strPassword = "";
  
  /* read setting "streamport" from settings.xml */
  if (!XBMC->GetSetting("streamport", &g_iPortStream))
    g_iPortStream = DEFAULT_STREAM_PORT;
  
  /* read setting "webport" from settings.xml */
  if (!XBMC->GetSetting("webport", &g_iPortWeb))
    g_iPortWeb = DEFAULT_WEB_PORT;
  
  /* read setting "checkchannels" from settings.xml */
  if (!XBMC->GetSetting("checkchannels", &g_bCheckForChannelUpdates))
    g_bCheckForChannelUpdates = true;
  
  /* read setting "updateint" from settings.xml */
  if (!XBMC->GetSetting("updateint", &g_iUpdateInterval))
    g_iConnectTimeout = DEFAULT_UPDATE_INTERVAL;

  /* read setting "channeldatapath" from settings.xml */
  if (XBMC->GetSetting("channeldatapath", buffer))
    g_strChannelDataPath = buffer;
  else
    g_strChannelDataPath = "/tmp/";
  
  free (buffer);
}

ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!hdl || !props)
    return ADDON_STATUS_UNKNOWN;

  PVR_PROPERTIES* pvrprops = (PVR_PROPERTIES*)props;

  XBMC = new CHelper_libXBMC_addon;
  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  PVR = new CHelper_libXBMC_pvr;
  if (!PVR->RegisterMe(hdl))
  {
    delete PVR;
    delete XBMC;
    PVR = NULL;
    XBMC = NULL;
    return ADDON_STATUS_UNKNOWN;
  }

  XBMC->Log(LOG_DEBUG, "%s - Creating N7 PVR-Client", __FUNCTION__);

  m_CurStatus     = ADDON_STATUS_UNKNOWN;
  g_iClientId     = pvrprops->iClientId;
  g_szUserPath   = pvrprops->strUserPath;
  g_szClientPath  = pvrprops->strClientPath;

  ADDON_ReadSettings();

  N7Data = new N7;
  if (!N7Data->Open()) 
  {
    delete N7Data;
    delete PVR;
    delete XBMC;
    N7Data = NULL;
    PVR = NULL;
    XBMC = NULL;
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;
    return m_CurStatus;
  }

  m_CurStatus = ADDON_STATUS_OK;
  m_bCreated = true;
  return m_CurStatus;
}

ADDON_STATUS ADDON_GetStatus()
{
  /* check whether we're still connected */
  if (m_CurStatus == ADDON_STATUS_OK && !N7Data->IsConnected())
    m_CurStatus = ADDON_STATUS_LOST_CONNECTION;

  return m_CurStatus;
}

void ADDON_Destroy()
{
  if (m_bCreated)
  {
    m_bCreated = false;
  }

  if (PVR)
  {
    delete PVR;
    PVR = NULL;
  }

  if (XBMC)
  {
    delete XBMC;
    XBMC = NULL;
  }

  if (N7Data)
  {
    delete N7Data;
    N7Data = NULL;
  }

  m_CurStatus = ADDON_STATUS_UNKNOWN;
}

bool ADDON_HasSettings()
{
  return true;
}

unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

ADDON_STATUS ADDON_SetSetting(const char *settingName, const void *settingValue)
{
  string str = settingName;
  if (str == "host")
  {
    string tmp_sHostname;
    XBMC->Log(LOG_INFO, "%s - Changed Setting 'host' from %s to %s", __FUNCTION__, g_strHostname.c_str(), (const char*) settingValue);
    tmp_sHostname = g_strHostname;
    g_strHostname = (const char*) settingValue;
    if (tmp_sHostname != g_strHostname)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (str == "user")
  {
    string tmp_sUsername = g_strUsername;
    g_strUsername = (const char*) settingValue;
    if (tmp_sUsername != g_strUsername)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'user'", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "pass")
  {
    string tmp_sPassword = g_strPassword;
    g_strPassword = (const char*) settingValue;
    if (tmp_sPassword != g_strPassword)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'pass'", __FUNCTION__);
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (str == "streamport")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iPortStream != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'streamport' from %u to %u", __FUNCTION__, g_iPortStream, iNewValue);
      g_iPortStream = iNewValue;
      return ADDON_STATUS_OK;
    }
  }
  else if (str == "webport")
  {
    int iNewValue = *(int*) settingValue + 1;
    if (g_iPortWeb != iNewValue)
    {
      XBMC->Log(LOG_INFO, "%s - Changed Setting 'webport' from %u to %u", __FUNCTION__, g_iPortWeb, iNewValue);
      g_iPortWeb = iNewValue;
      return ADDON_STATUS_OK;
    }
  }
  return ADDON_STATUS_OK;
}

void ADDON_Stop()
{
}

void ADDON_FreeSettings()
{
}

/***********************************************************
 * PVR Client AddOn specific public library functions
 ***********************************************************/

PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES* pCapabilities)
{
  pCapabilities->bSupportsChannelSettings = false;
  pCapabilities->bSupportsTimeshift       = false;
  pCapabilities->bSupportsEPG             = false;
  pCapabilities->bSupportsTV              = true;
  pCapabilities->bSupportsRadio           = true;
  pCapabilities->bSupportsRecordings      = false;
  pCapabilities->bSupportsTimers          = false;
  pCapabilities->bSupportsChannelGroups   = false;
  pCapabilities->bSupportsChannelScan     = false;
  pCapabilities->bHandlesInputStream      = true;
  pCapabilities->bHandlesDemuxing         = false;

  return PVR_ERROR_NO_ERROR;
}

const char *GetBackendName(void)
{
  static const char *strBackendName = N7Data ? N7Data->GetServerName() : "unknown";
  return strBackendName;
}

const char *GetBackendVersion(void)
{
  static const char *strBackendVersion = "UNKNOWN";
  return strBackendVersion;
}

const char *GetConnectionString(void)
{
  CStdString strConnectionString;
  if (N7Data)
    strConnectionString.Format("%s%s", g_strHostname.c_str(), N7Data->IsConnected() ? "" : " (Not connected!)");
  else
    strConnectionString.Format("%s (addon error!)", g_strHostname.c_str());
  return strConnectionString.c_str();
}

PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed)
{
  return PVR_ERROR_SERVER_ERROR;
}

PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
  if (!N7Data || !N7Data->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return N7Data->GetEPGForChannel(handle, channel, iStart, iEnd);
}

int GetChannelsAmount(void)
{
  if (!N7Data || !N7Data->IsConnected())
    return 0;

  return N7Data->GetChannelsAmount();
}

PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio)
{
  if (!N7Data || !N7Data->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return N7Data->GetChannels(handle, bRadio);
}

int GetRecordingsAmount(void)
{
  return 0;
}

PVR_ERROR GetRecordings(PVR_HANDLE handle)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteRecording(const PVR_RECORDING &recording)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR RenameRecording(const PVR_RECORDING &recording)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetTimersAmount(void)
{
  return 0;
}

PVR_ERROR GetTimers(PVR_HANDLE handle)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR AddTimer(const PVR_TIMER &timer)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR UpdateTimer(const PVR_TIMER &timer)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

int GetCurrentClientChannel(void)
{
  if (!N7Data || !N7Data->IsConnected())
    return PVR_ERROR_SERVER_ERROR;

  return N7Data->GetCurrentClientChannel();
}

bool SwitchChannel(const PVR_CHANNEL &channel)
{
  if (!N7Data || !N7Data->IsConnected())
    return false;

  return N7Data->SwitchChannel(channel);
}

int GetChannelGroupsAmount(void)
{
  return 1;
}

PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group)
{
  return PVR_ERROR_NOT_IMPLEMENTED;
}

void CloseLiveStream(void) 
{ 
  N7Data->CloseLiveStream();
};

bool OpenLiveStream(const PVR_CHANNEL &channel) 
{ 
  if (!N7Data || !N7Data->IsConnected())
    return false;

  return N7Data->OpenLiveStream(channel);
}

const char * GetLiveStreamURL(const PVR_CHANNEL &channel) 
{ 
  if (!N7Data || !N7Data->IsConnected())
    return "";

  return N7Data->GetLiveStreamURL(channel);
}

/** UNUSED API FUNCTIONS */
PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus) { return PVR_ERROR_NO_ERROR; }
PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) { return PVR_ERROR_NOT_IMPLEMENTED; } 
void DemuxAbort(void) { return; }
DemuxPacket* DemuxRead(void) { return NULL; }
PVR_ERROR DialogChannelScan(void) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR RenameChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR MoveChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel) { return PVR_ERROR_NOT_IMPLEMENTED; }
bool OpenRecordedStream(const PVR_RECORDING &recording) { return false; }
void CloseRecordedStream(void) {}
int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekRecordedStream(long long iPosition, int iWhence /* = SEEK_SET */) { return 0; }
long long PositionRecordedStream(void) { return -1; }
long long LengthRecordedStream(void) { return 0; }
void DemuxReset(void) {}
void DemuxFlush(void) {}
int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize) { return 0; }
long long SeekLiveStream(long long iPosition, int iWhence /* = SEEK_SET */) { return -1; }
long long PositionLiveStream(void) { return -1; }
long long LengthLiveStream(void) { return -1; }
}
