/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010-2015 Adrian Sai-wah Tam
 * Copyright (c) 2016 Natale Patriciello <natale.patriciello@gmail.com>
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
 * Original author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#include <algorithm>
#include <iostream>

#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/tcp-option-ts.h"

#include"scpstp-tx-buffer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ScpsTpTxBuffer");
NS_OBJECT_ENSURE_REGISTERED (ScpsTpTxBuffer);

TypeId
ScpsTpTxBuffer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ScpsTpTxBuffer")
    .SetParent<TcpTxBuffer> ()
    .SetGroupName ("Internet")
    .AddConstructor<ScpsTpTxBuffer> ()
  ;
  return tid;
}

ScpsTpTxBuffer::ScpsTpTxBuffer (uint32_t n)
  : TcpTxBuffer (n)
{
}

ScpsTpTxBuffer::ScpsTpTxBuffer (const TcpTxBuffer &buffer)
  : TcpTxBuffer (buffer)
{
}
ScpsTpTxBuffer::~ScpsTpTxBuffer (void)
{
}

void
ScpsTpTxBuffer::UpdateSnackedData (const ScpsTpOptionSnack::SnackList &snackList)
{
  NS_LOG_FUNCTION (this << &snackList);
  for (ScpsTpOptionSnack::SnackList::const_iterator i = snackList.begin ();
       i != snackList.end (); ++i)
    {
      SequenceNumber32 startSeq = i->first;
      SequenceNumber32 endSeq = i->second;
      bool listEdited = false;
      NS_LOG_INFO ("Marking packets from " << startSeq << " to " << endSeq << " as lost");
      MarkLostPacketsInRange (m_sentList,m_firstByteSeq,startSeq,endSeq,&listEdited);
    }

  ConsistencyCheck();
}



void
ScpsTpTxBuffer::MergeItems (TcpTxItem *t1, TcpTxItem *t2) const
{
  NS_ASSERT (t1 != nullptr && t2 != nullptr);
  NS_LOG_FUNCTION (this << *t1 << *t2);
  NS_LOG_INFO ("Merging " << *t2 << " into " << *t1);

  // 这种情况目前出现在SNACK后标记列表内容为丢失时，与RenoSACK冲突，暂时按照SNACK的逻辑将其标为丢失
  if(t1->m_sacked != t2->m_sacked)
    {
      NS_LOG_INFO ("Merging one sacked and another not sacked.");
      if (t1->m_sacked)
        {
          ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
          self->m_sackedOut -= t1->m_packet->GetSize ();
          t1->m_retrans = false;
        }
      else
        {
          NS_ASSERT (t2->m_sacked);
          ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
          self->m_sackedOut -= t2->m_packet->GetSize ();
          t2->m_sacked = false;
        }

    }
  //当delayACK到达时，已经被标记为lost的包重传还没有被接收，这个ACK会包含此时会出现
  //其中一个被标记为丢失，另一个没有被标记为丢失，正常现象
  if(t1->m_lost != t2->m_lost)
    {
      NS_LOG_INFO ("Merging one lost and another not lost.");
      if (!t1->m_lost)
        {
          ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
          self->m_lostOut += t1->m_packet->GetSize ();
          t1->m_lost = true;
        }
      else
        {
          ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
          self->m_lostOut += t2->m_packet->GetSize ();
          t2->m_lost = true;
        }
    }
  // If one is retrans and the other is not, cancel the retransmitted flag.
  // We are merging this segment for the retransmit, so the count will
  // be updated in MarkTransmittedSegment.
  if (t1->m_retrans != t2->m_retrans)
    {
      if (t1->m_retrans)
        {
          ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
          self->m_retrans -= t1->m_packet->GetSize ();
          t1->m_retrans = false;
        }
      else
        {
          NS_ASSERT (t2->m_retrans);
          ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
          self->m_retrans -= t2->m_packet->GetSize ();
          t2->m_retrans = false;
        }
    }

  if (t1->m_lastSent < t2->m_lastSent)
    {
      t1->m_lastSent = t2->m_lastSent;
    }

  t1->m_packet->AddAtEnd (t2->m_packet);

  NS_LOG_INFO ("Situation after the merge: " << *t1);
}



