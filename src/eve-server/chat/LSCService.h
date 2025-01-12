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
    Author:     Zhur, Aknor Jaden
*/


#ifndef __LSC_SERVICE_H_INCL__
#define __LSC_SERVICE_H_INCL__

#include "chat/LSCDB.h"
#include "chat/LSCChannel.h"
#include "PyService.h"

/*
 *
 * LSC stands for Large Scale Chat
 *
 *
CHTMODE_CREATOR = (((8 + 4) + 2) + 1)
CHTMODE_OPERATOR = ((4 + 2) + 1)
CHTMODE_CONVERSATIONALIST = (2 + 1)
CHTMODE_SPEAKER = 2
CHTMODE_LISTENER = 1
CHTMODE_NOTSPECIFIED = -1
CHTMODE_DISALLOWED = -2 *
 *
 *
CHTERR_NOSUCHCHANNEL = -3
CHTERR_ACCESSDENIED = -4
CHTERR_INCORRECTPASSWORD = -5
CHTERR_ALREADYEXISTS = -6
CHTERR_TOOMANYCHANNELS = -7
CHT_MAX_USERS_PER_IMMEDIATE_CHANNEL = 50
 *
 *
 *
 *
 *
*/

class CommandDispatcher;

class LSCService : public PyService
{
public:
    LSCService(CommandDispatcher *cd);
    virtual ~LSCService();

protected:
    class Dispatcher;
   
    CommandDispatcher *const m_commandDispatch;

    //make sure you add things to the constructor too
    PyCallable_DECL_CALL(GetChannels)
    PyCallable_DECL_CALL(GetRookieHelpChannel)
//    PyCallable_DECL_CALL(JoinChannels)
//    PyCallable_DECL_CALL(LeaveChannels)
//    PyCallable_DECL_CALL(LeaveChannel)
    PyCallable_DECL_CALL(CreateChannel)
    PyCallable_DECL_CALL(Configure)
    PyCallable_DECL_CALL(DestroyChannel)
    PyCallable_DECL_CALL(GetMembers)
    PyCallable_DECL_CALL(GetMember)
//    PyCallable_DECL_CALL(SendMessage)
    PyCallable_DECL_CALL(Invite)
    PyCallable_DECL_CALL(AccessControl)

    PyCallable_DECL_CALL(GetMyMessages)
    PyCallable_DECL_CALL(GetMessageDetails)
    PyCallable_DECL_CALL(Page)
    PyCallable_DECL_CALL(MarkMessagesRead)
    PyCallable_DECL_CALL(DeleteMessages)

private:
};




#endif


