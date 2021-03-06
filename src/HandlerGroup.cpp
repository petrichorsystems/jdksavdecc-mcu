/*
  Copyright (c) 2014, J.D. Koftinoff Software, Ltd.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of J.D. Koftinoff Software, Ltd. nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

#include "JDKSAvdeccMCU/World.hpp"
#include "JDKSAvdeccMCU/HandlerGroup.hpp"

namespace JDKSAvdeccMCU
{

HandlerGroup::HandlerGroup( Frame *frame, Handler **item_storage, uint16_t max_items )
    : m_num_items( 0 ), m_max_items( max_items ), m_item( item_storage ), m_rx_count( 0 ), m_handled_count( 0 ), m_frame( frame )
{
}

bool HandlerGroup::add( Handler *v )
{
    bool r = false;
    if ( m_num_items < m_max_items )
    {
        m_item[m_num_items++] = v;
        r = true;
    }
    return r;
}

/// Send Tick() messages to all encapsulated Handlers
/// and poll incoming network for PDU's and dispatch them
void HandlerGroup::tick( jdksavdecc_timestamp_in_milliseconds time_in_millis )
{
    // TODO:
}

/// Send ReceivedPDU message to each handler until one returns true.
bool HandlerGroup::receivedPDU( RawSocket *incoming_socket, Frame &frame )
{
    bool r = false;
    for ( uint16_t i = 0; i < m_num_items; ++i )
    {
        if ( m_item[i]->receivedPDU( incoming_socket, frame ) )
        {
            r = true;
            break;
        }
    }
    return r;
}
}
