/*
MIT License

Copyright (c) 2010-2021 Eren Okka

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string>

#include "dde.h"
#include "string.h"

namespace win {

DynamicDataExchange::DynamicDataExchange()
    : conversation_(nullptr),
      instance_(0),
      is_unicode_(false) {
}

DynamicDataExchange::~DynamicDataExchange() {
  Disconnect();
  UnInitialize();
}

////////////////////////////////////////////////////////////////////////////////

BOOL DynamicDataExchange::Initialize(DWORD afCmd, bool unicode) {
  is_unicode_ = unicode;

  if (is_unicode_) {
    ::DdeInitializeW(&instance_, DdeCallback, afCmd, 0);
  } else {
    ::DdeInitializeA(&instance_, DdeCallback, afCmd, 0);
  }

  return instance_ != 0;
}

void DynamicDataExchange::UnInitialize() {
  if (instance_) {
    ::DdeUninitialize(instance_);
    instance_ = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

BOOL DynamicDataExchange::Connect(const std::wstring& service,
                                  const std::wstring& topic) {
  if (!instance_)
    return FALSE;

  HSZ hszService = CreateStringHandle(service);
  HSZ hszTopic = CreateStringHandle(topic);

  conversation_ = ::DdeConnect(instance_, hszService, hszTopic, NULL);

  FreeStringHandle(hszTopic);
  FreeStringHandle(hszService);

  return conversation_ != nullptr;
}

void DynamicDataExchange::Disconnect() {
  if (conversation_) {
    ::DdeDisconnect(conversation_);
    conversation_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////

BOOL DynamicDataExchange::ClientTransaction(const std::wstring& item,
                                            const std::wstring& data,
                                            std::wstring* output,
                                            UINT wType) {
  DWORD dwResult = 0;
  HSZ hszItem = wType != XTYP_EXECUTE ? CreateStringHandle(item) : nullptr;

  LPBYTE pData =
      is_unicode_ ? (LPBYTE)data.data() : (LPBYTE)WstrToStr(data).data();
  DWORD cbData = static_cast<DWORD>(
      is_unicode_ ? (data.size() + 1) * sizeof(WCHAR) : data.size() + 1);
  UINT wFmt = is_unicode_ ? CF_UNICODETEXT : CF_TEXT;

  HDDEDATA hData = ::DdeClientTransaction(
      pData, cbData, conversation_, hszItem, wFmt, wType, 3000, &dwResult);

  FreeStringHandle(hszItem);

  if (output) {
    char szResult[255];
    ::DdeGetData(hData, (unsigned char*)szResult, 255, 0);
    output->assign(StrToWstr(szResult));
  }

  return hData != 0;
}

BOOL DynamicDataExchange::IsAvailable() {
  return instance_ != 0;
}

BOOL DynamicDataExchange::NameService(const std::wstring& service, UINT afCmd) {
  HSZ hszService = CreateStringHandle(service);
  HDDEDATA result = ::DdeNameService(instance_, hszService, 0, afCmd);
  FreeStringHandle(hszService);
  return result != nullptr;
}

////////////////////////////////////////////////////////////////////////////////

HSZ DynamicDataExchange::CreateStringHandle(const std::wstring& str) {
  if (is_unicode_) {
    return ::DdeCreateStringHandleW(instance_, str.c_str(), CP_WINUNICODE);
  } else {
    return ::DdeCreateStringHandleA(instance_, WstrToStr(str).c_str(), CP_WINANSI);
  }
}

void DynamicDataExchange::FreeStringHandle(HSZ str) {
  if (str)
    ::DdeFreeStringHandle(instance_, str);
}

HDDEDATA CALLBACK DynamicDataExchange::DdeCallback(UINT uType, UINT uFmt,
                                                   HCONV hconv,
                                                   HSZ hsz1, HSZ hsz2,
                                                   HDDEDATA hdata,
                                                   ULONG_PTR dwData1,
                                                   ULONG_PTR dwData2) {
  DWORD cb = 0;
  LPVOID lpData = nullptr;
  char sz1[256] = {'\0'};
  char sz2[256] = {'\0'};
  //DdeQueryStringA(instance_, hsz1, sz1, 256, CP_WINANSI);
  //DdeQueryStringA(instance_, hsz2, sz2, 256, CP_WINANSI);

  switch (uType) {
    case XTYP_CONNECT: {
      OutputDebugStringA("[CONNECT]\n");
      //BOOL result = OnConnect();
      return reinterpret_cast<HDDEDATA>(TRUE);
    }

    case XTYP_POKE: {
      if (hdata)
        lpData = DdeAccessData(hdata, &cb);
#ifdef _DEBUG
      std::string str = "[POKE]";
      str += "Topic: " + *sz1;
      str += " - Item: " + *sz2;
      if (lpData) str += " - Data: "; str += (LPCSTR)lpData;
      str += "\n";
      OutputDebugStringA(str.c_str());
#endif
      //OnPoke();
      if (hdata)
        DdeUnaccessData(hdata);
      return reinterpret_cast<HDDEDATA>(DDE_FACK);
    }

    case XTYP_REQUEST: {
      // TODO: Call DdeCreateDataHandle();
#ifdef _DEBUG
      std::string str = "[REQUEST] ";
      str += "Topic: "; str += sz1;
      str += " - Item: "; str += sz2;
      str += "\n";
      OutputDebugStringA(str.c_str());
#endif
      //OnRequest();
      break;
    }

    default:
      break;
  }

  return reinterpret_cast<HDDEDATA>(0);
}

}  // namespace win
