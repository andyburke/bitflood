#ifndef _LBF_ENCODER_H_
#define _LBF_ENCODER_H_

namespace libBitFlood
{
  // the encoder takes as input:
  //  - a list of files
  //  - a tracker
  //  - a chunksize
  //  - a weighting function
  // and returns xml describing chunks and the tracker
  namespace Encoder
  {
    struct ToEncode
    {
      V_String m_files;
      U32 m_chunksize;
    };

    Error::ErrorCode EncodeFile( const ToEncode& i_toencode, std::string& o_xml );
  }
};

#endif; // _LBF_ENCODER_H_
