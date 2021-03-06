// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#include <iostream>
#include <tchar.h>
#include <Types.H>
#include <SmartPointer.H>
#include <Error.H>

// helpful forward declares
namespace libBitFlood
{
  typedef ReferenceCountTemplate< class Flood >          FloodSPtr;
  typedef ReferenceCountTemplate< class FloodFile >      FloodFileSPtr;
  typedef ReferenceCountTemplate< class PeerConnection > PeerConnectionSPtr;
  typedef ReferenceCountTemplate< class MethodHandler >  MethodHandlerSPtr;
  typedef ReferenceCountTemplate< class Peer >           PeerSPtr;

  typedef std::vector< FloodSPtr >          V_FloodSPtr;
  typedef std::vector< PeerConnectionSPtr > V_PeerConnectionSPtr;
  typedef std::vector< MethodHandlerSPtr >  V_MethodHandlerSPtr;

}

// TODO: reference additional headers your program requires here
