#include "sps/desc.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <set>
#include <tuple>

#pragma once

namespace cm
{

template <template <typename> typename vec_gen_t> class AnnotationDescIndex
{
    struct Interval
    {
        size_t uiIntervalId;
        size_t uiIntervalStart;
        size_t uiIntervalEnd;
        size_t uiAnnoStart;
        size_t uiAnnoEnd;
        size_t uiDescId;
        bool bForwStrnd;
        size_t uiIntervalCoordsStart;
        size_t uiIntervalCoordsEnd;
        size_t uiAnnoCoordsStart;
        size_t uiAnnoCoordsEnd;
    };

    struct Dataset
    {
        size_t uiStart;
        size_t uiEnd;
    };

    typename sps::DescImpl<vec_gen_t> vDesc;
    vec_gen_t<Interval> xIntervalVecGen = vec_gen_t<Interval>( );
    typename vec_gen_t<Interval>::file_t xIntervalFile;
    typename vec_gen_t<Interval>::vec_t vIntervals;

    vec_gen_t<Dataset> xDatasetVecGen = vec_gen_t<Dataset>( );
    typename vec_gen_t<Dataset>::file_t xDatasetFile;
    typename vec_gen_t<Dataset>::vec_t vDatasets;


  public:
    AnnotationDescIndex( std::string sPrefix, bool bWrite )
        : vDesc( sPrefix, bWrite ),

          xIntervalFile( xIntervalVecGen.file( sPrefix + ".intervals", bWrite ) ),
          vIntervals( xIntervalVecGen.vec( xIntervalFile ) ),

          xDatasetFile( xDatasetVecGen.file( sPrefix + ".datasets", bWrite ) ),
          vDatasets( xDatasetVecGen.vec( xDatasetFile ) )
    {}

    AnnotationDescIndex( )
    {}

    size_t addIntervals( std::vector<std::tuple<size_t, size_t, std::string, bool>> vIntervalsIn, size_t uiDividend,
                         size_t /*uiVerbosity = 0*/ )
    {
        size_t uiStartSize = vIntervals.size( );
        std::vector<std::tuple<size_t, size_t, bool>> vLineSweepPos;
        std::vector<size_t> vDescID;
        vDescID.resize( vIntervalsIn.size( ) );
        for( size_t uiI = 0; uiI < vIntervalsIn.size( ); uiI++ )
        {
            vLineSweepPos.push_back( std::make_tuple(
                (size_t)std::round( std::get<0>( vIntervalsIn[ uiI ] ) / (double)uiDividend ), uiI, false ) );
            vLineSweepPos.push_back( std::make_tuple(
                (size_t)std::round( std::get<1>( vIntervalsIn[ uiI ] ) / (double)uiDividend ), uiI, true ) );
            vDescID[ uiI ] = vDesc.add( std::get<2>( vIntervalsIn[ uiI ] ) );
        }
        std::sort( vLineSweepPos.begin( ), vLineSweepPos.end( ) );

        size_t uiIntervalCoordPos = 0;
        size_t uiSkippedCoords = 0;
        size_t uiIntervalId = 0;
        std::set<size_t> xActiveIntervals;
        size_t uiLastPos = 0;
        for( size_t uiJ = 0; uiJ < vLineSweepPos.size( ); )
        {
            size_t uiCurrPos = std::get<0>( vLineSweepPos[ uiJ ] );
            while( uiJ < vLineSweepPos.size( ) && std::get<0>( vLineSweepPos[ uiJ ] ) == uiCurrPos )
            {
                if( !std::get<2>( vLineSweepPos[ uiJ ] ) )
                    // interval start
                    xActiveIntervals.insert( std::get<1>( vLineSweepPos[ uiJ ] ) );
                else /*if(std::get<1>(vIntervalsIn[vLineSweepPos[uiJ].second]) + 1 == uiCurrPos)*/
                {
                    assert( xActiveIntervals.count( std::get<1>( vLineSweepPos[ uiJ ] ) ) > 0 );
                    // interval end
                    xActiveIntervals.erase( std::get<1>( vLineSweepPos[ uiJ ] ) );
                }
                uiJ++;
            }

            assert( uiJ < vLineSweepPos.size( ) || xActiveIntervals.size( ) == 0 );

            if( xActiveIntervals.size( ) > 0 )
            {
                uiSkippedCoords += uiCurrPos - uiLastPos;
                size_t uiNextPos = std::get<0>( vLineSweepPos[ uiJ ] );

                for( size_t uiActive : xActiveIntervals )
                    vIntervals.push_back( Interval{
                        .uiIntervalId = uiIntervalId,

                        .uiIntervalStart = uiCurrPos,
                        .uiIntervalEnd = uiNextPos,

                        .uiAnnoStart = std::get<0>( vIntervalsIn[ uiActive ] ),
                        .uiAnnoEnd = std::get<1>( vIntervalsIn[ uiActive ] ),

                        .uiDescId = vDescID[ uiActive ],
                        .bForwStrnd = std::get<3>( vIntervalsIn[ uiActive ] ),

                        .uiIntervalCoordsStart = uiIntervalCoordPos,
                        .uiIntervalCoordsEnd = uiIntervalCoordPos + ( uiNextPos - uiCurrPos ),

                        .uiAnnoCoordsStart = std::get<0>( vIntervalsIn[ uiActive ] ) - uiSkippedCoords * uiDividend,
                        .uiAnnoCoordsEnd = std::get<1>( vIntervalsIn[ uiActive ] ) - uiSkippedCoords * uiDividend,
                    } );
                uiIntervalCoordPos += ( uiNextPos - uiCurrPos );
                ++uiIntervalId;
                uiLastPos = uiNextPos;
            }
        }
        vDatasets.push_back( Dataset{ .uiStart = uiStartSize, .uiEnd = vIntervals.size( ) } );
        return vDatasets.size( ) - 1;
    }

    std::string desc( const Interval& rI )
    {
        return vDesc.get( rI.uiDescId );
    }

    using interval_it_t = typename vec_gen_t<Interval>::vec_t::iterator;
    void iterate( interval_it_t xStart, interval_it_t xEnd,
                  std::function<bool( std::tuple<size_t, size_t, std::string, bool> )> fYield,
                  bool bIntervalCoords = false, bool bIntervalCount = false )
    {
        assert( !( bIntervalCoords && bIntervalCount ) );
        std::set<size_t> xActiveIntervals{ };

        assert( xStart <= xEnd );

        while( xStart != xEnd )
        {
            if( xActiveIntervals.count( xStart->uiDescId ) == 0 )
            {
                xActiveIntervals.insert( xStart->uiDescId );
                if( !fYield( std::make_tuple(
                        bIntervalCoords ? xStart->uiAnnoCoordsStart
                                        : ( bIntervalCount ? xStart->uiIntervalId : xStart->uiAnnoStart ),
                        bIntervalCoords ? xStart->uiAnnoCoordsEnd
                                        : ( bIntervalCount ? xStart->uiIntervalId : xStart->uiAnnoEnd ),
                        desc( *xStart ),
                        xStart->bForwStrnd ) ) )
                    return;
            }
            ++xStart;
        }
    }

    void iterate( size_t uiDatasetId, size_t uiFrom, size_t uiTo,
                  std::function<bool( std::tuple<size_t, size_t, std::string, bool> )> fYield,
                  bool bIntervalCoords = false, bool bIntervalCount = false )
    {
        auto xStart = lowerBound( uiDatasetId, uiFrom, bIntervalCoords, bIntervalCount );
        auto xEnd = upperBound( uiDatasetId, uiTo, bIntervalCoords, bIntervalCount );
        iterate( xStart, xEnd, fYield, bIntervalCoords, bIntervalCount );
    }

    void iterate( size_t uiDatasetId, std::function<bool( std::tuple<size_t, size_t, std::string, bool> )> fYield,
                  bool bIntervalCoords = false, bool bIntervalCount = false )
    {
        auto xStart = begin( uiDatasetId );
        auto xEnd = end( uiDatasetId );
        iterate( xStart, xEnd, fYield, bIntervalCoords, bIntervalCount );
    }

    std::vector<std::tuple<size_t, size_t, std::string, bool>>
    query( size_t uiDatasetId, size_t uiFrom, size_t uiTo, bool bIntervalCoords = false, bool bIntervalCount = false )
    {
        assert( !( bIntervalCoords && bIntervalCount ) );
        std::vector<std::tuple<size_t, size_t, std::string, bool>> vRet = { };

        iterate(
            uiDatasetId, uiFrom, uiTo,
            [ & ]( std::tuple<size_t, size_t, std::string, bool> xTup ) {
                vRet.push_back( xTup );
                return true;
            },
            bIntervalCoords, bIntervalCount );

        std::sort( vRet.begin( ), vRet.end( ) );

        return vRet;
    }

    interval_it_t begin( size_t uiDatasetId )
    {
        return vIntervals.begin( ) + vDatasets[ uiDatasetId ].uiStart;
    }
    interval_it_t end( size_t uiDatasetId )
    {
        return vIntervals.begin( ) + vDatasets[ uiDatasetId ].uiEnd;
    }

    interval_it_t lowerBound( size_t uiDatasetId, size_t uiFrom, bool bIntervalCoords = false,
                              bool bIntervalCount = false )
    {
        assert( !( bIntervalCoords && bIntervalCount ) );

        auto xBegin = begin( uiDatasetId );
        auto xEnd = end( uiDatasetId );

        return std::lower_bound(
            xBegin, xEnd, uiFrom,
            bIntervalCoords  ? []( const Interval& rI, size_t uiFrom ) { return rI.uiIntervalCoordsEnd <= uiFrom; }
            : bIntervalCount ? []( const Interval& rI, size_t uiFrom ) { return rI.uiIntervalId < uiFrom; }
                             : []( const Interval& rI, size_t uiFrom ) { return rI.uiIntervalEnd <= uiFrom; } );
    }
    interval_it_t upperBound( size_t uiDatasetId, size_t uiTo, bool bIntervalCoords = false,
                              bool bIntervalCount = false )
    {
        assert( !( bIntervalCoords && bIntervalCount ) );

        auto xBegin = begin( uiDatasetId );
        auto xEnd = end( uiDatasetId );

        return std::upper_bound(
            xBegin, xEnd, uiTo,
            bIntervalCoords  ? []( size_t uiTo, const Interval& rI ) { return uiTo <= rI.uiIntervalCoordsStart; }
            : bIntervalCount ? []( size_t uiTo, const Interval& rI ) { return uiTo <= rI.uiIntervalId; }
                             : []( size_t uiTo, const Interval& rI ) { return uiTo <= rI.uiIntervalStart; } );
    }

    std::array<interval_it_t, 2> getRange( size_t uiDatasetId, size_t uiFrom, size_t uiTo, bool bIntervalCoords = false,
                                           bool bIntervalCount = false )
    {
        assert( !( bIntervalCoords && bIntervalCount ) );

        auto xStart = lowerBound( uiDatasetId, uiFrom, bIntervalCoords, bIntervalCount );
        auto xStop = upperBound( uiDatasetId, uiTo, bIntervalCoords, bIntervalCount );

        return std::array<interval_it_t, 2>{ xStart, xStop };
    }

    size_t count( size_t uiDatasetId, size_t uiFrom, size_t uiTo, bool bIntervalCoords = false,
                  bool bIntervalCount = false )
    {
        auto xRange = getRange( uiDatasetId, uiFrom, uiTo, bIntervalCoords, bIntervalCount );

        if( xRange[ 1 ] < xRange[ 0 ] )
            return 0;
        return xRange[ 1 ] - xRange[ 0 ];
    }

    std::vector<bool> getCategories( size_t uiFrom, size_t uiTo, size_t uiDividend, std::vector<int> vCats,
                                     bool bIntervalCoords = false, bool bIntervalCount = false )
    {
        std::vector<bool> vRet;
        vRet.reserve( vCats.size( ) );
        for( int uiDatasetId : vCats )
        {
            bool bFound = false;
            if(uiDatasetId > 0)
                iterate(
                    uiDatasetId, (size_t)std::round( uiFrom / (double)uiDividend ),
                    (size_t)std::round( uiTo / (double)uiDividend ),
                    [ & ]( std::tuple<size_t, size_t, std::string, bool> xTup ) {
                        if( std::get<0>( xTup ) <= uiTo && std::get<1>( xTup ) > uiFrom )
                            bFound = true;
                        return !bFound;
                    },
                    bIntervalCoords, bIntervalCount );
            vRet.push_back( bFound );
        }
        return vRet;
    }

    size_t totalIntervalSize( size_t uiDatasetId )
    {
        auto xBegin = begin( uiDatasetId );
        auto xEnd = end( uiDatasetId ) - 1;

        return xEnd->uiIntervalCoordsEnd - xBegin->uiIntervalCoordsStart;
    }

    size_t numIntervals( size_t uiDatasetId )
    {
        auto xEnd = end( uiDatasetId ) - 1;

        return xEnd->uiIntervalId + 1;
    }
};

} // namespace cm
