#ifndef _LBF_SMART_POINTER_H_
#define _LBF_SMART_POINTER_H_

#include <assert.h>

namespace libBitFlood
{
  template<class T> class ReferenceCountTemplate
  {
  public:
    explicit ReferenceCountTemplate( T* i_pointer = NULL );
    ReferenceCountTemplate( const ReferenceCountTemplate<T>& i_rhs );
    ~ReferenceCountTemplate( void );
    
    const T& operator*( void ) const { assert( m_pointer ); return* m_pointer; }
    T& operator*( void ) { assert( m_pointer ); return* m_pointer; }

    const T* operator->( void ) const { return m_pointer; }
    T* operator->( void ) { return m_pointer; }

    const T* Get() const { return m_pointer; }
    T* Get() { return m_pointer; }
    
    ReferenceCountTemplate<T>& operator=(const ReferenceCountTemplate<T>& i_rhs );
    
  private:
    T* m_pointer;
  };
  
  template <class T> ReferenceCountTemplate<T>::ReferenceCountTemplate( T *i_pointer )
    : m_pointer( i_pointer ) 
  {
    if ( m_pointer )
    {
      m_pointer->m_referencecount++;
    }
  }
  
  template <class T> ReferenceCountTemplate<T>::ReferenceCountTemplate(const ReferenceCountTemplate<T>& i_rhs)
    : m_pointer( i_rhs.m_pointer )
  {
    if (m_pointer)
    {
      m_pointer->m_referencecount++;
    }
  }
  
  template <class T> ReferenceCountTemplate<T>::~ReferenceCountTemplate()
  {
    if ( m_pointer && --m_pointer->m_referencecount == 0 )
    {
      delete m_pointer;
    }

    m_pointer = NULL;
  }
  
  template <class T> ReferenceCountTemplate<T> & ReferenceCountTemplate<T>::operator=(const ReferenceCountTemplate<T>& i_rhs)
  {
    if ( this != &i_rhs )
    {
      if (m_pointer != i_rhs.m_pointer)
      {
        if (m_pointer && --m_pointer->m_referencecount == 0)
        {
          delete m_pointer;
        }
        
        m_pointer = i_rhs.m_pointer;
        
        if (m_pointer)
        {
          m_pointer->m_referencecount++;
        }
      }
    }
      
    return *this;
  }

  class ReferenceCountBase
  {
  public:
    U32 m_referencecount;

  protected:
    ReferenceCountBase( void )
      : m_referencecount ( 0 ) {}
    ~ReferenceCountBase( void ) { assert( m_referencecount == 0 ); }
  };

};

#endif // _LBF_SMART_POINTER_H_
