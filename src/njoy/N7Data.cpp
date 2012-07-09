#include "N7Data.h"

#include <curl/curl.h>
#include "client.h" 
#include <iostream> 
#include <fstream> 

using namespace ADDON;
using namespace PLATFORM;

std::string& N7::Escape(std::string &s, std::string from, std::string to)
{ 
  int pos = -1;
  while ( (pos = s.find(from, pos+1) ) != std::string::npos)         
    s.erase(pos, from.length()).insert(pos, to);        

  return s;     
} 

bool N7::CheckForChannelUpdate() 
{
  if (!g_bCheckForChannelUpdates)
    return false;

  std::vector<N7Channel> oldchannels = m_channels;

  LoadChannels();

  for(unsigned int i=0; i< oldchannels.size(); i++)
    oldchannels[i].iChannelState = N7_UPDATE_STATE_NONE;

  for (unsigned int j=0; j<m_channels.size(); j++)
  {
    for (unsigned int i=0; i<oldchannels.size(); i++)
    {
      if (!oldchannels[i].strServiceReference.compare(m_channels[j].strServiceReference))
      {
        if(oldchannels[i] == m_channels[j])
        {
          m_channels[j].iChannelState = N7_UPDATE_STATE_FOUND;
          oldchannels[i].iChannelState = N7_UPDATE_STATE_FOUND;
        }
        else
        {
          oldchannels[i].iChannelState = N7_UPDATE_STATE_UPDATED;
          m_channels[j].iChannelState = N7_UPDATE_STATE_UPDATED;
        }
      }
    }
  }
  
  int iNewChannels = 0; 
  for (unsigned int i=0; i<m_channels.size(); i++) 
  {
    if (m_channels[i].iChannelState == N7_UPDATE_STATE_NEW)
      iNewChannels++;
  }

  int iRemovedChannels = 0;
  int iNotUpdatedChannels = 0;
  int iUpdatedChannels = 0;
  for (unsigned int i=0; i<oldchannels.size(); i++) 
  {
    if(oldchannels[i].iChannelState == N7_UPDATE_STATE_NONE)
      iRemovedChannels++;
    
    if(oldchannels[i].iChannelState == N7_UPDATE_STATE_FOUND)
      iNotUpdatedChannels++;  

    if(oldchannels[i].iChannelState == N7_UPDATE_STATE_UPDATED)
      iUpdatedChannels++;
  }

  XBMC->Log(LOG_INFO, "%s No of channels: removed [%d], untouched [%d], updated '%d', new '%d'", __FUNCTION__, iRemovedChannels, iNotUpdatedChannels, iUpdatedChannels, iNewChannels); 

  if ((iRemovedChannels > 0) || (iUpdatedChannels > 0) || (iNewChannels > 0))
  {
    //Channels have been changed, so return "true"
    return true;
  }
  else 
  {
    m_channels = oldchannels;
    return false;
  }
}

