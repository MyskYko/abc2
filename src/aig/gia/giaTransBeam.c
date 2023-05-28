#include <base/abc/abc.h>
#include <aig/aig/aig.h>
#include <opt/dar/dar.h>
#include <aig/gia/gia.h>
#include <aig/gia/giaAig.h>
#include <base/main/main.h>
#include <base/main/mainInt.h>
#include <map/mio/mio.h>
#include <opt/sfm/sfm.h>
#include <opt/fxu/fxu.h>

#ifdef ABC_USE_PTHREADS

#ifdef _WIN32
#include "../lib/pthread.h"
#else
#include <pthread.h>
#include <unistd.h>
#endif

#endif

ABC_NAMESPACE_IMPL_START

/* Vec_Ptr_t * Gia_ManTranBeamTableAlloc( int nDepth ) { */
/*   int i; */
/*   Vec_Ptr_t * pTable = Vec_PtrAlloc( nDepth ); */
/*   for ( i = 0; i < nDepth; i++ ) */
/*     Vec_PtrPush( pTable, Vec_PtrAlloc( 1 ) ); */
/*   return pTable; */
/* } */

void Gia_ManTransBeamRegister( Vec_Ptr_t * pTable, Vec_Str_t * pNew, int index ) {
  int i;
  Vec_Str_t * pStr;
  Vec_Ptr_t * pRow = Vec_PtrGetEntry( pTable, index );
  if ( !pRow ) {
    pRow = Vec_PtrAlloc( 1 );
    Vec_PtrWriteEntry( pTable, index, pRow );
  }
  Vec_PtrForEachEntry( Vec_Str_t *, pRow, pStr, i )
    if ( Vec_StrEqual( pNew, pStr ) ) {
      Vec_StrFree( pNew );
      return;
    }
  Vec_PtrPush( pRow, pNew );
}

Vec_Str_t * Gia_ManTransBeamGacha( Vec_Ptr_t * pTable, int nMin, double progress ) {
  int i;
  Vec_Ptr_t * pRow;
  Vec_PtrForEachEntryStart( Vec_Ptr_t *, pTable, pRow, i, nMin )
    if ( pRow && 1.0 * rand() / RAND_MAX < progress)
      return Vec_PtrEntry( pRow, rand() % Vec_PtrSize( pRow ) );
  pRow = Vec_PtrEntryLast( pTable );
  return Vec_PtrEntry( pRow, rand() % Vec_PtrSize( pRow ) );
}

void Gia_ManTransBeamTableFree( Vec_Ptr_t * pTable ) {
  int i, j;
  Vec_Ptr_t * pRow;
  Vec_Str_t * pStr;
  Vec_PtrForEachEntry( Vec_Ptr_t *, pTable, pRow, i )
    if ( pRow ) {
      Vec_PtrForEachEntry( Vec_Str_t *, pRow, pStr, j )
        Vec_StrFree( pStr );
      Vec_PtrFree( pRow );
    }
  Vec_PtrFree( pTable );
}

typedef struct Gia_ManTransBeamParam_ Gia_ManTransBeamParam;
struct Gia_ManTransBeamParam_ {
  int nThreadId;
  int fWorking;
  Gia_Man_t * pStart;
  int nRandom;
  Vec_Str_t * pRes;
  int nResSize;
};

void * Gia_ManTransBeamWorkerThread( void * pArg ) {
  Gia_Man_t * pGia, * pTemp;
  Gia_ManTransBeamParam * p = (Gia_ManTransBeamParam *)pArg;
  volatile int * pPlace = &p->fWorking;
  while ( 1 ) {
    while ( *pPlace == 0 );
    assert( p->fWorking );
    if ( !p->pStart ) {
      pthread_exit( NULL );
      assert( 0 );
      return NULL;
    }
    if ( p->nThreadId ) {
      int nType = 8;//7 + (p->nRandom % 2);
      int fMspf = 1;//(p->nRandom >> 1) % 2;
      pGia = Gia_ManTransductionBdd( p->pStart, nType, fMspf, p->nRandom >> 2, 0, 0, 0, 0, NULL, 0, 0 );
    } else {
      extern Gia_Man_t * Gia_ManDeepSynOne( int nNoImpr, int TimeOut, int nAnds, int Seed, int fUseTwo, int fVerbose );
      Abc_FrameUpdateGia( Abc_FrameGetGlobalFrame(), Gia_ManDup(p->pStart) );
      //pGia = Gia_ManDeepSynOne( 1000000000, 1, 0, p->nRandom % 100, 0, 0 );
      char Command[100];
      char * pOpt;
      int nOpt = p->nRandom & 1;
      int fIfs = (p->nRandom >> 1) & 1;
      int fDch = (p->nRandom >> 2) & 1;
      int fFx  = (p->nRandom >> 3) & 1;
      int nLut = 3 + ((p->nRandom >> 4) % 4);
      switch ( nOpt ) {
      case 0:
        pOpt = "&dc2";
        break;
      case 1:
        pOpt = "&put; compress2rs; &get";
        break;
      }
      if ( fIfs )
        sprintf( Command, "&dch%s; &if -a -K %d; &mfs -e -W 20 -L 20;%s &st; %s",
                 fDch? " -f": "",
                 nLut,
                 fFx? " &fx;": "",
                 pOpt );
      else
        sprintf( Command, "%s", pOpt );
      //printf("%s\n", Command);
      Cmd_CommandExecute(Abc_FrameGetGlobalFrame(), Command);
      pGia = Gia_ManDup( Abc_FrameReadGia(Abc_FrameGetGlobalFrame()) );
    }
    pGia = Gia_ManIsoCanonicize( pTemp = pGia, 0 );
    Gia_ManStop( pTemp );
    p->nResSize = Gia_ManAndNum( pGia );
    p->pRes = Gia_AigerWriteIntoMemoryStr( pGia );
    Gia_ManStop( pGia );
    Gia_ManStop( p->pStart );
    p->pStart = NULL;
    p->fWorking = 0;
  }
  assert( 0 );
  return NULL;
}

