/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ltp-queue-base.h"

NS_LOG_COMPONENT_DEFINE ("LTPQueueBase");

namespace ns3 {
namespace ltp {
NS_OBJECT_ENSURE_REGISTERED (LTPQueueBase);

TypeId 
LTPQueueBase::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LTPQueueBase")
    .SetParent<Object> ()
	.AddTraceSource ("Enqueue", "Enqueue a packet in the queue.",
	                  MakeTraceSourceAccessor (&LTPQueueBase::m_traceEnqueue),
	                  "ns3::Packet::TracedCallback")
	    .AddTraceSource ("Dequeue", "Dequeue a packet from the queue.",
	                  MakeTraceSourceAccessor (&LTPQueueBase::m_traceDequeue),
	                  "ns3::Packet::TracedCallback")
	    .AddTraceSource ("Drop", "Drop a packet stored in the queue.",
	                  MakeTraceSourceAccessor (&LTPQueueBase::m_traceDrop),
	                  "ns3::Packet::TracedCallback")
	  ;
  return tid;
}

LTPQueueBase::LTPQueueBase() :
  m_nBytes (0),
  m_nTotalReceivedBytes (0),
  m_nPackets (0),
  m_nTotalReceivedPackets (0),
  m_nTotalDroppedBytes (0),
  m_nTotalDroppedPackets (0)
{
  NS_LOG_FUNCTION (this);
}

LTPQueueBase::~LTPQueueBase()
{
  NS_LOG_FUNCTION (this);
}


bool 
LTPQueueBase::Enqueue (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  //
  // If DoEnqueue fails, Queue::Drop is called by the subclass
  //
  bool retval = DoEnqueue (p);
  if (retval)
    {
      NS_LOG_LOGIC ("m_traceEnqueue (p)");
      m_traceEnqueue (p);

      uint32_t size = p->GetSize ();
      m_nBytes += size;
      m_nTotalReceivedBytes += size;

      m_nPackets++;
      m_nTotalReceivedPackets++;
    }
  return retval;
}

Ptr<Packet>
LTPQueueBase::Dequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<Packet> packet = DoDequeue ();

  if (packet != 0)
    {
      NS_ASSERT (m_nBytes >= packet->GetSize ());
      NS_ASSERT (m_nPackets > 0);

      m_nBytes -= packet->GetSize ();
      m_nPackets--;

      NS_LOG_LOGIC ("m_traceDequeue (packet)");
      m_traceDequeue (packet);
    }
  return packet;
}

void
LTPQueueBase::DequeueAll (void)
{
  NS_LOG_FUNCTION (this);
  while (!IsEmpty ())
    {
      Dequeue ();
    }
}

Ptr<const Packet>
LTPQueueBase::Peek (void) const
{
  NS_LOG_FUNCTION (this);
  return DoPeek ();
}


uint32_t 
LTPQueueBase::GetNPackets (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("returns " << m_nPackets);
  return m_nPackets;
}

uint32_t
LTPQueueBase::GetNBytes (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (" returns " << m_nBytes);
  return m_nBytes;
}

bool
LTPQueueBase::IsEmpty (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("returns " << (m_nPackets == 0));
  return m_nPackets == 0;
}

uint32_t
LTPQueueBase::GetTotalReceivedBytes (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("returns " << m_nTotalReceivedBytes);
  return m_nTotalReceivedBytes;
}

uint32_t
LTPQueueBase::GetTotalReceivedPackets (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("returns " << m_nTotalReceivedPackets);
  return m_nTotalReceivedPackets;
}

uint32_t
LTPQueueBase:: GetTotalDroppedBytes (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("returns " << m_nTotalDroppedBytes);
  return m_nTotalDroppedBytes;
}

uint32_t
LTPQueueBase::GetTotalDroppedPackets (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("returns " << m_nTotalDroppedPackets);
  return m_nTotalDroppedPackets;
}

void 
LTPQueueBase::ResetStatistics (void)
{
  NS_LOG_FUNCTION (this);
  m_nTotalReceivedBytes = 0;
  m_nTotalReceivedPackets = 0;
  m_nTotalDroppedBytes = 0;
  m_nTotalDroppedPackets = 0;
}

void
LTPQueueBase::Drop (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  m_nTotalDroppedPackets++;
  m_nTotalDroppedBytes += p->GetSize ();

  NS_LOG_LOGIC ("m_traceDrop (p)");
  m_traceDrop (p);
}

}
} // namespace ns3