void
ScpsTpTxBuffer::MarkLostPacketsInRange (PacketList &list, const SequenceNumber32 &listStartFrom, const SequenceNumber32 &startSeq, 
                                         const SequenceNumber32 &endSeq, bool *listEdited) const
{
  NS_LOG_FUNCTION (this << startSeq << endSeq);

  Ptr<Packet> currentPacket = nullptr;
  TcpTxItem *currentItem = nullptr;
  PacketList::iterator it = list.begin ();
  SequenceNumber32 beginOfCurrentPacket = listStartFrom;

  while (it != list.end ())
    {
      currentItem = *it;
      currentPacket = currentItem->m_packet;
      NS_ASSERT_MSG (list != m_sentList || currentItem->m_startSeq >= m_firstByteSeq,
                     "start: " << m_firstByteSeq << " currentItem start: " << currentItem->m_startSeq);

      SequenceNumber32 packetStart = beginOfCurrentPacket;
      SequenceNumber32 packetEnd = beginOfCurrentPacket + currentPacket->GetSize ();

      // 检查数据包是否在指定范围内
      if (startSeq < packetEnd && endSeq > packetStart)
        {
          // 包完全在范围内
          if (startSeq <= packetStart && endSeq >= packetEnd)
            {
              NS_LOG_DEBUG ("Marking the entire packet lost from seq " << packetStart << " to " << packetEnd);
              if (!currentItem->m_lost)
                {
                  currentItem->m_lost = true;

                  ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
                  self->m_lostOut += currentPacket->GetSize ();
                  if (listEdited)
                    {
                      *listEdited = true;
                    }
                }
              if(currentItem->m_sacked)
                {
                  currentItem->m_sacked = false;
                  ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
                  self->m_sackedOut -= currentPacket->GetSize ();
                  if (listEdited)
                    {
                      *listEdited = true;
                    }
                }
                // 移动到下一个包
              beginOfCurrentPacket += currentPacket->GetSize ();
              ++it;
            }
          // 包部分在范围内，需要分割
          else
            {
              // 提前更新beginOfCurrentPacket
              beginOfCurrentPacket += currentPacket->GetSize ();
              if (startSeq > packetStart)
                {
                  // 分割前半部分
                  NS_LOG_INFO ("Splitting packet at the beginning for seq " << startSeq);
                  TcpTxItem *firstPart = new TcpTxItem ();
                  SplitItems (firstPart, currentItem, startSeq - packetStart);
                  list.insert (it, firstPart);

                  // 标记当前包
                  if (!currentItem->m_lost)
                    {
                      currentItem->m_lost = true;
                      ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
                      self->m_lostOut += currentPacket->GetSize ();
                      if (listEdited)
                        {
                          *listEdited = true;
                        }
                    }
                  if(currentItem->m_sacked)
                    {
                      currentItem->m_sacked = false;
                      ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
                      self->m_sackedOut -= currentPacket->GetSize ();
                      if (listEdited)
                        {
                          *listEdited = true;
                        }
                    }                    
                  
                  if (listEdited)
                    {
                      *listEdited = true;
                    }
                }

              if (endSeq < packetEnd)
                {
                  // 分割后半部分
                  NS_LOG_DEBUG ("Splitting packet at the end for seq " << endSeq);
                  TcpTxItem *firstPart = new TcpTxItem ();
                  SplitItems (firstPart, currentItem, endSeq - packetStart);  // 先创建第一部分
                  list.insert (it, firstPart);  // 插入第一部分

                  // 标记
                  if (!firstPart->m_lost)
                    {
                      firstPart->m_lost = true;
                      ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
                      self->m_lostOut += firstPart->m_packet->GetSize ();
                      if (listEdited)
                        {
                          *listEdited = true;
                        }
                    }
                  if (firstPart->m_sacked)
                    {
                      firstPart->m_sacked = false;
                      ScpsTpTxBuffer *self = const_cast<ScpsTpTxBuffer*> (this);
                      self->m_sackedOut -= firstPart->m_packet->GetSize ();
                      if (listEdited)
                        {
                          *listEdited = true;
                        }
                    }

                  if (listEdited)
                    {
                      *listEdited = true;
                    }
                }
              ++it;

            }
        }
      else
        {
          // 移动到下一个包
          beginOfCurrentPacket += currentPacket->GetSize ();
          ++it;
        }

      // 如果当前包的结束位置已经超过范围，退出循环
      if (beginOfCurrentPacket >= endSeq)
        {
          break;
        }
    }

  NS_LOG_DEBUG ("Finished marking lost packets in range.");
}


} // namespace ns3