void N7::LoadChannelData()
{
  XBMC->Log(LOG_DEBUG, "%s Load channel data from file: '%schanneldata.xml'", __FUNCTION__, g_strChannelDataPath.c_str());

  XMLResults pResults;

  CStdString strFileName;
  strFileName.Format("%schanneldata.xml", g_strChannelDataPath.c_str());
  XMLNode xMainNode = XMLNode::parseFile(strFileName.c_str(), "channeldata", &pResults);

  if (pResults.error != 0) {
    XBMC->Log(LOG_ERROR, "%s error parsing channeldata!", __FUNCTION__);
    return;
  }

  int iVersion;
  if (!GetInt(xMainNode, "version", iVersion))
  {
    XBMC->Log(LOG_NOTICE, "%s No channeldata version string found, abort loading data from HDD!", __FUNCTION__);
    return;
  } 

  XBMC->Log(LOG_DEBUG, "%s Found channeldata version: '%d', current channeldata version: '%d'", __FUNCTION__, iVersion, CHANNELDATAVERSION);

  if (iVersion != CHANNELDATAVERSION) {
    XBMC->Log(LOG_NOTICE, "%s The channeldata versions do not match, we will abort loading the data from the HDD.", __FUNCTION__);
    return;
  }
  
  XMLNode xNode = xMainNode.getChildNode("channellist");
  int n = xNode.nChildNode("channel");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("channel", i);

    CStdString strTmp;
    bool bTmp;
    int iTmp;

    N7Channel channel; 
    
    if (GetBoolean(xTmp, "radio", bTmp)) {
      channel.bRadio = bTmp;
    }

    if (!GetInt(xTmp, "id", iTmp))
      continue;
    channel.iUniqueId = iTmp;

    if (!GetInt(xTmp, "channelnumber", iTmp))
      continue;
    channel.iChannelNumber = iTmp;

    if (!GetString(xTmp, "channelname", strTmp))
      continue;
    channel.strChannelName = strTmp.c_str();

    if (!GetString(xTmp, "servicereference", strTmp))
      continue;

    channel.strServiceReference = strTmp.c_str();
     
    if (!GetString(xTmp, "streamurl", strTmp))
      continue;
    channel.strStreamURL = strTmp.c_str();

    m_channels.push_back(channel);

    XBMC->Log(LOG_DEBUG, "%s Loaded channel '%s' from HDD", __FUNCTION__, channel.strChannelName.c_str());
  }

}

void N7::StoreChannelData()
{
  XBMC->Log(LOG_DEBUG, "%s Store channel data into file: '%schanneldata.xml'", __FUNCTION__, g_strChannelDataPath.c_str());

  std::ofstream stream;
  
  CStdString strFileName;
  strFileName.Format("%schanneldata.xml", g_strChannelDataPath.c_str());
  stream.open(strFileName.c_str());

  if(stream.fail())
    XBMC->Log(LOG_ERROR, "%s Could not open channeldata file for writing!", __FUNCTION__);

  CStdString strTmp;

  stream << "<channeldata>\n";
  stream << "\t<version>\n" << CHANNELDATAVERSION;
  stream << "\t</version>\n";
  stream << "\t<channellist>\n";

  for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    stream << "\t\t<channel>\n";
    N7Channel &channel = m_channels.at(iChannelPtr);

    // store channel properties
    stream << "\t\t\t<radio>";
    if (channel.bRadio)
      stream << "true";
    else
      stream << "false";
    stream << "</radio>\n";

    stream << "\t\t\t<id>" << channel.iUniqueId;
    stream << "</id>\n";
    stream << "\t\t\t<channelnumber>" << channel.iChannelNumber;
    stream << "</channelnumber>\n";
    
    strTmp = channel.strChannelName;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<channelname>" << strTmp;
    stream << "</channelname>\n";

    strTmp = channel.strServiceReference;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<servicereference>" << strTmp;
    stream << "</servicereference>\n";

    strTmp =  channel.strStreamURL;
    Escape(strTmp, "&", "&quot;");
    Escape(strTmp, "<", "&lt;");
    Escape(strTmp, ">", "&gt;");

    stream << "\t\t\t<streamurl>" << strTmp;
    stream << "</streamurl>\n";

    stream << "\t\t</channel>\n";

  }
  stream << "\t</channellist>\n";
  stream << "</channeldata>\n";
  stream.close();
}

N7::N7() 
{
  m_bIsConnected = false;
  m_strServerName = "N7";
  CStdString strURL = "";

  // simply add user@pass in front of the URL if username/password is set
  if ((g_strUsername.length() > 0) && (g_strPassword.length() > 0))
    strURL.Format("%s:%s@", g_strUsername.c_str(), g_strPassword.c_str());
  strURL.Format("http://%s%s:%u/", strURL.c_str(), g_strHostname.c_str(), g_iPortWeb);
  m_strURL = strURL.c_str();
  m_iCurrentChannel = -1;
  m_iClientIndexCounter = 1;
  m_bInitial = false;

  m_iUpdateTimer = 0;
}

