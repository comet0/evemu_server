#
# CMake build system file for EVEmu.
#
# Author: Bloody.Rabbit
#

##############
# Initialize #
##############
SET( TARGET_NAME        "eve-core" )
SET( TARGET_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src/${TARGET_NAME}" )
SET( TARGET_SOURCE_DIR  "${PROJECT_SOURCE_DIR}/src/${TARGET_NAME}" )

#########
# Files #
#########
SET( INCLUDE
     "${TARGET_INCLUDE_DIR}/eve-core.h"
     "${TARGET_INCLUDE_DIR}/eve-compat.h" )
SET( SOURCE
     "${TARGET_SOURCE_DIR}/eve-compat.cpp" )

SET( database_INCLUDE
     "${TARGET_INCLUDE_DIR}/database/dbcore.h"
     "${TARGET_INCLUDE_DIR}/database/dbtype.h" )
SET( database_SOURCE
     "${TARGET_SOURCE_DIR}/database/dbcore.cpp"
     "${TARGET_SOURCE_DIR}/database/dbtype.cpp" )

SET( log_INCLUDE
     "${TARGET_INCLUDE_DIR}/log/Task_Log.h"
     "${TARGET_INCLUDE_DIR}/log/SystemLog.h"
     "${TARGET_INCLUDE_DIR}/log/logsys.h"
     "${TARGET_INCLUDE_DIR}/log/logtypes.h" )
SET( log_SOURCE
     "${TARGET_SOURCE_DIR}/log/Task_Log.cpp"
     "${TARGET_SOURCE_DIR}/log/SystemLog.cpp"
     "${TARGET_SOURCE_DIR}/log/logsys.cpp" )

SET( network_INCLUDE
     "${TARGET_INCLUDE_DIR}/network/NetUtils.h"
     "${TARGET_INCLUDE_DIR}/network/Socket.h"
     "${TARGET_INCLUDE_DIR}/network/StreamPacketizer.h"
     "${TARGET_INCLUDE_DIR}/network/TCPConnection.h"
     "${TARGET_INCLUDE_DIR}/network/TCPServer.h" )
SET( network_SOURCE
     "${TARGET_SOURCE_DIR}/network/NetUtils.cpp"
     "${TARGET_SOURCE_DIR}/network/Socket.cpp"
     "${TARGET_SOURCE_DIR}/network/StreamPacketizer.cpp"
     "${TARGET_SOURCE_DIR}/network/TCPConnection.cpp" )

SET( threading_INCLUDE
     "${TARGET_INCLUDE_DIR}/threading/Mutex.h" )
SET( threading_SOURCE
     "${TARGET_SOURCE_DIR}/threading/Mutex.cpp" )

SET( utils_INCLUDE
     "${TARGET_INCLUDE_DIR}/utils/Buffer.h"
     "${TARGET_INCLUDE_DIR}/utils/crc32.h"
     "${TARGET_INCLUDE_DIR}/utils/Deflate.h"
     "${TARGET_INCLUDE_DIR}/utils/Lock.h"
     "${TARGET_INCLUDE_DIR}/utils/misc.h"
     "${TARGET_INCLUDE_DIR}/utils/RefPtr.h"
     "${TARGET_INCLUDE_DIR}/utils/SafeMem.h"
     "${TARGET_INCLUDE_DIR}/utils/Seperator.h"
     "${TARGET_INCLUDE_DIR}/utils/str2conv.h"
     "${TARGET_INCLUDE_DIR}/utils/timer.h"
     "${TARGET_INCLUDE_DIR}/utils/utils_hex.h"
     "${TARGET_INCLUDE_DIR}/utils/utils_string.h"
     "${TARGET_INCLUDE_DIR}/utils/utils_time.h"
     "${TARGET_INCLUDE_DIR}/utils/XMLDocument.h"
     "${TARGET_INCLUDE_DIR}/math/Vector3D.h"
)
SET( utils_SOURCE
     "${TARGET_SOURCE_DIR}/utils/crc32.cpp"
     "${TARGET_SOURCE_DIR}/utils/Deflate.cpp"
     "${TARGET_SOURCE_DIR}/utils/misc.cpp"
     "${TARGET_SOURCE_DIR}/utils/Seperator.cpp"
     "${TARGET_SOURCE_DIR}/utils/str2conv.cpp"
     "${TARGET_SOURCE_DIR}/utils/timer.cpp"
     "${TARGET_SOURCE_DIR}/utils/utils_hex.cpp"
     "${TARGET_SOURCE_DIR}/utils/utils_string.cpp"
     "${TARGET_SOURCE_DIR}/utils/utils_time.cpp"
     "${TARGET_SOURCE_DIR}/utils/XMLDocument.cpp"
     "${TARGET_SOURCE_DIR}/math/Vector3D.cpp"
      )

#####################
# Setup the library #
#####################
SOURCE_GROUP( "src"            FILES ${SOURCE} )
SOURCE_GROUP( "src\\database"  FILES ${database_SOURCE} )
SOURCE_GROUP( "src\\log"       FILES ${log_SOURCE} )
SOURCE_GROUP( "src\\network"   FILES ${network_SOURCE} )
SOURCE_GROUP( "src\\threading" FILES ${threading_SOURCE} )
SOURCE_GROUP( "src\\utils"     FILES ${utils_SOURCE} )

ADD_LIBRARY( "${TARGET_NAME}"
${SOURCE}
${database_SOURCE}
${log_SOURCE}
${network_SOURCE}
${threading_SOURCE}
${utils_SOURCE} )

TARGET_INCLUDE_DIRECTORIES( "${TARGET_NAME}"
                            ${Boost_INCLUDE_DIRS}
                            ${MYSQL_INCLUDE_DIRS}
                            "${ZLIB_INCLUDE_DIR}"
                            "${TARGET_INCLUDE_DIR}" )
TARGET_LINK_LIBRARIES( "${TARGET_NAME}"
                       ${Boost_LIBRARIES}
                       ${MYSQL_LIBRARIES}
                       "${ZLIB_LIBRARY}"
                       ${CMAKE_NET_LIBS}
                       ${CMAKE_THREAD_LIBS} )
INCLUDE_DIRECTORIES( "${PROJECT_BINARY_DIR}" )
IF(PCH)
TARGET_BUILD_PCH( "${TARGET_NAME}"
                  "${TARGET_INCLUDE_DIR}/eve-core.h"
                  "${TARGET_SOURCE_DIR}/eve-compat.cpp" )
ENDIF()
