#include "stdafx.H"
#include "Flood.H"
#include "Peer.H"
#include "PeerConnection.H"
#include "Encoder.H"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <assert.h>

namespace libBitFlood
{
  //
  Error::ErrorCode Flood::Initialize( const FloodFile& i_floodfile )
  {
    m_floodfile = i_floodfile;

    V_String::const_iterator trackeriter = m_floodfile.m_trackers.begin();
    V_String::const_iterator trackerend = m_floodfile.m_trackers.end();

    for( ; trackeriter != trackerend; ++trackeriter )
    {
      TrackerInfo info;
      const std::string& trackerurl = *trackeriter;

      U32 h_start = trackerurl.find( "http://" ) + strlen( "http://" );
      U32 p_start = trackerurl.find( ':', h_start ) + 1;
      U32 u_start = trackerurl.find( '/', p_start ) + 1;

      info.m_host = trackerurl.substr( h_start, p_start - h_start - 1 );
      std::stringstream port_converter;
      port_converter << trackerurl.substr( p_start, u_start - p_start - 1 );
      port_converter >> info.m_port;

      std::stringstream idstrm;
      idstrm << info.m_host << info.m_port;

      Encoder::Base64Encode( (const U8*)idstrm.str().c_str(), idstrm.str().length(), info.m_id );
      m_trackerinfos.push_back( info );
    }

    // here we setup our internal representation of the state of the files
    // and the chunks that we either have already downloaded or want to download
    _SetupFilesAndChunks();

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Flood::InqPeer( const std::string& i_peerid, PeerConnectionSPtr& o_peer )
  {
    o_peer = PeerConnectionSPtr( NULL );
    if ( !i_peerid.empty() )
    {
      V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
      V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

      for ( ; peeriter != peerend; ++peeriter )
      {
        if ( (*peeriter)->m_id.compare( i_peerid ) == 0 )
        {
          o_peer = (*peeriter);
          break;
        }
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Flood::_SetupFilesAndChunks( void )
  {
    m_totalbytes = 0;

    FloodFile::V_File::const_iterator fileiter = m_floodfile.m_files.begin();
    FloodFile::V_File::const_iterator fileend  = m_floodfile.m_files.end();

    for ( ; fileiter != fileend; ++fileiter )
    {
      const FloodFile::File& file = *fileiter;

      // track the total size of the flood
      m_totalbytes += file.m_size;

      // setup runtime data
      RuntimeFile rtf;
      rtf.m_chunkoffsets = V_U32( file.m_chunks.size(), 0 );
      rtf.m_chunkmap = std::string( file.m_chunks.size(), '0' );

      FloodFile::V_Chunk::const_iterator chunkiter = file.m_chunks.begin();
      FloodFile::V_Chunk::const_iterator chunkend  = file.m_chunks.end();

      FILE* fileptr = fopen( file.m_name.c_str(), "rb" );

      U32 next_offset = 0;
      for ( ; chunkiter != chunkend; ++chunkiter )
      {
        const FloodFile::Chunk& chunk = *chunkiter;

        assert( rtf.m_chunkindices.find( chunk.m_hash ) == rtf.m_chunkindices.end() );
        assert( rtf.m_chunkoffsets[ chunk.m_index ] == 0 );
        assert( rtf.m_chunkmap[ chunk.m_index ] == '0' );

        rtf.m_chunkindices[ chunk.m_hash ]  = chunk.m_index;
        rtf.m_chunkoffsets[ chunk.m_index ] = next_offset;

        // if the file exists we need to see if this chunk is valid
        // allocate a buffer the size of the chunk and attempt to read in that data
        // if we can read it in and it hashes the same as what is in our flood file, 
        // we don't need to download it
        if ( fileptr )
        {
          if ( fseek( fileptr, next_offset, SEEK_SET ) == 0 )
          {
            U8* data = (U8*)malloc( chunk.m_size );
            if ( fread( data, 1, chunk.m_size, fileptr ) == chunk.m_size )
            {
              std::string test;
              Encoder::Base64Encode( data, chunk.m_size, test );
              if( chunk.m_hash.compare( test ) == 0 )
              {
                rtf.m_chunkmap[ chunk.m_index ] = '1';
              }
            }
            free( data );
          }
        }

        // update the next offset
        next_offset += chunk.m_size;

        // do we need to download this chunk?  remember that somewhere
        if ( rtf.m_chunkmap[ chunk.m_index ] == '0' )
        {
          m_chunkstodownload.push_back( P_ChunkKey( m_runtimefiles.size(), chunk.m_index ) );
        }
      }

      // don't forget to close the file handle
      if ( fileptr )
      {
        fclose( fileptr );
      }

      // add this file data to our vector
      m_runtimefiles.push_back( rtf );
    }

    // done
    return Error::NO_ERROR_LBF;
  }
};

