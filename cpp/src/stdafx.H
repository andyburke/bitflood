// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef _LBF_STDAFX_H_
#define _LBF_STDAFX_H_

#include "Error.H"
#include "Types.H"
#include "SmartPointer.H"

// helpful forward declares
namespace libBitFlood
{
  typedef ReferenceCountTemplate< class Flood >          FloodSPtr;
  typedef ReferenceCountTemplate< class FloodFile >      FloodFileSPtr;
  typedef ReferenceCountTemplate< class PeerConnection > PeerConnectionSPtr;
  typedef ReferenceCountTemplate< class PeerPlugin >     PeerPluginSPtr;
  typedef ReferenceCountTemplate< class Peer >           PeerSPtr;

  typedef std::vector< FloodSPtr >          V_FloodSPtr;
  typedef std::vector< PeerConnectionSPtr > V_PeerConnectionSPtr;
  typedef std::vector< PeerPluginSPtr >     V_PeerPluginSPtr;

}

#endif _LBF_STDAFX_H_
