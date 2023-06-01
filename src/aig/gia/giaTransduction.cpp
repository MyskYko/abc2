#ifdef _WIN32
#ifndef __MINGW32__
#pragma warning(disable : 4786) // warning C4786: identifier was truncated to '255' characters in the browser information
#endif
#endif

#include "giaTransduction.h"
#include "giaNewBdd.h"
#include "giaNewTt.h"

ABC_NAMESPACE_IMPL_START

Gia_Man_t *Gia_ManTransductionBdd(Gia_Man_t *pGia, int nType, int fMspf, int nRandom, int nSortType, int nPiShuffle, int nParameter, int fLevel, Gia_Man_t *pExdc, int fNewLine, int nVerbose) {
  if(nRandom) {
    unsigned seed = nRandom;
    for(int i = 0; i < 5; i++)
      seed = 36969 * (seed & 65535) + (seed >> 16);
    nSortType = seed % 4;
    seed = 36969 * (seed & 65535) + (seed >> 16);
    nPiShuffle = seed;
    seed = 36969 * (seed & 65535) + (seed >> 16);
    nParameter = seed % 16;
  }
  NewBdd::Param p;
  Transduction::Transduction<NewBdd::Man, NewBdd::Param, NewBdd::lit, 0xffffffff> t(pGia, nVerbose, fNewLine, nSortType, nPiShuffle, fLevel, pExdc, p);
  int count = t.CountWires();
  switch(nType) {
  case 0:
    count -= fMspf? t.Mspf(): t.Cspf();
    break;
  case 1:
    count -= t.Resub(fMspf);
    break;
  case 2:
    count -= t.ResubMono(fMspf);
    break;
  case 3:
    count -= t.ResubShared(fMspf);
    break;
  case 4:
    count -= t.RepeatResub(false, fMspf);
    break;
  case 5:
    count -= t.RepeatResub(true, fMspf);
    break;
  case 6: {
    bool fInner = (nParameter / 4) % 2;
    count -= t.RepeatInner(fMspf, fInner);
    break;
  }
  case 7: {
    bool fInner = (nParameter / 4) % 2;
    bool fOuter = (nParameter / 8) % 2;
    count -= t.RepeatOuter(fMspf, fInner, fOuter);
    break;
  }
  case 8: {
    bool fFirstMerge = nParameter % 2;
    bool fMspfMerge = fMspf? (nParameter / 2) % 2: false;
    bool fInner = (nParameter / 4) % 2;
    bool fOuter = (nParameter / 8) % 2;
    count -= t.RepeatAll(fFirstMerge, fMspfMerge, fMspf, fInner, fOuter);
    break;
  }
  default:
    std::cout << "Unknown transduction type " << nType << std::endl;
  }
  assert(t.Verify());
  assert(count == t.CountWires());
  return t.GenerateAig();
}

Gia_Man_t *Gia_ManTransductionTt(Gia_Man_t *pGia, int nType, int fMspf, int nRandom, int nSortType, int nPiShuffle, int nParameter, int fLevel, Gia_Man_t *pExdc, int fNewLine, int nVerbose) {
  if(nRandom) {
    unsigned seed = nRandom;
    for(int i = 0; i < 5; i++)
      seed = 36969 * (seed & 65535) + (seed >> 16);
    nSortType = seed % 4;
    seed = 36969 * (seed & 65535) + (seed >> 16);
    nPiShuffle = seed;
    seed = 36969 * (seed & 65535) + (seed >> 16);
    nParameter = seed % 16;
  }
  NewTt::Param p;
  Transduction::Transduction<NewTt::Man, NewTt::Param, NewTt::lit, 0xffffffff> t(pGia, nVerbose, fNewLine, nSortType, nPiShuffle, fLevel, pExdc, p);
  int count = t.CountWires();
  switch(nType) {
  case 0:
    count -= fMspf? t.Mspf(): t.Cspf();
    break;
  case 1:
    count -= t.Resub(fMspf);
    break;
  case 2:
    count -= t.ResubMono(fMspf);
    break;
  case 3:
    count -= t.ResubShared(fMspf);
    break;
  case 4:
    count -= t.RepeatResub(false, fMspf);
    break;
  case 5:
    count -= t.RepeatResub(true, fMspf);
    break;
  case 6: {
    bool fInner = (nParameter / 4) % 2;
    count -= t.RepeatInner(fMspf, fInner);
    break;
  }
  case 7: {
    bool fInner = (nParameter / 4) % 2;
    bool fOuter = (nParameter / 8) % 2;
    count -= t.RepeatOuter(fMspf, fInner, fOuter);
    break;
  }
  case 8: {
    bool fFirstMerge = nParameter % 2;
    bool fMspfMerge = fMspf? (nParameter / 2) % 2: false;
    bool fInner = (nParameter / 4) % 2;
    bool fOuter = (nParameter / 8) % 2;
    count -= t.RepeatAll(fFirstMerge, fMspfMerge, fMspf, fInner, fOuter);
    break;
  }
  default:
    std::cout << "Unknown transduction type " << nType << std::endl;
  }
  assert(t.Verify());
  assert(count == t.CountWires());
  return t.GenerateAig();
}

ABC_NAMESPACE_IMPL_END
