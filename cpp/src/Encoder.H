#ifndef _LBF_ENCODER_H_
#define _LBF_ENCODER_H_

namespace libBitFlood
{
  // forward declare
  class FloodFile;

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

      typedef std::pair< std::string, U32> Tracker;
      typedef std::vector< Tracker > V_Tracker;
      V_Tracker m_trackers;
    };

    Error::ErrorCode EncodeFile( const ToEncode& i_toencode, FloodFile& o_xml );
    Error::ErrorCode Base64Encode( const U8* i_data, U32 i_size, std::string& o_string );
  }
};

#endif; // _LBF_ENCODER_H_
