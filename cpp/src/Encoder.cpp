#include "stdafx.H"
#include "Encoder.H"
#include <io.h>
#include <sha.h>
#include <hex.h>
#include <channels.h>
#include <files.h>
#include <sstream>
#include <base64.h>
#include "Flood.H"

namespace libBitFlood
{
  namespace Encoder
  {
    /*static*/
    Error::ErrorCode EncodeFile( const ToEncode& i_toencode, FloodFile& o_floodfile )
    {
      Error::ErrorCode ret = Error::NO_ERROR_LBF;

      libBitFlood::FloodFile toReturn;
      toReturn.m_trackers = i_toencode.m_trackers;

      // check incoming data - move to a function in ToEncode?
      if ( i_toencode.m_files.empty() )
      {
        ret = Error::UNKNOWN_ERROR_LBF;
      }
      else if ( i_toencode.m_chunksize <= 0 )
      {
        ret = Error::UNKNOWN_ERROR_LBF;
      }
      else
      {
        V_String::const_iterator fileiter = i_toencode.m_files.begin();
        V_String::const_iterator fileend  = i_toencode.m_files.end();

        U32 num_bytes = i_toencode.m_chunksize / sizeof( U8 );
        U8* buffer = new U8[ num_bytes ];

        for ( ; fileiter != fileend; ++fileiter )
        {
          U32 filesize = 0;
          FILE* file = fopen( fileiter->c_str(), "rb" );
          if ( file == NULL )
          {
            ret = Error::UNKNOWN_ERROR_LBF;
          }
          else
          {
            libBitFlood::FloodFile::File fileInfo;
            fileInfo.m_name = *fileiter;

            U32 index = 0;
            for( ;; ++index )
            {
              U32 bytesRead = (U32)fread( buffer, sizeof( U8 ), num_bytes, file );
              if ( bytesRead > 0 )
              {
                filesize += bytesRead;

                libBitFlood::FloodFile::Chunk chunk;
                chunk.m_index = index;
                chunk.m_size = bytesRead;
                chunk.m_weight = 0;
                Base64Encode( buffer, bytesRead, chunk.m_hash );
                fileInfo.m_chunks.push_back( chunk );
              }
              else
              {
                break;
              }
            }

            fclose( file );

            fileInfo.m_size = filesize;
            toReturn.m_files.push_back( fileInfo );
          }
        }

        delete [] buffer;
      }

      if ( ret == Error::NO_ERROR_LBF )
      {
        o_floodfile = toReturn;
      }

      return ret;
    }

    static const byte s_Base64[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    Error::ErrorCode Base64Encode( const U8* i_data, U32 i_size, std::string& o_string )
    {
      using namespace CryptoPP;
      SHA sha;
      std::stringstream out;
      HashFilter shaFilter(sha, new BaseN_Encoder( s_Base64, 6, new FileSink( out ) ) );
      std::auto_ptr<ChannelSwitch> channelSwitch(new ChannelSwitch);
      channelSwitch->AddDefaultRoute(shaFilter);
      
      StringSource( i_data, i_size, true, channelSwitch.release() );
      o_string = out.str();

      return Error::NO_ERROR_LBF;
    }
  }
};

