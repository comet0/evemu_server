/*
    ------------------------------------------------------------------------------------
    LICENSE:
    ------------------------------------------------------------------------------------
    This file is part of EVEmu: EVE Online Server Emulator
    Copyright 2006 - 2016 The EVEmu Team
    For the latest information visit http://evemu.org
    ------------------------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify it under
    the terms of the GNU Lesser General Public License as published by the Free Software
    Foundation; either version 2 of the License, or (at your option) any later
    version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place - Suite 330, Boston, MA 02111-1307, USA, or go to
    http://www.gnu.org/copyleft/lesser.txt.
    ------------------------------------------------------------------------------------
    Author:        Zhur
*/

#ifndef __EVE_SESSION_H__INCL__
#define __EVE_SESSION_H__INCL__

#include "network/EVETCPConnection.h"

class PyPacket;
class PyRep;

class VersionExchangeClient;
class VersionExchangeServer;
class CryptoRequestPacket;
class CryptoChallengePacket;
class CryptoHandshakeResult;

/**
 * @brief Client session from server's side.
 *
 * Defines what server needs to do when dealing with client;
 * takes care of initial authentication, before actual
 * packets are sent.
 *
 * @author Bloody.Rabbit
 */
class EVEClientSession
{
public:
    /** Typedef for readability. */
    typedef TCPConnection::state_t state_t;

    /**
     * @param[in] n Connection of this session.
     */
    EVEClientSession(EVETCPConnection** n);
    /**
     * @brief Destroys contained connection.
     */
    virtual ~EVEClientSession();

    /**
     * Get the TCPConnection state.
     * @return The connection state.
     */
    state_t getState() const
    {
        return m_tcpConnecton->GetState();
    }

    /**
     * Get the TCPConnection address.
     * @return The address of the tcp connection.
     */
    std::string getAddress() const
    {
        return m_tcpConnecton->GetAddress();
    }

    /**
     * Reset session so we act like client just connected.
     */
    void reset();

    /**
     * @brief Queues new packet, retaking ownership.
     * @param[in] p Packed to be queued.
     * @param[in] front should the packet be queued to be sent first?
     */
    void fastQueuePacket(PyPacket** p, bool front = false);

    /**
     * @brief Pops new packet from queue.
     * @return Popped packet.
     */
    PyPacket* popPacket();

    /**
     * @brief Disconnects client from the server
     */
    void closeClientConnection()
    {
        m_tcpConnecton->Disconnect();
    }


protected:
    /**
     * @brief Obtains version.
     *
     * @param[in] version Object to be filled with version information.
     */
    virtual void _GetVersion( VersionExchangeServer& version ) = 0;
    /** @return Current count of connected users. */
    virtual uint32 _GetUserCount() = 0;
    /** @return Current position in connection queue. */
    virtual uint32 _GetQueuePosition() = 0;

    /**
     * @brief Verifies version.
     *
     * @param[in] version Version sent by client.
     *
     * @retval true  Verification succeeded; proceeds to next state.
     * @retval false Verification failed; stays in current state.
     */
    virtual bool _VerifyVersion( VersionExchangeClient& version ) = 0;
    /**
     * @brief Verifies VIP key.
     *
     * @param[in] vipKey VIP key sent by client.
     *
     * @retval true  Verification succeeded; proceeds to next state.
     * @retval false Verification failed; stays in current state.
     */
    virtual bool _VerifyVIPKey( const std::string& vipKey ) = 0;
    /**
     * @brief Verifies crypto.
     *
     * @param[in] cr Crypto sent by client.
     *
     * @retval true  Verification succeeded; proceeds to next state.
     * @retval false Verification failed; stays in current state.
     */
    virtual bool _VerifyCrypto( CryptoRequestPacket& cr ) = 0;
    /**
     * @brief Verifies login.
     *
     * @param[in] ccp Login data sent by client.
     *
     * @retval true  Verification succeeded; proceeds to next state.
     * @retval false Verification failed; stays in current state.
     */
    virtual bool _VerifyLogin( CryptoChallengePacket& ccp ) = 0;
    /**
     * @brief Verifies function result.
     *
     * @param[in] result Function result sent by client.
     *
     * @retval true  Verification succeeded; proceeds to next state.
     * @retval false Verification failed; stays in current state.
     */
    virtual bool _VerifyFuncResult( CryptoHandshakeResult& result ) = 0;

    /** Connection of this session. */
    EVETCPConnection * const m_tcpConnecton;

private:
    // State machine facility:
    PyPacket* (EVEClientSession::*m_packetHandler)(PyRep* rep);

    PyPacket* _HandleVersion( PyRep* rep );
    PyPacket* _HandleCommand( PyRep* rep );
    PyPacket* _HandleCrypto( PyRep* rep );
    PyPacket* _HandleAuthentication( PyRep* rep );
    PyPacket* _HandleFuncResult( PyRep* rep );
    PyPacket* _HandlePacket( PyRep* rep );
};

#endif /* !__EVE_SESSION_H__INCL__ */
