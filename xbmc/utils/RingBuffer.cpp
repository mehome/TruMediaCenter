/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include "RingBuffer.h"
#include "threads/SingleLock.h"

#include <cstring>
#include <cstdlib>
#include <algorithm>

/* Constructor */
CRingBuffer::CRingBuffer()
{
  m_buffer = NULL;
  m_size = 0;
  m_readPtr = 0;
  m_writePtr = 0;
  m_fillCount = 0;
}

/* Destructor */
CRingBuffer::~CRingBuffer()
{
  Destroy();
}

/* Create a ring buffer with the specified 'size' */
bool CRingBuffer::Create(size_t size)
{
  CSingleLock lock(m_critSection);
  m_buffer = (char*)malloc(size);
  if (m_buffer != nullptr)
  {
    m_size = size;
    return true;
  }
  return false;
}

/* Free the ring buffer and set all values to NULL or 0 */
void CRingBuffer::Destroy()
{
  CSingleLock lock(m_critSection);
  if (m_buffer != nullptr)
  {
    free(m_buffer);
    m_buffer = nullptr;
  }
  m_size = 0;
  m_readPtr = 0;
  m_writePtr = 0;
  m_fillCount = 0;
}

/* Clear the ring buffer */
void CRingBuffer::Clear()
{
  CSingleLock lock(m_critSection);
  m_readPtr = 0;
  m_writePtr = 0;
  m_fillCount = 0;
}

/* Read in data from the ring buffer to the supplied buffer 'buf'. The amount
 * read in is specified by 'size'.
 */
bool CRingBuffer::ReadData(char *buf, size_t size)
{
  CSingleLock lock(m_critSection);
  if (size > m_fillCount)
  {
    return false;
  }
  if (size + m_readPtr > m_size)
  {
    size_t chunk = m_size - m_readPtr;
    memcpy(buf, m_buffer + m_readPtr, chunk);
    memcpy(buf + chunk, m_buffer, size - chunk);
    m_readPtr = size - chunk;
  }
  else
  {
    memcpy(buf, m_buffer + m_readPtr, size);
    m_readPtr += size;
  }
  if (m_readPtr == m_size)
    m_readPtr = 0;
  m_fillCount -= size;
  return true;
}

/* Read in data from the ring buffer to another ring buffer object specified by
 * 'rBuf'.
 */
bool CRingBuffer::ReadData(CRingBuffer &rBuf, size_t size)
{
  CSingleLock lock(m_critSection);
  if (rBuf.getBuffer() == nullptr)
    rBuf.Create(size);

  bool bOk = size <= rBuf.getMaxWriteSize() && size <= getMaxReadSize();
  if (bOk)
  {
    size_t chunksize = std::min(size, m_size - m_readPtr);
    bOk = rBuf.WriteData(&getBuffer()[m_readPtr], chunksize);
    if (bOk && chunksize < size)
      bOk = rBuf.WriteData(&getBuffer()[0], size - chunksize);
    if (bOk)
      SkipBytes(size);
  }

  return bOk;
}

/* Write data to ring buffer from buffer specified in 'buf'. Amount read in is
 * specified by 'size'.
 */
bool CRingBuffer::WriteData(const char *buf, size_t size)
{
  CSingleLock lock(m_critSection);
  if (size > m_size - m_fillCount)
  {
    return false;
  }
  if (size + m_writePtr > m_size)
  {
    size_t chunk = m_size - m_writePtr;
    memcpy(m_buffer + m_writePtr, buf, chunk);
    memcpy(m_buffer, buf + chunk, size - chunk);
    m_writePtr = size - chunk;
  }
  else
  {
    memcpy(m_buffer + m_writePtr, buf, size);
    m_writePtr += size;
  }
  if (m_writePtr == m_size)
    m_writePtr = 0;
  m_fillCount += size;
  return true;
}

/* Write data to ring buffer from another ring buffer object specified by
 * 'rBuf'.
 */
bool CRingBuffer::WriteData(CRingBuffer &rBuf, size_t size)
{
  CSingleLock lock(m_critSection);
  if (m_buffer == nullptr)
    Create(size);

  bool bOk = size <= rBuf.getMaxReadSize() && size <= getMaxWriteSize();
  if (bOk)
  {
    size_t readpos = rBuf.getReadPtr();
    size_t chunksize = std::min(size, rBuf.getSize() - readpos);
    bOk = WriteData(&rBuf.getBuffer()[readpos], chunksize);
    if (bOk && chunksize < size)
      bOk = WriteData(&rBuf.getBuffer()[0], size - chunksize);
  }

  return bOk;
}

/* Skip bytes in buffer to be read */
bool CRingBuffer::SkipBytes(long skipSize)
{
  CSingleLock lock(m_critSection);
  if (skipSize < 0)
  {
    return false; // skipping backwards is not supported
  }

  size_t size = skipSize;
  if (size > m_fillCount)
  {
    return false;
  }
  if (size + m_readPtr > m_size)
  {
    size_t chunk = m_size - m_readPtr;
    m_readPtr = size - chunk;
  }
  else
  {
    m_readPtr += size;
  }
  if (m_readPtr == m_size)
    m_readPtr = 0;
  m_fillCount -= size;
  return true;
}

/* Append all content from ring buffer 'rBuf' to this ring buffer */
bool CRingBuffer::Append(CRingBuffer &rBuf)
{
  return WriteData(rBuf, rBuf.getMaxReadSize());
}

/* Copy all content from ring buffer 'rBuf' to this ring buffer overwriting any existing data */
bool CRingBuffer::Copy(CRingBuffer &rBuf)
{
  Clear();
  return Append(rBuf);
}

/* Our various 'get' methods */
char *CRingBuffer::getBuffer()
{
  return m_buffer;
}

size_t CRingBuffer::getSize()
{
  CSingleLock lock(m_critSection);
  return m_size;
}

size_t CRingBuffer::getReadPtr() const
{
  return m_readPtr;
}

size_t CRingBuffer::getWritePtr()
{
  CSingleLock lock(m_critSection);
  return m_writePtr;
}

size_t CRingBuffer::getMaxReadSize()
{
  CSingleLock lock(m_critSection);
  return m_fillCount;
}

size_t CRingBuffer::getMaxWriteSize()
{
  CSingleLock lock(m_critSection);
  return m_size - m_fillCount;
}
