#pragma once
/*
 *      Copyright (C) 2015 MrMC
 *      https://github.com/MrMC
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
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */



#include <string>
#include "FileItem.h"
#include "Application.h"
#include "SubtitleSearch.h"

class COpenSubtitlesSearch:public CSubtitleSearch
{
public:
  COpenSubtitlesSearch();
  virtual ~COpenSubtitlesSearch(void);
  virtual bool SubtitleSearch(const std::string &path,const std::string strLanguages,
                              const std::string preferredLanguage,CFileItemList &subtitlesList,
                              const std::string &strSearch);
  virtual std::string ModuleName();
  virtual void ChangeUserPass();
  virtual bool Download(const CFileItem *subItem,std::vector<std::string> &items);
private:
  bool LogIn();
  std::string m_strToken;
  std::string m_strUser;
  std::string m_strPass;
  bool m_authenticated;
  
};