// Curl callback
int N7::N7WebResponseCallback(void *contents, int iLength, int iSize, void *memPtr)
{
  int iRealSize = iSize * iLength;
  struct N7WebResponse *resp = (struct N7WebResponse*) memPtr;

  resp->response = (char*) realloc(resp->response, resp->iSize + iRealSize + 1);

  if (resp->response == NULL)
  {
    XBMC->Log(LOG_ERROR, "%s Could not allocate memeory!", __FUNCTION__);
    return 0;
  }

  memcpy(&(resp->response[resp->iSize]), contents, iRealSize);
  resp->iSize += iRealSize;
  resp->response[resp->iSize] = 0;

  return iRealSize;
}

bool N7::Open()
{
  CLockObject lock(m_mutex);

  XBMC->Log(LOG_NOTICE, "%s - N7 Addon Configuration options", __FUNCTION__);
  XBMC->Log(LOG_NOTICE, "%s - Hostname: '%s'", __FUNCTION__, g_strHostname.c_str());
  XBMC->Log(LOG_NOTICE, "%s - WebPort: '%d'", __FUNCTION__, g_iPortWeb);

  LoadChannelData();

  if (m_channels.size() == 0) {
    XBMC->Log(LOG_DEBUG, "%s No stored channels found, fetch from webapi", __FUNCTION__);
    if (!LoadChannels())
      return false;

    m_bInitial = true;
    StoreChannelData();
  }
  m_bIsConnected = true;

  XBMC->Log(LOG_INFO, "%s Starting separate client update thread...", __FUNCTION__);
  CreateThread(); 
  
  return IsRunning(); 
}

void  *N7::Process()
{
  XBMC->Log(LOG_DEBUG, "%s - starting", __FUNCTION__);

  while(!IsStopped())
  {

    Sleep(5 * 1000);
    m_iUpdateTimer += 5;

    if (((int)m_iUpdateTimer > (g_iUpdateInterval * 60)) || (m_bInitial == false))
    {
      m_iUpdateTimer = 0;
 
      if (!m_bInitial)
      {
        // Load the TV channels - close connection if no channels are found
        bool bTriggerChannelsUpdate = CheckForChannelUpdate();

        m_bInitial = true;

        if (bTriggerChannelsUpdate) 
        {
          PVR->TriggerChannelUpdate();
          // Store the channel data on HDD
          StoreChannelData();
        }
      }
    
      // Trigger Timer and Recording updates acording to the addon settings
      CLockObject lock(m_mutex);
      XBMC->Log(LOG_INFO, "%s Perform Updates!", __FUNCTION__);

    }

  }

  CLockObject lock(m_mutex);
  m_started.Broadcast();
  //XBMC->Log(LOG_DEBUG, "%s - exiting", __FUNCTION__);

  return NULL;
}


bool N7::LoadChannels() 
{
  CStdString strUrl;
  strUrl.Format("%sn7channel_nt.xml", m_strURL.c_str());

  CStdString strXML = GetHttpXML(strUrl);

  if(strXML.length() == 0)
  {
    XBMC->Log(LOG_DEBUG, "%s - Could not open connection to N7 backend.", __FUNCTION__);
    m_bIsConnected = false;
  }
  else
  {
    m_bIsConnected = true;
    XBMC->Log(LOG_DEBUG, "%s - Connected to N7 backend.", __FUNCTION__);

    XMLResults xe;
    XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

    if(xe.error != 0)  {
      XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
      return false;
    }

    XMLNode xNode = xMainNode.getChildNode("rss");
    xNode = xNode.getChildNode("channel");
    int n = xNode.nChildNode("item");

    XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

    int iUniqueChannelId = 0;

    for(int i=0; i<n; i++)
    {
      XMLNode xTmp = xNode.getChildNode("item", i);
      CStdString strTmp;
      int iTmp;

      N7Channel newChannel;
      newChannel.bRadio = false;

      /* channel number */
      if (!GetInt(xTmp, "number", newChannel.iChannelNumber))
        newChannel.iChannelNumber = m_channels.size()+1;

      newChannel.iUniqueId = m_channels.size()+1;
      newChannel.strServiceReference = strTmp;

      if (!GetString(xTmp, "title", strTmp)) 
        continue;

           /* channel name */
      if (!GetString(xTmp, "title", strTmp))
        continue;
      newChannel.strChannelName = strTmp;

      /* icon path */
      XMLNode xMedia = xTmp.getChildNode("media:thumbnail");
      strTmp = xMedia.getAttribute("url");

      newChannel.strIconPath = strTmp;

      /* channel url */
      if (!GetString(xTmp, "guid", strTmp))
        newChannel.strStreamURL = "";
      else
        newChannel.strStreamURL = strTmp;

      m_channels.push_back(newChannel);

      XBMC->Log(LOG_INFO, "%s Loaded channel: %s, Icon: %s", __FUNCTION__, newChannel.strChannelName.c_str(), newChannel.strIconPath.c_str());
    }

  }
  XBMC->Log(LOG_INFO, "%s Loaded %d Channels", __FUNCTION__, m_channels.size()); 
  return true;
}

