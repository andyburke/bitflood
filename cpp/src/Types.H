#ifndef _LBF_TYPES_H_
#define _LBF_TYPES_H_

#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

namespace libBitFlood
{
  // 8 bytes
  typedef signed char I8;
  typedef std::vector<I8> V_I8;
  typedef std::set<I8> S_I8;

  typedef unsigned char U8;
  typedef std::vector<U8> V_U8;
  typedef std::set<U8> S_U8;

  // 16 bytes
  typedef signed short I16;
  typedef std::vector<I16> V_I16;
  typedef std::set<I16> S_I16;

  typedef unsigned short U16;
  typedef std::vector<U16> V_U16;
  typedef std::set<U16> S_U16;

  // 32 bytes
  typedef signed int I32;
  typedef std::vector<I32> V_I32;
  typedef std::set<I32> S_I32;

  typedef unsigned int U32;
  typedef std::vector<U32> V_U32;
  typedef std::set<U32> S_U32;
  
  // 64 bytes
  typedef signed long long I64;
  typedef std::vector<I64> V_I64;
  typedef std::set<I64> S_I64;

  typedef unsigned long long U64;
  typedef std::vector<U64> V_U64;
  typedef std::set<U64> S_U64;

  // other helpful typedefs
  typedef std::vector<std::string> V_String;
  typedef std::set<std::string> S_String;

  typedef std::vector<std::wstring> V_WString;
  typedef std::set<std::wstring> S_WString;

  typedef std::vector<bool> V_Bool;
};

#endif; // _LBF_TYPES_H_
