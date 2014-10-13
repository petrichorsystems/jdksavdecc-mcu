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
#pragma once

#include "JDKSAvdeccMCU_World.hpp"

namespace JDKSAvdeccMCU
{

class FixedBuffer
{
  protected:
    uint8_t *m_buf;
    uint16_t m_write_pos;
    uint16_t m_len;

  public:
    FixedBuffer( uint8_t *buf, uint16_t len )
        : m_buf( buf ), m_write_pos( 0 ), m_len( len )
    {
    }

    uint16_t getMaxLength() const { return m_len; }

    void putEUI48( jdksavdecc_eui48 const &val )
    {
        for ( uint16_t i = 0; i < 6; ++i )
        {
            m_buf[m_write_pos++] = val.value[i];
        }
    }

    void putEUI64( jdksavdecc_eui64 const &val )
    {
        for ( uint16_t i = 0; i < 8; ++i )
        {
            m_buf[m_write_pos++] = val.value[i];
        }
    }

    void putOctet( uint8_t val ) { m_buf[m_write_pos++] = val; }

    void putDoublet( uint16_t val )
    {
        m_buf[m_write_pos++] = ( val >> 8 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 0 ) & 0xff;
    }

    void putQuadlet( uint32_t val )
    {
        m_buf[m_write_pos++] = ( val >> 24 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 16 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 8 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 0 ) & 0xff;
    }

    void putOctlet( uint64_t val )
    {
        m_buf[m_write_pos++] = ( val >> 56 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 48 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 40 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 32 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 24 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 16 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 8 ) & 0xff;
        m_buf[m_write_pos++] = ( val >> 0 ) & 0xff;
    }

    void putZeros( uint16_t count )
    {
        for ( uint16_t i = 0; i < count; ++i )
        {
            m_buf[m_write_pos++] = 0;
        }
    }

    void putBuf( uint8_t *buf, uint16_t len )
    {
        for ( uint16_t i = 0; i < len; ++i )
        {
            m_buf[m_write_pos++] = buf[i];
        }
    }

    void setOctet( uint16_t val, uint8_t pos ) { m_buf[pos] = val; }
    uint8_t getOctet( uint16_t pos ) const { return m_buf[pos]; }

    void setDoublet( uint16_t v, uint16_t pos )
    {
        jdksavdecc_uint16_set( v, m_buf, pos );
    }

    uint16_t getDoublet( uint16_t pos ) const
    {
        return jdksavdecc_uint16_get( m_buf, pos );
    }

    void setQuadlet( uint32_t v, uint16_t pos )
    {
        jdksavdecc_uint32_set( v, m_buf, pos );
    }

    uint32_t getQuadlet( uint16_t pos ) const
    {
        return jdksavdecc_uint32_get( m_buf, pos );
    }

    void setEUI48( jdksavdecc_eui48 v, uint16_t pos )
    {
        jdksavdecc_eui48_set( v, m_buf, pos );
    }

    jdksavdecc_eui48 getEUI48( uint16_t pos ) const
    {
        return jdksavdecc_eui48_get( m_buf, pos );
    }

    void setEUI64( jdksavdecc_eui64 v, uint16_t pos )
    {
        jdksavdecc_eui64_set( v, m_buf, pos );
    }

    jdksavdecc_eui64 getEUI64( uint16_t pos )
    {
        return jdksavdecc_eui64_get( m_buf, pos );
    }

    uint8_t const *getBuf() const { return m_buf; }
    uint8_t *getBuf() { return m_buf; }
    uint8_t const *getBuf( uint16_t pos ) const { return &m_buf[pos]; }
    uint8_t *getBuf( uint16_t pos ) { return &m_buf[pos]; }

    uint16_t getWritePosition() const { return m_write_pos; }
    uint16_t getSize() const { return m_write_pos; }
    void setWritePosition( uint16_t n ) { m_write_pos = n; }
    void setLength( uint16_t n ) { m_write_pos = n; }
};

template <size_t MaxSize>
class FixedBufferWithSize : public FixedBuffer
{
  protected:
    uint8_t m_buf_storage[MaxSize];

  public:
    FixedBufferWithSize() : FixedBuffer( m_buf_storage, MaxSize ) {}
};
}