bool N7::IsConnected() 
{
  return m_bIsConnected;
}

CStdString N7::GetHttpXML(CStdString& url) 
{
  CLockObject lock(m_mutex);
  CURL* curl_handle;

  XBMC->Log(LOG_INFO, "%s Open webAPI with URL: '%s'", __FUNCTION__, url.c_str());

  struct N7WebResponse response;

  response.response = (char*) malloc(1);
  response.iSize = 0;

  // retrieve the webpage and store it in memory
  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &N7WebResponseCallback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&response);
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "vuplus-pvraddon-agent/1.0");
  curl_easy_perform(curl_handle);

  if (response.iSize == 0)
  {
    XBMC->Log(LOG_INFO, "%s Could not open webAPI", __FUNCTION__);
    return "";
  }

  CStdString strTmp;
  strTmp.Format("%s", response.response);

  XBMC->Log(LOG_INFO, "%s Got result. Length: %u", __FUNCTION__, strTmp.length());
  
  free(response.response);
  curl_easy_cleanup(curl_handle);

  return strTmp;
}

const char * N7::GetServerName() 
{
  return m_strServerName.c_str();  
}

int N7::GetChannelsAmount()
{
  return m_channels.size();
}

PVR_ERROR N7::GetChannels(PVR_HANDLE handle, bool bRadio) 
{
    for (unsigned int iChannelPtr = 0; iChannelPtr < m_channels.size(); iChannelPtr++)
  {
    N7Channel &channel = m_channels.at(iChannelPtr);
    if (channel.bRadio == bRadio)
    {
      PVR_CHANNEL xbmcChannel;
      memset(&xbmcChannel, 0, sizeof(PVR_CHANNEL));

      xbmcChannel.iUniqueId         = channel.iUniqueId;
      xbmcChannel.bIsRadio          = channel.bRadio;
      xbmcChannel.iChannelNumber    = channel.iChannelNumber;
      xbmcChannel.strChannelName    = channel.strChannelName.c_str();
      xbmcChannel.strInputFormat    = ""; // unused

      CStdString strStream;
      strStream.Format("pvr://stream/tv/%i.ts", channel.iUniqueId);
      xbmcChannel.strStreamURL      = strStream.c_str(); //channel.strStreamURL.c_str();
      xbmcChannel.iEncryptionSystem = 0;
      
      xbmcChannel.bIsHidden         = false;

      PVR->TransferChannelEntry(handle, &xbmcChannel);
    }
  }

  return PVR_ERROR_NO_ERROR;
}

N7::~N7() 
{
  StopThread();
  
  m_channels.clear();  
  m_bIsConnected = false;
}

