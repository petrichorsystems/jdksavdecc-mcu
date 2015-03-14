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
#include "JDKSAvdeccMCU/Aps.hpp"

namespace JDKSAvdeccMCU
{

ApsStateMachine::ApsStateMachine( ApsStateMachine::StateVariables *variables,
                                  ApsStateMachine::StateActions *actions,
                                  ApsStateMachine::StateEvents *events,
                                  ApsStateMachine::States *states,
                                  uint16_t &active_entity_id_count,
                                  active_connections_type &active_connections )
    : m_variables( variables )
    , m_actions( actions )
    , m_events( events )
    , m_states( states )
    , m_active_entity_id_count( active_entity_id_count )
    , m_active_connections( active_connections )
{
}

ApsStateMachine::~ApsStateMachine() {}

void ApsStateMachine::setup() { clear(); }

void ApsStateMachine::setLinkMac( Eui48 mac )
{
    getVariables()->m_linkMac = mac;
    getVariables()->m_linkStatus = true;
}

bool ApsStateMachine::run() { return getStates()->run(); }

void ApsStateMachine::onIncomingTcpConnection()
{
    getEvents()->onIncomingTcpConnection();
}

void ApsStateMachine::onTcpConnectionClosed()
{
    m_events->onTcpConnectionClosed();
}

void ApsStateMachine::onTimeTick( uint32_t time_in_seconds )
{
    getEvents()->onTimeTick( time_in_seconds );
}

void ApsStateMachine::closeTcpConnection()
{
    m_active_connections.erase( m_assigned_count );
}

void ApsStateMachine::closeTcpServer() {}

ssize_t ApsStateMachine::onIncomingTcpData( const uint8_t *data, ssize_t len )
{
    return m_events->onIncomingTcpData( data, len );
}

void ApsStateMachine::sendTcpData( const uint8_t *data, ssize_t len ) {}

void ApsStateMachine::sendAvdeccToL2( Frame const &frame ) {}

void ApsStateMachine::onNetAvdeccMessageReceived( const Frame &frame )
{
    getEvents()->onNetAvdeccMessageReceived( frame );
}

void ApsStateMachine::onNetLinkStatusUpdated( Eui48 link_mac, bool link_status )
{
    getEvents()->onNetLinkStatusUpdated( link_mac, link_status );
}

Eui64 ApsStateMachine::assignEntityId( Eui48 server_link_mac,
                                       Eui48 apc_link_mac,
                                       Eui64 requested_entity_id )
{
    // increase our count of assigned entity_id's
    ++m_active_entity_id_count;

    // loop and increment while this count still active
    while ( m_active_connections.count( m_active_entity_id_count ) > 0 )
    {
        ++m_active_entity_id_count;
    }

    m_assigned_count = m_active_entity_id_count;
    m_active_connections.insert( m_assigned_count );

    Eui64 r;
    r.value[0] = server_link_mac.value[0];
    r.value[1] = server_link_mac.value[1];
    r.value[2] = server_link_mac.value[2];
    r.value[3] = uint8_t( ( m_active_entity_id_count >> 8 ) & 0xff );
    r.value[4] = uint8_t( ( m_active_entity_id_count >> 0 ) & 0xff );
    r.value[5] = server_link_mac.value[3];
    r.value[6] = server_link_mac.value[4];
    r.value[7] = server_link_mac.value[5];
    return r;
}

void ApsStateMachine::clear()
{
    m_variables->setOwner( this );
    m_actions->setOwner( this );
    m_events->setOwner( this );
    m_states->setOwner( this );

    m_variables->clear();
    m_actions->clear();
    m_events->clear();
    m_states->clear();
}

ApsStateMachine::StateEvents::StateEvents( HttpServerParser *http_parser,
                                           std::string path )
    : m_owner( 0 )
    , m_http_parser( http_parser )
    , m_path( path )
    , m_app_parser( *this )
{
}

void ApsStateMachine::StateEvents::clear()
{
    m_in_http = true;
    m_http_parser->clear();
    m_app_parser.clear();
}

void ApsStateMachine::StateEvents::sendTcpData( const uint8_t *data,
                                                ssize_t len )
{
    getOwner()->sendTcpData( data, len );
}

void ApsStateMachine::StateEvents::onIncomingTcpConnection()
{
    getVariables()->m_tcpConnected = true;
    m_in_http = true;
    m_http_parser->clear();
}

ssize_t ApsStateMachine::StateEvents::onIncomingTcpData( const uint8_t *data,
                                                         ssize_t len )
{
    ssize_t r = -1;
    if ( len > 0 )
    {
        getVariables()->m_tcpConnected = true;
    }
    if ( m_in_http )
    {
        r = onIncomingTcpHttpData( data, len );

        // If the http request was accepted
        if ( r >= 0 && r < len && m_in_http == false )
        {
            // and if there was additional data in the payload, then parse it
            // as APP message data
            ssize_t r1 = onIncomingTcpAppData( data + r, len - r );
            if ( r1 > 0 )
            {
                r += r1;
            }
            else
            {
                r = -1;
            }
        }
    }
    else
    {
        r = onIncomingTcpAppData( data, len );
    }
    return r;
}

ssize_t
    ApsStateMachine::StateEvents::onIncomingTcpHttpData( const uint8_t *data,
                                                         ssize_t len )
{
    return m_http_parser->onIncomingHttpData( data, len );
}

bool ApsStateMachine::StateEvents::onIncomingHttpConnectRequest(
    const HttpRequest &request )
{
    bool r = false;
    if ( request.m_path == m_path )
    {
        m_in_http = false;
        getVariables()->m_requestValid = 200;
        r = true;
    }
    return r;
}

ssize_t ApsStateMachine::StateEvents::onIncomingTcpAppData( const uint8_t *data,
                                                            ssize_t len )
{
    ssize_t r = 0;

    for ( ssize_t r = 0; r < len; ++r )
    {
        m_app_parser.parse( data[r] );
    }

    return r;
}

void ApsStateMachine::StateEvents::onTcpConnectionClosed()
{
    getVariables()->m_incomingTcpClosed = true;
}

void ApsStateMachine::StateEvents::onNetLinkStatusUpdated( Eui48 link_mac,
                                                           bool link_status )
{
    if ( getVariables()->m_linkStatus != link_status )
    {
        getVariables()->m_linkStatus = link_status;
        getVariables()->m_linkMac = link_mac;
        getVariables()->m_linkStatusChanged = true;
    }
}

void ApsStateMachine::StateEvents::onNetAvdeccMessageReceived(
    const Frame &frame )
{
    getVariables()->m_in.setAvdeccFromAps( frame );
    getVariables()->m_L2Msg = true;
}

void ApsStateMachine::StateEvents::onTimeTick( uint32_t time_in_seconds )
{
    getVariables()->m_currentTime = time_in_seconds;
}

void ApsStateMachine::StateEvents::onAppNop( const AppMessage &msg )
{
    // Do nothing
}

void ApsStateMachine::StateEvents::onAppEntityIdRequest( const AppMessage &msg )
{
    getVariables()->m_entity_id = msg.getEntityIdRequestEntityId();
    getVariables()->m_assignEntityIdRequest = true;

    // TODO: assign proper entity_id
}

void
    ApsStateMachine::StateEvents::onAppEntityIdResponse( const AppMessage &msg )
{
    // Do nothing
}

void ApsStateMachine::StateEvents::onAppLinkUp( const AppMessage &msg )
{
    // Do nothing
}

void ApsStateMachine::StateEvents::onAppLinkDown( const AppMessage &msg )
{
    // Do Nothing
}

void ApsStateMachine::StateEvents::onAppAvdeccFromAps( const AppMessage &msg )
{
    // Do Nothing
}

void ApsStateMachine::StateEvents::onAppAvdeccFromApc( const AppMessage &msg )
{
    getVariables()->m_out = msg;
    getVariables()->m_apcMsg = true;
}

void ApsStateMachine::StateEvents::onAppVendor( const AppMessage &msg )
{
    // Do nothing
}

void ApsStateMachine::StateEvents::onAppUnknown( const AppMessage &msg )
{
    // Do nothing
}

void ApsStateMachine::StateActions::initialize()
{
    StateVariables *v = getVariables();

    v->m_apcMsg = false;
    v->m_assignEntityIdRequest = false;
    v->m_currentTime = 0;
    v->m_finished = false;
    v->m_L2Msg = false;
    v->m_linkStatus = false;
    v->m_nopTimeout = 0;
    v->m_tcpConnected = false;
}

void ApsStateMachine::StateActions::sendHttpResponse( int requestValid )
{
    char statusbuf[16];
    HttpResponse response;

    if ( requestValid < 0 )
    {
        requestValid = 404;
        response.m_reason_phrase = "NOT FOUND";
    }
#if defined( _WIN32 )
    sprintf_s( statusbuf, sizeof( statusbuf ), "%d", requestValid );
#else
    sprintf( statusbuf, "%d", requestValid );
#endif
    response.m_version = "HTTP/1.1";
    response.m_status_code = statusbuf;
    response.m_reason_phrase = "OK";

    std::vector<uint8_t> buf;
    response.flatten( &buf );

    getEvents()->sendTcpData( buf.data(), buf.size() );
}

void ApsStateMachine::StateActions::sendMsgToApc( const AppMessage &apsMsg )
{
    FixedBufferWithSize<1500> msg_as_octets;
    if ( apsMsg.store( &msg_as_octets ) )
    {
        getEvents()->sendTcpData( msg_as_octets.getBuf(),
                                  msg_as_octets.getLength() );
    }
}

void ApsStateMachine::StateActions::sendLinkStatus( Eui48 link_mac,
                                                    bool linkStatus )
{
    AppMessage msg;
    if ( linkStatus )
    {
        msg.setLinkUp( link_mac );
    }
    else
    {
        msg.setLinkDown( link_mac );
    }
    sendMsgToApc( msg );
}

void ApsStateMachine::StateActions::sendAvdeccToL2( const AppMessage *msg )
{
    FrameWithMTU frame( 0,
                        msg->getAddress(),
                        getVariables()->m_linkMac,
                        JDKSAVDECC_AVTP_ETHERTYPE );

    frame.putBuf( msg->getPayload(), msg->getPayloadLength() );
    getOwner()->sendAvdeccToL2( frame );
}

void ApsStateMachine::StateActions::sendAvdeccToApc( const AppMessage *msg )
{
    sendMsgToApc( *msg );
}

void ApsStateMachine::StateActions::sendEntityIdAssignment( Eui48 a,
                                                            Eui64 entity_id )
{
    AppMessage msg;
    msg.setEntityIdResponse(
        a,
        getOwner()->assignEntityId( getVariables()->m_linkMac, a, entity_id ) );
    sendMsgToApc( msg );
}

void ApsStateMachine::StateActions::sendNopToApc()
{
    AppMessage msg;
    sendMsgToApc( msg );
}

void ApsStateMachine::StateActions::closeTcpConnection()
{
    getOwner()->closeTcpConnection();
}

void ApsStateMachine::StateActions::closeTcpServer()
{
    getOwner()->closeTcpServer();
}

ApsStateMachine::StateVariables::StateVariables() {}

ApsStateMachine::StateVariables::~StateVariables() {}

void ApsStateMachine::StateVariables::clear()
{
    m_finished = false;
    m_tcpConnected = false;
    m_requestValid = -1;
    m_incomingTcpClosed = false;
    m_linkStatus = false;
    m_linkStatusChanged = false;
    m_apcMsg = false;
    m_L2Msg = false;
    m_assignEntityIdRequest = false;
    m_nopTimeout = 0;
    m_currentTime = 0;
    m_out.setNOP();
    m_in.setNOP();
}

void ApsStateMachine::States::clear()
{
    m_current_state = &States::doBegin;

    getVariables()->m_tcpConnected = false;
    getVariables()->m_incomingTcpClosed = false;
    getVariables()->m_linkStatusChanged = false;
    getVariables()->m_apcMsg = false;
    getVariables()->m_L2Msg = false;
    getVariables()->m_assignEntityIdRequest = false;
}

bool ApsStateMachine::States::run()
{
    bool r = false;

    state_proc last_state;

    // call the current state function
    do
    {
        last_state = m_current_state;

        // only call state function if it is set
        if ( m_current_state )
        {
            ( this->*m_current_state )();
            r = true;
        }

        // if the current state transitioned, repeat
    } while ( last_state != m_current_state );

    // return true if there was activity,
    // return false if there is no state
    return r;
}

void ApsStateMachine::States::doBegin() { goToInitialize(); }

void ApsStateMachine::States::goToInitialize()
{
    m_current_state = &States::doInitialize;
    getActions()->initialize();
}

void ApsStateMachine::States::doInitialize() { goToWaitForConnect(); }

void ApsStateMachine::States::goToWaitForConnect()
{
    m_current_state = &States::doWaitForConnect;
    getVariables()->m_tcpConnected = false;
    getVariables()->m_incomingTcpClosed = false;
}

void ApsStateMachine::States::doWaitForConnect()
{
    if ( getVariables()->m_finished == true )
    {
        goToFinish();
    }
    if ( getVariables()->m_tcpConnected == true )
    {
        goToAccept();
    }
}

void ApsStateMachine::States::goToAccept()
{
    m_current_state = &States::doAccept;
    getVariables()->m_requestValid = -1;
}

void ApsStateMachine::States::doAccept()
{
    int requestValid = getVariables()->m_requestValid;

    if ( requestValid == 200 )
    {
        goToStartTransfer();
    }
    if ( requestValid != -1 && requestValid != 200 )
    {
        goToReject();
    }
}

void ApsStateMachine::States::goToReject()
{
    m_current_state = &States::doReject;
    getActions()->sendHttpResponse( getVariables()->m_requestValid );
}

void ApsStateMachine::States::doReject() { goToClosed(); }

void ApsStateMachine::States::goToClosed()
{
    m_current_state = &States::doClosed;
}

void ApsStateMachine::States::doClosed()
{
    getActions()->closeTcpConnection();
    goToWaitForConnect();
}

void ApsStateMachine::States::goToStartTransfer()
{
    m_current_state = &States::doStartTransfer;

    getActions()->sendHttpResponse( getVariables()->m_requestValid );

    getActions()->sendLinkStatus( getVariables()->m_linkMac,
                                  getVariables()->m_linkStatus );

    getVariables()->m_nopTimeout = getVariables()->m_currentTime + 10;
}

void ApsStateMachine::States::doStartTransfer() { goToWaiting(); }

void ApsStateMachine::States::goToWaiting()
{
    m_current_state = &States::doWaiting;
}

void ApsStateMachine::States::doWaiting()
{
    if ( getVariables()->m_incomingTcpClosed == true )
    {
        goToClosed();
    }
    else if ( getVariables()->m_finished == true )
    {
        goToCloseAndFinish();
    }
    else if ( getVariables()->m_linkStatusChanged == true )
    {
        goToLinkStatus();
    }
    else if ( getVariables()->m_apcMsg == true )
    {
        goToTransferToL2();
    }
    else if ( getVariables()->m_L2Msg == true )
    {
        goToTransferToApc();
    }
    else if ( getVariables()->m_assignEntityIdRequest == true )
    {
        goToAssignEntityId();
    }
    else if ( ( (int)getVariables()->m_nopTimeout
                - (int)getVariables()->m_currentTime ) < 0 )
    {
        goToSendNop();
    }
}

void ApsStateMachine::States::goToLinkStatus()
{
    m_current_state = &States::doLinkStatus;

    getActions()->sendLinkStatus( getVariables()->m_linkMac,
                                  getVariables()->m_linkStatus );

    getVariables()->m_nopTimeout = getVariables()->m_currentTime + 10;

    getVariables()->m_linkStatusChanged = false;
}

void ApsStateMachine::States::doLinkStatus() { goToWaiting(); }

void ApsStateMachine::States::goToTransferToL2()
{
    m_current_state = &States::doTransferToL2;
    getActions()->sendAvdeccToL2( &getVariables()->m_out );
    getVariables()->m_apcMsg = false;
}

void ApsStateMachine::States::doTransferToL2() { goToWaiting(); }

void ApsStateMachine::States::goToTransferToApc()
{
    m_current_state = &States::doTransferToApc;
    getActions()->sendAvdeccToApc( &getVariables()->m_in );
    getVariables()->m_nopTimeout = getVariables()->m_currentTime + 10;
    getVariables()->m_L2Msg = false;
}

void ApsStateMachine::States::doTransferToApc() { goToWaiting(); }

void ApsStateMachine::States::goToAssignEntityId()
{
    m_current_state = &States::doAssignEntityId;
    getActions()->sendEntityIdAssignment( getVariables()->m_a,
                                          getVariables()->m_entity_id );
    getVariables()->m_nopTimeout = getVariables()->m_currentTime + 10;
    getVariables()->m_assignEntityIdRequest = false;
}

void ApsStateMachine::States::doAssignEntityId() { goToWaiting(); }

void ApsStateMachine::States::goToSendNop()
{
    m_current_state = &States::doSendNop;
    getActions()->sendNopToApc();
    getVariables()->m_nopTimeout = getVariables()->m_currentTime + 10;
}

void ApsStateMachine::States::doSendNop() { goToWaiting(); }

void ApsStateMachine::States::goToCloseAndFinish()
{
    m_current_state = &States::doCloseAndFinish;
    getActions()->closeTcpConnection();
}

void ApsStateMachine::States::doCloseAndFinish() { goToFinish(); }

void ApsStateMachine::States::goToFinish()
{
    m_current_state = &States::doFinish;
    getActions()->closeTcpServer();
}

void ApsStateMachine::States::doFinish() { m_current_state = 0; }
}