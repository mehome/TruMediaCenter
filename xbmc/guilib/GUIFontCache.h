/*!
\file GUIFontCache.h
\brief
*/

#ifndef CGUILIB_GUIFONTCACHE_H
#define CGUILIB_GUIFONTCACHE_H
#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <cstddef>
#include <cstring>
#include <stdint.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <list>
#include <cassert>

#include "TransformMatrix.h"

#define FONT_CACHE_TIME_LIMIT (1000)
#define FONT_CACHE_DIST_LIMIT (0.01f)

template<class Position, class Value> class CGUIFontCache;
class CGUIFontTTFBase;

template<class Position, class Value>
class CGUIFontCacheImpl;

template<class Position>
struct CGUIFontCacheKey
{
  Position m_pos;
  vecColors &m_colors;
  vecText &m_text;
  uint32_t m_alignment;
  float m_maxPixelWidth;
  bool m_scrolling;
  const TransformMatrix &m_matrix;
  float m_scaleX;
  float m_scaleY;

  CGUIFontCacheKey(Position pos,
                   vecColors &colors, vecText &text,
                   uint32_t alignment, float maxPixelWidth,
                   bool scrolling, const TransformMatrix &matrix,
                   float scaleX, float scaleY) :
    m_pos(pos),
    m_colors(colors), m_text(text),
    m_alignment(alignment), m_maxPixelWidth(maxPixelWidth),
    m_scrolling(scrolling), m_matrix(matrix),
    m_scaleX(scaleX), m_scaleY(scaleY)
  {}
};

template<class Position, class Value>
struct CGUIFontCacheEntry
{
  CGUIFontCacheKey<Position> m_key;
  TransformMatrix m_matrix;

  unsigned int m_lastUsedMillis;
  Value m_value;

  CGUIFontCacheEntry(const CGUIFontCacheKey<Position> &key, unsigned int nowMillis) :
    m_key(key.m_pos,
          *new vecColors, *new vecText,
          key.m_alignment, key.m_maxPixelWidth,
          key.m_scrolling, m_matrix,
          key.m_scaleX, key.m_scaleY),
    m_lastUsedMillis(nowMillis)
  {
    m_key.m_colors.assign(key.m_colors.begin(), key.m_colors.end());
    m_key.m_text.assign(key.m_text.begin(), key.m_text.end());
    m_matrix = key.m_matrix;
  }

  CGUIFontCacheEntry(const CGUIFontCacheEntry &other) :
    m_key(other.m_key.m_pos,
          *new vecColors, *new vecText,
          other.m_key.m_alignment, other.m_key.m_maxPixelWidth,
          other.m_key.m_scrolling, m_matrix,
          other.m_key.m_scaleX, other.m_key.m_scaleY),
    m_lastUsedMillis(other.m_lastUsedMillis),
    m_value(other.m_value)
  {
    m_key.m_colors.assign(other.m_key.m_colors.begin(), other.m_key.m_colors.end());
    m_key.m_text.assign(other.m_key.m_text.begin(), other.m_key.m_text.end());
    m_matrix = other.m_key.m_matrix;
  }

  ~CGUIFontCacheEntry();
};

template<class Position, class Value>
class CGUIFontCache
{
  CGUIFontCacheImpl<Position, Value>* m_impl;

public:
  const CGUIFontTTFBase &m_font;

  CGUIFontCache(CGUIFontTTFBase &font);

  ~CGUIFontCache();
 
  Value &Lookup(Position &pos,
                const vecColors &colors, const vecText &text,
                uint32_t alignment, float maxPixelWidth,
                bool scrolling,
                unsigned int nowMillis, bool &dirtyCache);
  void Flush();
};

struct CGUIFontCacheStaticPosition
{
  float m_x;
  float m_y;
  CGUIFontCacheStaticPosition(float x, float y) : m_x(x), m_y(y) {}
  void UpdateWithOffsets(const CGUIFontCacheStaticPosition &cached, bool scrolling) {}
};

struct CGUIFontCacheStaticValue : public std::shared_ptr<std::vector<SVertex> >
{
  void clear()
  {
    if (*this)
      (*this)->clear();
  }
};

struct CGUIFontCacheDynamicPosition
{
  float m_x;
  float m_y;
  float m_z;
  CGUIFontCacheDynamicPosition() {}
  CGUIFontCacheDynamicPosition(float x, float y, float z) : m_x(x), m_y(y), m_z(z) {}
  void UpdateWithOffsets(const CGUIFontCacheDynamicPosition &cached, bool scrolling)
  {
    if (scrolling)
      m_x = m_x - cached.m_x;
    else
      m_x = floorf(m_x - cached.m_x + FONT_CACHE_DIST_LIMIT);
    m_y = floorf(m_y - cached.m_y + FONT_CACHE_DIST_LIMIT);
    m_z = floorf(m_z - cached.m_z + FONT_CACHE_DIST_LIMIT);
  }
};

struct CVertexBuffer
{
  unsigned int bufferHandle; // this is really a GLuint
  size_t size;
  CVertexBuffer() : bufferHandle(NULL), size(0), m_font(NULL) {}
  CVertexBuffer(unsigned int bufferHandle, size_t size, const CGUIFontTTFBase *font) : bufferHandle(bufferHandle), size(size), m_font(font) {}
  CVertexBuffer(const CVertexBuffer &other) : bufferHandle(other.bufferHandle), size(other.size), m_font(other.m_font)
  {
    /* In practice, the copy constructor is only called before a vertex buffer
     * has been attached. If this should ever change, we'll need another support
     * function in GUIFontTTFGL/DX to duplicate a buffer, given its handle. */
    assert(other.bufferHandle == 0);
  }
  CVertexBuffer &operator=(CVertexBuffer &other)
  {
    /* This is used with move-assignment semantics for initialising the object in the font cache */
    assert(bufferHandle == 0);
    bufferHandle = other.bufferHandle;
    other.bufferHandle = 0;
    size = other.size;
    m_font = other.m_font;
    return *this;
  }
  void clear();
private:
  const CGUIFontTTFBase *m_font;
};

typedef CVertexBuffer CGUIFontCacheDynamicValue;
#endif
