/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Adrian Sai-wah Tam
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#include "ns3/packet.h"
#include "ns3/log.h"
#include "scpstp-rx-buffer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ScpsTpRxBuffer");

NS_OBJECT_ENSURE_REGISTERED (ScpsTpRxBuffer);

TypeId
ScpsTpRxBuffer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ScpsTpRxBuffer")
    .SetParent<TcpRxBuffer> ()
    .SetGroupName ("Internet")
    .AddConstructor<ScpsTpRxBuffer> ()
  ;
  return tid;
}

ScpsTpRxBuffer::ScpsTpRxBuffer (uint32_t n)
  : TcpRxBuffer (n)
{
}

ScpsTpRxBuffer::~ScpsTpRxBuffer ()
{
}

ScpsTpRxBuffer::ScpsTpRxBuffer (const TcpRxBuffer &buffer)
  : TcpRxBuffer (buffer)
{
}
ScpsTpOptionSnack::SnackList
ScpsTpRxBuffer::GetSnackList (void) const
{
  return m_snackList;
}

uint32_t
ScpsTpRxBuffer::GetSnackListSize (void) const
{
  return static_cast<uint32_t> (m_snackList.size ());
}



void
ScpsTpRxBuffer::UpdateSackList (const SequenceNumber32 &head, const SequenceNumber32 &tail)
{
  NS_LOG_FUNCTION (this << head << tail);
  NS_ASSERT (head > m_nextRxSeq);

  TcpOptionSack::SackBlock current;
  current.first = head;
  current.second = tail;

  // The block "current" has been safely stored. Now we need to build the SACK
  // list, to be advertised. From RFC 2018:
  // (a) The first SACK block (i.e., the one immediately following the
  //     kind and length fields in the option) MUST specify the contiguous
  //     block of data containing the segment which triggered this ACK,
  //     unless that segment advanced the Acknowledgment Number field in
  //     the header.  This assures that the ACK with the SACK option
  //     reflects the most recent change in the data receiver's buffer
  //     queue.
  //
  // (b) The data receiver SHOULD include as many distinct SACK blocks as
  //     possible in the SACK option.  Note that the maximum available
  //     option space may not be sufficient to report all blocks present in
  //     the receiver's queue.
  //
  // (c) The SACK option SHOULD be filled out by repeating the most
  //     recently reported SACK blocks (based on first SACK blocks in
  //     previous SACK options) that are not subsets of a SACK block
  //     already included in the SACK option being constructed.  This
  //     assures that in normal operation, any segment remaining part of a
  //     non-contiguous block of data held by the data receiver is reported
  //     in at least three successive SACK options, even for large-window
  //     TCP implementations [RFC1323]).  After the first SACK block, the
  //     following SACK blocks in the SACK option may be listed in
  //     arbitrary order.

  m_sackList.push_front (current);

  // We have inserted the block at the beginning of the list. Now, we should
  // check if any existing blocks overlap with that.
  bool updated = false;
  TcpOptionSack::SackList::iterator it = m_sackList.begin ();
  TcpOptionSack::SackBlock begin = *it;
  TcpOptionSack::SackBlock merged;
  ++it;

  // Iterates until we examined all blocks in the list (maximum 4)
  while (it != m_sackList.end ())
    {
      current = *it;

      // This is a left merge:
      // [current_first; current_second] [beg_first; beg_second]
      if (begin.first == current.second)
        {
          NS_ASSERT (current.first < begin.second);
          merged = TcpOptionSack::SackBlock (current.first, begin.second);
          updated = true;
        }
      // while this is a right merge
      // [begin_first; begin_second] [current_first; current_second]
      else if (begin.second == current.first)
        {
          NS_ASSERT (begin.first < current.second);
          merged = TcpOptionSack::SackBlock (begin.first, current.second);
          updated = true;
        }

      // If we have merged the blocks (and the result is in merged) we should
      // delete the current block (it), the first block, and insert the merged
      // one at the beginning.
      if (updated)
        {
          m_sackList.erase (it);
          m_sackList.pop_front ();
          m_sackList.push_front (merged);
          it = m_sackList.begin ();
          begin = *it;
          updated = false;
        }

      ++it;
    }

  // Since the maximum blocks that fits into a TCP header are 4, there's no
  // point on maintaining the others.
  //但是为了给snack list提供更准确的信息，这里有必要保留更多的sack block
  /*
  if (m_sackList.size () > 4)
    {
      m_sackList.pop_back ();
    }
  */
  // Please note that, if a block b is discarded and then a block contiguous
  // to b is received, only that new block (without the b part) is reported.
  // This is perfectly fine for the RFC point (a), given that we do not report any
  // overlapping blocks shortly after.

  UpdateSnackList();
}

