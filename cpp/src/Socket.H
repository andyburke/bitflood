#ifndef _LBF_SOCKET_H_
#define _LBF_SOCKET_H_

namespace libBitFlood
{
  // a pure virtual socket interface
  typedef ReferenceCountTemplate< class Socket > SocketSPtr;
  class Socket : public ReferenceCountBase
  {
  public:
    virtual bool Connect( const std::string& i_host, U32 i_port ) = 0;
    virtual void Close( void ) = 0;

    virtual bool Read( U32 i_windowsize, U8* o_buffer, I32& o_bytesread ) = 0;
    virtual bool Write( U32 i_buffersize, const U8* i_buffer, I32& o_byteswritten ) = 0;
    
    virtual bool SetNonBlocking( void ) = 0;
    virtual bool SetReuse( void ) = 0;

    virtual bool Bind( U32 i_port ) = 0;
    virtual bool Listen( U32 i_backlog ) = 0;
    virtual bool Accept( SocketSPtr& o_socket, std::string& o_host, U32& o_port ) = 0;

    virtual bool CanRead( U32 i_timeout ) = 0;
    virtual bool CanWrite( U32 i_timeout ) = 0;
    
  public:
    static bool CreateSocket( SocketSPtr& o_socket );
    ~Socket( void ) {}

  protected:
    Socket( void ) {}
  };



  class Win32Socket : public Socket
  {
  public:
    Win32Socket( void );
    ~Win32Socket( void );

    bool Connect( const std::string& i_host, U32 i_port ) ;
    void Close( void );

    bool Read( U32 i_windowsize, U8* o_buffer, I32& o_bytesread );
    bool Write( U32 i_buffersize, const U8* i_buffer, I32& o_byteswritten );
    
    bool SetNonBlocking( void );
    bool SetReuse( void );

    bool Bind( U32 i_port );
    bool Listen( U32 i_backlog );
    bool Accept( SocketSPtr& o_socket, std::string& o_host, U32& o_port );

    bool CanRead( U32 i_timeout );
    bool CanWrite( U32 i_timeout );

    U32 m_handle;
  };
};

#endif; // _LBF_SOCKET_H_