PVR_ERROR N7::GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd)
{
/*
  VuChannel &myChannel = m_channels.at(channel.iUniqueId-1);

  CStdString url;
  url.Format("%s%s%s",  m_strURL.c_str(), "web/epgservice?sRef=",  URLEncodeInline(myChannel.strServiceReference)); 
 
  CStdString strXML;
  strXML = GetHttpXML(url);

  int iNumEPG = 0;

  XMLResults xe;
  XMLNode xMainNode = XMLNode::parseString(strXML.c_str(), NULL, &xe);

  if(xe.error != 0)  {
    XBMC->Log(LOG_ERROR, "%s Unable to parse XML. Error: '%s' ", __FUNCTION__, XMLNode::getError(xe.error));
    return PVR_ERROR_SERVER_ERROR;
  }

  XMLNode xNode = xMainNode.getChildNode("e2eventlist");
  int n = xNode.nChildNode("e2event");

  XBMC->Log(LOG_INFO, "%s Number of elements: '%d'", __FUNCTION__, n);

  for (int i = 0; i<n; i++)
  {
    XMLNode xTmp = xNode.getChildNode("e2event", i);

    CStdString strTmp;
    int iTmpStart;
    int iTmp;

    // check and set event starttime and endtimes
    if (!GetInt(xTmp, "e2eventstart", iTmpStart)) 
      continue;
 
    if (!GetInt(xTmp, "e2eventduration", iTmp))
      continue;

    if ((iEnd > 1) && (iEnd < (iTmpStart + iTmp)))
       continue;
    
    VuEPGEntry entry;
    entry.startTime = iTmpStart;
    entry.endTime = iTmpStart + iTmp;

    if (!GetInt(xTmp, "e2eventid", entry.iEventId))  
      continue;

    entry.iChannelId = channel.iUniqueId;
    
    if(!GetString(xTmp, "e2eventtitle", strTmp))
      continue;

    entry.strTitle = strTmp;
    
    entry.strServiceReference = myChannel.strServiceReference;

    if (GetString(xTmp, "e2eventdescriptionextended", strTmp))
      entry.strPlot = strTmp;

    if (GetString(xTmp, "e2eventdescription", strTmp))
       entry.strPlotOutline = strTmp;

    EPG_TAG broadcast;
    memset(&broadcast, 0, sizeof(EPG_TAG));

    broadcast.iUniqueBroadcastId  = entry.iEventId;
    broadcast.strTitle            = entry.strTitle.c_str();
    broadcast.iChannelNumber      = channel.iChannelNumber;
    broadcast.startTime           = entry.startTime;
    broadcast.endTime             = entry.endTime;
    broadcast.strPlotOutline      = entry.strPlotOutline.c_str();
    broadcast.strPlot             = entry.strPlot.c_str();
    broadcast.strIconPath         = ""; // unused
    broadcast.iGenreType          = 0; // unused
    broadcast.iGenreSubType       = 0; // unused
    broadcast.strGenreDescription = "";
    broadcast.firstAired          = 0;  // unused
    broadcast.iParentalRating     = 0;  // unused
    broadcast.iStarRating         = 0;  // unused
    broadcast.bNotify             = false;
    broadcast.iSeriesNumber       = 0;  // unused
    broadcast.iEpisodeNumber      = 0;  // unused
    broadcast.iEpisodePartNumber  = 0;  // unused
    broadcast.strEpisodeName      = ""; // unused

    PVR->TransferEpgEntry(handle, &broadcast);

    iNumEPG++; 

    XBMC->Log(LOG_INFO, "%s loaded EPG entry '%d:%s' channel '%d' start '%d' end '%d'", __FUNCTION__, broadcast.iUniqueBroadcastId, broadcast.strTitle, entry.iChannelId, entry.startTime, entry.endTime);
  }

  XBMC->Log(LOG_INFO, "%s Loaded %u EPG Entries for channel '%s'", __FUNCTION__, iNumEPG, channel.strChannelName);
*/
  return PVR_ERROR_NO_ERROR;
}

int N7::GetChannelNumber(CStdString strServiceReference)  
{
  for (unsigned int i = 0;i<m_channels.size();  i++) 
  {
    if (!strServiceReference.compare(m_channels[i].strServiceReference))
      return i+1;
  }
  return -1;
}

