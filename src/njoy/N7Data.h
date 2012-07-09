#pragma once 

#include "../platform/util/StdString.h"
#include "xmlParser.h"
#include "client.h"
#include "../platform/threads/threads.h"
    
struct N7WebResponse {
  char *response;
  int iSize;
};

#define CHANNELDATAVERSION  2

typedef enum N7_UPDATE_STATE
{
    N7_UPDATE_STATE_NONE,
    N7_UPDATE_STATE_FOUND,
    N7_UPDATE_STATE_UPDATED,
    N7_UPDATE_STATE_NEW
} N7_UPDATE_STATE;

struct N7Channel
{
  bool bRadio;
  int iUniqueId;
  int iChannelNumber;
  std::string strChannelName;
  std::string strServiceReference;
  std::string strStreamURL;
  std::string strIconPath;
  int iChannelState;

  N7Channel()
  {
    iChannelState = N7_UPDATE_STATE_NEW;
  }
  
  bool operator==(const N7Channel &right) const
  {
    bool bChanged = true;
    bChanged = bChanged && (bRadio == right.bRadio); 
    bChanged = bChanged && (iUniqueId == right.iUniqueId); 
    bChanged = bChanged && (iChannelNumber == right.iChannelNumber); 
    bChanged = bChanged && (! strChannelName.compare(right.strChannelName));
    bChanged = bChanged && (! strServiceReference.compare(right.strServiceReference));
    bChanged = bChanged && (! strStreamURL.compare(right.strStreamURL));
    bChanged = bChanged && (! strIconPath.compare(right.strIconPath));

    return bChanged;
  }

};

struct N7EPGEntry 
{
  int iEventId;
  std::string strServiceReference;
  std::string strTitle;
  int iChannelId;
  time_t startTime;
  time_t endTime;
  std::string strPlotOutline;
  std::string strPlot;
};

class N7  : public PLATFORM::CThread
{
private:

  // members
  bool  m_bIsConnected;
  std::string m_strServerName;
  std::string m_strURL;
  int m_iCurrentChannel;
  unsigned int m_iUpdateTimer;
  std::vector<N7Channel> m_channels;

  bool m_bInitial;
  unsigned int m_iClientIndexCounter;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;
 

  // functions

  void StoreChannelData();
  void LoadChannelData();
  CStdString GetHttpXML(CStdString& url);
  int GetChannelNumber(CStdString strServiceReference);
  CStdString URLEncodeInline(const CStdString& strData);
  static int N7WebResponseCallback(void *contents, int iLength, int iSize, void *memPtr); 
  bool LoadChannels();

  // helper functions
  static bool GetInt(XMLNode xRootNode, const char* strTag, int& iIntValue);
  static bool GetBoolean(XMLNode xRootNode, const char* strTag, bool& bBoolValue);
  static bool GetString(XMLNode xRootNode, const char* strTag, CStdString& strStringValue);
  static long TimeStringToSeconds(const CStdString &timeString);
  static int SplitString(const CStdString& input, const CStdString& delimiter, CStdStringArray &results, unsigned int iMaxStrings = 0);
  bool CheckForChannelUpdate();
  std::string& Escape(std::string &s, std::string from, std::string to);

protected:
  virtual void *Process(void);

public:
  N7(void);
  ~N7();

  const char * GetServerName();
  bool IsConnected(); 
  int GetChannelsAmount(void);
  PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio);
  PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  int GetCurrentClientChannel(void);
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  bool Open();
  void Action();
};

