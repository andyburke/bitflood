#include "stdafx.H"
#include "Flood.H"
#include "Peer.H"
#include "PeerConnection.H"
#include "Encoder.H"
#include "ChunkMethods.H"
#include "TrackerMethods.H"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <assert.h>

#include <WINSOCK2.H>
#include <Ws2tcpip.h>

namespace libBitFlood
{
  //
  Error::ErrorCode Flood::Initialize( PeerSPtr& i_localpeer, FloodFileSPtr& i_floodfile )
  {
    m_localpeer = i_localpeer.Get();
    m_floodfile = i_floodfile;

    V_String::const_iterator trackeriter = m_floodfile->m_trackers.begin();
    V_String::const_iterator trackerend = m_floodfile->m_trackers.end();

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

      hostent* tmp = gethostbyname( info.m_host.c_str() );
      info.m_host = inet_ntoa (*(struct in_addr *)*tmp->h_addr_list);

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

  Error::ErrorCode Flood::LoopOnce( void )
  {
    const U32 UPDATE_INTERVAL = 20;

    GetChunks();
    
    time_t now;
    time( &now );
    if ( now - m_lasttrackerupdate >= UPDATE_INTERVAL )
    {
      UpdateTrackers();
      time( &m_lasttrackerupdate );
    }

    // copy the peer sptr vector, chances are it will change in this loop
    V_PeerConnectionSPtr peers = m_peers;
    
    V_PeerConnectionSPtr::iterator peeriter = peers.begin();
    V_PeerConnectionSPtr::iterator peerend  = peers.end();

    for ( ; peeriter != peerend; ++peeriter )
    {
      // process
      (*peeriter)->LoopOnce();

      // reap defunct peer connections
      if ( (*peeriter)->m_disconnected )
      {
      }
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Flood::GetChunks( void )
  {
    bool foundchunk = false;
    PeerConnectionSPtr todownload_from;
    Flood::P_ChunkKey* todownload_key = NULL;

    // figure out what chunk to get from which peer and ask for it
    V_ChunkKey::iterator chunkiter = m_chunkstodownload.begin();
    V_ChunkKey::iterator chunkend  = m_chunkstodownload.end();
    
    for ( ; chunkiter != chunkend && !foundchunk; ++chunkiter )
    {
      Flood::P_ChunkKey& thechunkkey = (*chunkiter);
      if ( m_chunksdownloading.find( thechunkkey ) == m_chunksdownloading.end() )
      {
        const FloodFile::File& thefile = m_floodfile->m_files[ thechunkkey.first ];
        const FloodFile::Chunk& thechunk = thefile.m_chunks[ thechunkkey.second ];

        V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
        V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

        for ( ; peeriter != peerend && !foundchunk; ++peeriter )
        {
          PeerConnectionSPtr& thepeer = (*peeriter);
          M_StrToStrToStr::iterator peerchunkmap = m_peerchunkmaps.find( thepeer->m_id );
          if ( peerchunkmap != m_peerchunkmaps.end() )
          {
            M_StrToStr::iterator filechunkmap = (*peerchunkmap).second.find( thefile.m_name );
            if( filechunkmap != (*peerchunkmap).second.end() )
            {
              const std::string& chunkmap = (*filechunkmap).second;
              if ( chunkmap[ thechunkkey.second ] == '1' )
              {
                foundchunk = true;
                todownload_from  = thepeer;
                todownload_key   = &thechunkkey;
              }
            }
          }
        }
      }
    }

    if ( foundchunk )
    {
      XmlRpcValue args;
      args[0] = m_floodfile->m_files[ todownload_key->first ].m_name;
      args[1] = (int)todownload_key->second;

      todownload_from->SendMethod( ChunkMethodHandler::RequestChunk, args );
      m_chunksdownloading.insert( *todownload_key );
    }

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Flood::UpdateTrackers( void )
  {
    V_TrackerInfo::const_iterator trackeriter = m_trackerinfos.begin();
    V_TrackerInfo::const_iterator trackerend  = m_trackerinfos.end();
    for ( ; trackeriter != trackerend; ++trackeriter )
    {
      const Flood::TrackerInfo& tracker = (*trackeriter);
      
      if ( tracker.m_id.compare( m_localpeer->m_localid ) != 0 )
      {
        PeerConnectionSPtr peer;
        InqPeer( tracker.m_id, peer );
        
        if ( peer.Get() == NULL )
        {
          peer = PeerConnectionSPtr( new PeerConnection() );
          peer->InitializeCommon( PeerSPtr( m_localpeer ), tracker.m_host, tracker.m_port );
          peer->InitializeOutgoing( FloodSPtr( this ), tracker.m_id );
          m_peers.push_back( peer );
        }
        
        // request a peer list
        {
          XmlRpcValue args;
          peer->SendMethod( TrackerMethodHandler::RequestPeerList, args );
        }
      }
    }

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

    FloodFile::V_File::const_iterator fileiter = m_floodfile->m_files.begin();
    FloodFile::V_File::const_iterator fileend  = m_floodfile->m_files.end();

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