CStdString N7::URLEncodeInline(const CStdString& strData)
{
  CStdString buffer = strData;
  CURL* handle = curl_easy_init();
  char* encodedURL = curl_easy_escape(handle, strData.c_str(), strlen(strData.c_str()));

  buffer.Format("%s", encodedURL);
  curl_free(encodedURL);
  curl_easy_cleanup(handle);

  return buffer;
}

bool N7::GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty())
     return false;
  iIntValue = atoi(xNode.getText());
  return true;
}

bool N7::GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (xNode.isEmpty()) 
    return false;

  CStdString strEnabled = xNode.getText();

  strEnabled.ToLower();
  if (strEnabled == "off" || strEnabled == "no" || strEnabled == "disabled" || strEnabled == "false" || strEnabled == "0" )
    bBoolValue = false;
  else
  {
    bBoolValue = true;
    if (strEnabled != "on" && strEnabled != "yes" && strEnabled != "enabled" && strEnabled != "true")
      return false; // invalid bool switch - it's probably some other string.
  }
  return true;
}

bool N7::GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue)
{
  XMLNode xNode = xRootNode.getChildNode(strTag );
  if (!xNode.isEmpty())
  {
    strStringValue = xNode.getText();
    return true;
  }
  strStringValue.Empty();
  return false;
}

long N7::TimeStringToSeconds(const CStdString &timeString)
{
  CStdStringArray secs;
  SplitString(timeString, ":", secs);
  int timeInSecs = 0;
  for (unsigned int i = 0; i < secs.size(); i++)
  {
    timeInSecs *= 60;
    timeInSecs += atoi(secs[i]);
  }
  return timeInSecs;
}

int N7::SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings)
{
  int iPos = -1;
  int newPos = -1;
  int sizeS2 = delimiter.GetLength();
  int isize = input.GetLength();

  results.clear();
  std::vector<unsigned int> positions;

  newPos = input.Find (delimiter, 0);

  if ( newPos < 0 )
  {
    results.push_back(input);
    return 1;
  }

  while ( newPos > iPos )
  {
    positions.push_back(newPos);
    iPos = newPos;
    newPos = input.Find (delimiter, iPos + sizeS2);
  }

  // numFound is the number of delimeters which is one less
  // than the number of substrings
  unsigned int numFound = positions.size();
  if (iMaxStrings > 0 && numFound >= iMaxStrings)
    numFound = iMaxStrings - 1;

  for ( unsigned int i = 0; i <= numFound; i++ )
  {
    CStdString s;
    if ( i == 0 )
    {
      if ( i == numFound )
        s = input;
      else
        s = input.Mid( i, positions[i] );
    }
    else
    {
      int offset = positions[i - 1] + sizeS2;
      if ( offset < isize )
      {
        if ( i == numFound )
          s = input.Mid(offset);
        else if ( i > 0 )
          s = input.Mid( positions[i - 1] + sizeS2,
                         positions[i] - positions[i - 1] - sizeS2 );
      }
    }
    results.push_back(s);
  }
  // return the number of substrings
  return results.size();
}

int N7::GetCurrentClientChannel(void) 
{
  return m_iCurrentChannel;
}

const char* N7::GetLiveStreamURL(const PVR_CHANNEL &channelinfo)
{
  SwitchChannel(channelinfo);

  return m_channels.at(channelinfo.iUniqueId-1).strStreamURL.c_str();
}

bool N7::OpenLiveStream(const PVR_CHANNEL &channelinfo)
{
  XBMC->Log(LOG_INFO, "%s channel '%u'", __FUNCTION__, channelinfo.iUniqueId);

  if ((int)channelinfo.iUniqueId == m_iCurrentChannel)
    return true;

  return SwitchChannel(channelinfo);
}

void N7::CloseLiveStream(void) 
{
  m_iCurrentChannel = -1;
}

bool N7::SwitchChannel(const PVR_CHANNEL &channel)
{
  if ((int)channel.iUniqueId == m_iCurrentChannel)
    return true;

  return true;
}