void
ScpsTpRxBuffer::ClearSackList (const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this << seq);

  TcpOptionSack::SackList::iterator it;
  for (it = m_sackList.begin (); it != m_sackList.end (); )
    {
      TcpOptionSack::SackBlock block = *it;
      NS_ASSERT (block.first < block.second);

      if (block.second <= seq)
        {
          it = m_sackList.erase (it);
        }
      else
        {
          it++;
        }
    }
  ClearSnackList(seq);
}

void
ScpsTpRxBuffer::UpdateSnackList(void)
{

 // 清空原有的 snackList
  m_snackList.clear();

  // 创建 sackList 的副本并进行排序
  TcpOptionSack::SackList sackListCopy = m_sackList; 
  if(sackListCopy.size() > 1) {
    sackListCopy.sort([](const TcpOptionSack::SackBlock& a, const TcpOptionSack::SackBlock& b) {
    return a.first > b.first;  
    });
  }


  // 使用排序后的副本生成 snackHole
  TcpOptionSack::SackList::iterator it = sackListCopy.begin();
  TcpOptionSack::SackBlock begin = *it;
  ScpsTpOptionSnack::SnackHole hole;
  ++it;

  while (it != sackListCopy.end()) {
      TcpOptionSack::SackBlock next = *it;
      hole = ScpsTpOptionSnack::SnackHole(next.second, begin.first);
      m_snackList.push_front(hole);
      begin = *it;
      ++it;
  }

  // 生成最后一个 hole
  hole = ScpsTpOptionSnack::SnackHole(m_nextRxSeq.Get(), begin.first);
  m_snackList.push_front(hole);


  //调试信息：打印snack list和sack list
  /*
  NS_LOG_INFO("UpdateSnackList:");
  NS_LOG_INFO("sequence number:" << m_nextRxSeq.Get());
  NS_LOG_INFO("Sack list:");
  for (TcpOptionSack::SackList::iterator it = m_sackList.begin(); it != m_sackList.end(); ++it)
  {
      NS_LOG_INFO("[" << it->first << ";" << it->second << "]");
  }
  NS_LOG_INFO("Snack list:");
  for (ScpsTpOptionSnack::SnackList::iterator it = m_snackList.begin(); it != m_snackList.end(); ++it)
  {
      NS_LOG_INFO("[" << it->first << ";" << it->second << "]");
  }*/
}

void
ScpsTpRxBuffer::ClearSnackList(const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this << seq);
  ScpsTpOptionSnack::SnackList::iterator it;
  for(it = m_snackList.begin(); it != m_snackList.end(); )
  {
    ScpsTpOptionSnack::SnackHole &hole = *it;//hole是引用，通过修改hole改变m_snackList中的值
    if (hole.second <= seq) {
        it = m_snackList.erase(it);
    }
    else if ((hole.first < seq) && (seq < hole.second)) {
        hole.first = seq;
        ++it;
    }
    else {
        ++it;
    }
  }
  /*
  //调试信息：打印snack list和seq
  NS_LOG_INFO("ClearSnackList:");
  NS_LOG_INFO("seq:" << seq);
  NS_LOG_INFO("Snack list:");
  for (ScpsTpOptionSnack::SnackList::iterator it = m_snackList.begin(); it != m_snackList.end(); ++it)
  {
      NS_LOG_INFO("[" << it->first << ";" << it->second << "]");
  }*/
}

} // namespace ns3