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
    Author:     Captnoord
*/

#include "eve-core.h"

#include "log/SystemLog.h"
#include "log/logtypes.h"
#include "log/logsys.h"

/*************************************************************************/
/* NewLog                                                                */
/*************************************************************************/
#ifdef HAVE_WINDOWS_H
const WORD SysLog::COLOR_TABLE[ COLOR_COUNT ] =
{
    ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE                        ), // COLOR_DEFAULT
    ( 0                                                                          ), // COLOR_BLACK
    ( FOREGROUND_RED                                      | FOREGROUND_INTENSITY ), // COLOR_RED
    (                  FOREGROUND_GREEN                   | FOREGROUND_INTENSITY ), // COLOR_GREEN
    ( FOREGROUND_RED | FOREGROUND_GREEN                   | FOREGROUND_INTENSITY ), // COLOR_YELLOW
    (                                     FOREGROUND_BLUE | FOREGROUND_INTENSITY ), // COLOR_BLUE
    ( FOREGROUND_RED                    | FOREGROUND_BLUE | FOREGROUND_INTENSITY ), // COLOR_MAGENTA
    (                  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY ), // COLOR_CYAN
    ( FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY )  // COLOR_WHITE
};
#else /* !HAVE_WINDOWS_H */
const char* const SysLog::COLOR_TABLE[ COLOR_COUNT ] =
{
    "\033[" "00"    "m", // COLOR_DEFAULT
    "\033[" "30;22" "m", // COLOR_BLACK
    "\033[" "31;22" "m", // COLOR_RED
    "\033[" "32;22" "m", // COLOR_GREEN
    "\033[" "33;01" "m", // COLOR_YELLOW
    "\033[" "34;01" "m", // COLOR_BLUE
    "\033[" "35;22" "m", // COLOR_MAGENTA
    "\033[" "36;01" "m", // COLOR_CYAN
    "\033[" "37;01" "m"  // COLOR_WHITE
};
#endif /* !HAVE_WINDOWS_H */

#define CONSOLE_LOG_PADDING 20

/// The active logfile.
FILE* SysLog::mLogfile = nullptr;
/// Current timestamp.
time_t SysLog::mTime; // crap there should be 1 generic easy to understand time manager.
/// Protection against concurrent log messages
Mutex SysLog::mMutex;
bool SysLog::m_initialized = false;

#ifdef HAVE_WINDOWS_H
/// Handle to standard output stream.
HANDLE SysLog::mStdOutHandle = nullptr;
/// Handle to standard error stream.
HANDLE SysLog::mStdErrHandle = nullptr;
#endif /* !HAVE_WINDOWS_H */

SysLog::SysLog()
{
}

SysLog::~SysLog()
{
    Debug( "Log", "Log system shutting down" );

    // close logfile
    SetLogfile( (FILE*)NULL );
}

void SysLog::InitializeLogging(std::string logPath)
{
    mLogfile = nullptr;
    mTime = 0;
#ifdef HAVE_WINDOWS_H
    mStdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    mStdErrHandle = GetStdHandle(STD_ERROR_HANDLE);
#endif /* HAVE_WINDOWS_H */

    // open default logfile
    if( logPath.empty() )
        logPath = EVEMU_ROOT "/log/";

    m_initialized = true;

    SetLogfileDefault(logPath);

    Debug( "Log", "Log system initiated" );
}

void SysLog::Log(const char* source, const char* fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );

    PrintMsg( COLOR_DEFAULT, 'L', source, fmt, ap );

    va_end( ap );
}

void SysLog::Error(const char* source, const char* fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );

    PrintMsg( COLOR_RED, 'E', source, fmt, ap );

    va_end( ap );
}

void SysLog::Warning(const char* source, const char* fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );

    PrintMsg( COLOR_YELLOW, 'W', source, fmt, ap );

    va_end( ap );
}

void SysLog::Success(const char* source, const char* fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );

    PrintMsg( COLOR_GREEN, 'S', source, fmt, ap );

    va_end( ap );
}

