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
  const U32 MAX_CHUNKS_PER_PEER = 1;

  //
  Error::ErrorCode Flood::Initialize( PeerSPtr& i_localpeer, FloodFileSPtr& i_floodfile )
  {
    m_localpeer = i_localpeer.Get();
    m_floodfile = i_floodfile;

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

    {
      V_PeerConnectionSPtr::iterator peeriter = peers.begin();
      V_PeerConnectionSPtr::iterator peerend  = peers.end();

      for ( ; peeriter != peerend; ++peeriter )
      {
        // process
        (*peeriter)->LoopOnce();
      }
    }

    /// now reap peers
    {
      V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
      V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

      for ( ; peeriter != peerend; )
      {
        if ( (*peeriter)->m_disconnected )
        {
          printf( "Reaping %s\n", (*peeriter)->m_id.c_str() );
          peeriter = m_peers.erase( peeriter );
        }
        else
        {
          ++peeriter;
        }
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
        const FloodFile::FileSPtr& thefile = m_floodfile->m_files[ thechunkkey.first ];
        const FloodFile::Chunk& thechunk = thefile->m_chunks[ thechunkkey.second ];

        V_PeerConnectionSPtr::iterator peeriter = m_peers.begin();
        V_PeerConnectionSPtr::iterator peerend  = m_peers.end();

        for ( ; peeriter != peerend && !foundchunk; ++peeriter )
        {
          PeerConnectionSPtr& thepeer = (*peeriter);
          if ( thepeer->m_chunksdownloading < MAX_CHUNKS_PER_PEER )
          {
            const std::string& chunkmap = thepeer->m_chunkmaps[ thechunkkey.first ];
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

    if ( foundchunk )
    {
      XmlRpcValue args;
      args[0] = m_floodfile->m_files[ todownload_key->first ]->m_name;
      args[1] = (int)todownload_key->second;

      todownload_from->m_chunksdownloading++;
      todownload_from->SendMethod( ChunkMethodHandler::RequestChunk, args );

      ChunkDownload download;
      time( &download.m_start );
      download.m_from = todownload_from;

      m_chunksdownloading[ *todownload_key ] = download;
    }

    M_P_ChunkKeyToChunkDownload

    return Error::NO_ERROR_LBF;
  }

  Error::ErrorCode Flood::UpdateTrackers( void )
  {
    FloodFile::V_TrackerInfo::const_iterator trackeriter = m_floodfile->m_trackers.begin();
    FloodFile::V_TrackerInfo::const_iterator trackerend  = m_floodfile->m_trackers.end();
    for ( ; trackeriter != trackerend; ++trackeriter )
    {
      const FloodFile::TrackerInfo& tracker = (*trackeriter);

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

    FloodFile::M_StrToFileSPtr::const_iterator fileiter = m_floodfile->m_files.begin();
    FloodFile::M_StrToFileSPtr::const_iterator fileend  = m_floodfile->m_files.end();

    for ( ; fileiter != fileend; ++fileiter )
    {
      const FloodFile::FileSPtr& file = (*fileiter).second;

      // track the total size of the flood
      m_totalbytes += file->m_size;

      // setup runtime data
      RuntimeFile rtf;
      rtf.m_chunkoffsets = V_U32( file->m_chunks.size(), 0 );
      rtf.m_chunkmap = std::string( file->m_chunks.size(), '0' );
      rtf.m_file = file;

      FloodFile::V_Chunk::const_iterator chunkiter = file->m_chunks.begin();
      FloodFile::V_Chunk::const_iterator chunkend  = file->m_chunks.end();

      FILE* fileptr = fopen( file->m_name.c_str(), "rb" );

      U32 next_offset = 0;
      for ( ; chunkiter != chunkend; ++chunkiter )
      {
        const FloodFile::Chunk& chunk = *chunkiter;

        assert( rtf.m_chunkoffsets[ chunk.m_index ] == 0 );
        assert( rtf.m_chunkmap[ chunk.m_index ] == '0' );

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
          m_chunkstodownload.push_back( P_ChunkKey( file->m_name, chunk.m_index ) );
        }
      }

      // don't forget to close the file handle
      if ( fileptr )
      {
        fclose( fileptr );
      }

      // add this file data to our map
      m_runtimefiles[ file->m_name ] = rtf;
    }

    // done
    return Error::NO_ERROR_LBF;
  }
};