Gia_Man_t * Gia_ManTransBeam( Gia_Man_t * pOrig, int nThreads, int nTimeOut ) {
  int i, nMin, status;
  Gia_Man_t * pGia, * pBest;
  Vec_Str_t * pStr;
  Vec_Ptr_t * pTable, * pTable2;
  pGia = Gia_ManIsoCanonicize( pOrig, 0 );
  nMin = Gia_ManAndNum( pGia );
  pStr = Gia_AigerWriteIntoMemoryStr( pGia );
  pTable = Vec_PtrAlloc( nMin + 1 );
  pTable2 = Vec_PtrAlloc( nMin + 1 );
  Gia_ManTransBeamRegister( pTable, pStr, nMin );
  Gia_ManTransBeamRegister( pTable2, Vec_StrDup( pStr ), nMin );
  pBest = pGia;

  Gia_ManTransBeamParam ThData[100];
  pthread_t WorkerThread[100];
  for ( i = 0; i < nThreads; i++ ) {
    ThData[i].nThreadId = i;
    ThData[i].fWorking = 0;
    ThData[i].pStart = NULL;
    ThData[i].pRes = NULL;
    status = pthread_create( WorkerThread + i, NULL, Gia_ManTransBeamWorkerThread, (void *)(ThData + i) );
    assert( status == 0 );
  }

  abctime startclk = Abc_Clock();
  while ( (Abc_Clock() - startclk) / CLOCKS_PER_SEC < nTimeOut ) {
    for ( i = 0; i < nThreads; i++ ) {
      if ( ThData[i].fWorking )
        continue;
      if ( ThData[i].pRes != NULL ) {
        if ( i )
          printf( "thread %2d: done #nodes = %5d\n", i, ThData[i].nResSize );
        if ( nMin > ThData[i].nResSize ) {
          Gia_ManStop( pBest );
          pBest = Gia_AigerReadFromMemory( Vec_StrArray(ThData[i].pRes), Vec_StrSize(ThData[i].pRes), 1, 1, 0 );
          nMin = ThData[i].nResSize;
          Gia_ManTransBeamTableFree( pTable2 );
          pTable2 = Vec_PtrAlloc( nMin + 1 );
          Gia_ManTransBeamRegister( pTable2, Vec_StrDup( ThData[i].pRes ), nMin );
        }
        if ( i )
          Gia_ManTransBeamRegister( pTable, ThData[i].pRes, ThData[i].nResSize );
        else
          Gia_ManTransBeamRegister( pTable2, ThData[i].pRes, ThData[i].nResSize );
        ThData[i].pRes = NULL;
      }
      if ( i )
        //pStr = Gia_ManTransBeamGacha( pTable2, nMin, 1.0 * (Abc_Clock() - startclk) / CLOCKS_PER_SEC / nTimeOut );
        pStr = Gia_ManTransBeamGacha( pTable2, nMin, 0.2 );
      else
        pStr = Gia_ManTransBeamGacha( pTable, nMin, 0.8 );
      ThData[i].pStart = Gia_AigerReadFromMemory( Vec_StrArray(pStr), Vec_StrSize(pStr), 1, 1, 0 );
      ThData[i].nRandom = rand();
      ThData[i].fWorking = 1;
      if ( i )
        printf( "thread %2d: start #nodes = %5d\n", i, Gia_ManAndNum( ThData[i].pStart ) );
    }
  }

  int fWorking = 1;
  while ( fWorking ) {
    fWorking = 0;
    for ( i = 0; i < nThreads; i++ ) {
      if( ThData[i].fWorking ) {
        fWorking = 1;
        continue;
      }
      if ( ThData[i].pRes != NULL ) {
        printf( "thread %2d: done #nodes = %5d\n", i, ThData[i].nResSize );
        if ( nMin > ThData[i].nResSize ) {
          Gia_ManStop( pBest );
          pBest = Gia_AigerReadFromMemory( Vec_StrArray(ThData[i].pRes), Vec_StrSize(ThData[i].pRes), 1, 1, 0 );
          nMin = ThData[i].nResSize;
        }
        Vec_StrFree( ThData[i].pRes );
        ThData[i].pRes = NULL;
      }
    }
  }
  for ( i = 0; i < nThreads; i++ ) {
    ThData[i].fWorking = 1;
  }

  Gia_ManTransBeamTableFree( pTable );
  Gia_ManTransBeamTableFree( pTable2 );
  return pBest;
}

ABC_NAMESPACE_IMPL_END