void SysLog::Debug(const char* source, const char* fmt, ...)
{
//#ifndef NDEBUG
    if( is_log_enabled( DEBUG__DEBUG ) )
    {
        va_list ap;
        va_start( ap, fmt );

        PrintMsg( COLOR_CYAN, 'D', source, fmt, ap );

        va_end( ap );
    }
//#endif /* !NDEBUG */
}

bool SysLog::SetLogfile(const char* filename)
{
    MutexLock l( mMutex );

    FILE* file = NULL;

    if( NULL != filename )
    {
        file = fopen( filename, "w" );
#ifdef HAVE_UNISTD_H
		// Change file owner to nobody:nobody to prevent possible permissions problems.
        // Used if the server is started with root permissions on a Linux host.
        fchown(fileno(file), 99, 99);   // nobody:nobody
        fchmod(fileno(file), S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);    // -rw-rw-rw-
#endif
        if( NULL == file )
            return false;
    }

    return SetLogfile( file );
}

bool SysLog::SetLogfile(FILE* file)
{
    MutexLock l( mMutex );

    if( NULL != mLogfile )
        assert( 0 == fclose( mLogfile ) );

    mLogfile = file;
    return true;
}

void SysLog::PrintMsg(Color color, char pfx, const char* source, const char* fmt, va_list ap)
{
    if( !m_initialized )
        return;

    MutexLock l( mMutex );

    PrintTime();

    SetColor( color );
    Print( " %c ", pfx );

    std::string pad = "";
    if(strlen(source) < CONSOLE_LOG_PADDING)
        pad = std::string(CONSOLE_LOG_PADDING-strlen(source), ' ');

    if( source && *source )
    {
        SetColor( COLOR_WHITE );
        Print( "%s%s: ", pad.c_str(), source );

        SetColor( color );
    }

    PrintVa( fmt, ap );
    Print( "\n" );

    SetColor( COLOR_DEFAULT );
}

void SysLog::PrintTime()
{
    MutexLock l( mMutex );

    // this will be replaced my a timing thread somehow
    SetTime( time( NULL ) );

    tm t;
    localtime_r( &mTime, &t );

    Print( "%02u:%02u:%02u", t.tm_hour, t.tm_min, t.tm_sec );
}

void SysLog::Print(const char* fmt, ...)
{
    va_list ap;
    va_start( ap, fmt );

    PrintVa( fmt, ap );

    va_end( ap );
}

void SysLog::PrintVa(const char* fmt, va_list ap)
{
    MutexLock l( mMutex );

    if( NULL != mLogfile )
    {
        // this is a design flaw ( UNIX related )
        va_list ap2;
        va_copy( ap2, ap );

        vfprintf( mLogfile, fmt, ap2 );

#ifndef NDEBUG
        // flush immediately so logfile is accurate if we crash
        fflush( mLogfile );
#endif /* !NDEBUG */

        va_end( ap2 );
    }

    vprintf( fmt, ap );
}

void SysLog::SetColor(Color color)
{
    assert( 0 <= color && color < COLOR_COUNT );

    MutexLock l( mMutex );

#ifdef HAVE_WINDOWS_H
    ::SetConsoleTextAttribute( mStdOutHandle, COLOR_TABLE[ color ] );
#else /* !HAVE_WINDOWS_H */
    ::fputs( COLOR_TABLE[ color ], stdout );
#endif /* !HAVE_WINDOWS_H */
}

void SysLog::SetLogfileDefault(std::string logPath)
{
    MutexLock l( mMutex );

    // set initial log system time
    SetTime( time( NULL ) );

    tm t;
    localtime_r( &mTime, &t );

    // open default logfile
    char filename[ FILENAME_MAX + 1 ];
    std::string logFile = logPath + "log_%02u-%02u-%04u-%02u-%02u.log";
    snprintf( filename, FILENAME_MAX + 1, logFile.c_str(),
              t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min );
    //snprintf( filename, FILENAME_MAX + 1, EVEMU_ROOT "/log/log_%02u-%02u-%04u-%02u-%02u.log",
    //          t.tm_mday, t.tm_mon + 1, t.tm_year + 1900, t.tm_hour, t.tm_min );

    if( SetLogfile( filename ) )
        Success( "Log", "Opened logfile '%s'.", filename );
    else
        Warning( "Log", "Unable to open logfile '%s': %s", filename, strerror( errno ) );
}