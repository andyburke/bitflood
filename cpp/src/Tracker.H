#ifndef _LBF_TRACKER_H_
#define _LBF_TRACKER_H_

#include <XmlRpc.h>

using namespace XmlRpc;

namespace libBitFlood
{
  class Tracker : public XmlRpcServer
  {
  public:
    Error::ErrorCode Initialize( U32 port );
    Error::ErrorCode Run( double time );

    typedef std::vector< XmlRpcServerMethod* > V_ServerMethodPtr;
    V_ServerMethodPtr m_methods;

    struct PeerInfo
    {
      std::string m_id;
      std::string m_ip;
      U32 m_port;
      time_t m_timestamp;
    };

    typedef std::vector< PeerInfo > V_PeerInfo;
    typedef std::map< std::string, V_PeerInfo > M_StrToPeerInfo;

    M_StrToPeerInfo m_peerinfo;

  protected:
    class _Register : public XmlRpcServerMethod
    {
    public:
      _Register(XmlRpcServer* s);
      void execute(XmlRpcValue& params, XmlRpcValue& result);
    };

    class _Disconnect : public XmlRpcServerMethod
    {
    public:
      _Disconnect(XmlRpcServer* s);
      void execute(XmlRpcValue& params, XmlRpcValue& result);
    };

    class _Dump : public XmlRpcServerMethod
    {
    public:
      _Dump(XmlRpcServer* s);
      void execute(XmlRpcValue& params, XmlRpcValue& result);
    };

    class _RequestPeers : public XmlRpcServerMethod
    {
    public:
      _RequestPeers(XmlRpcServer* s);
      void execute(XmlRpcValue& params, XmlRpcValue& result);
    };
  };
};

#endif; // _LBF_TRACKER_H_
