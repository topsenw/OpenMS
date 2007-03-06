// -*- C++: make; tab-width: 2; -*-
// vi: set ts=2:
//
// --------------------------------------------------------------------------
//                   OpenMS Mass Spectrometry Framework
// --------------------------------------------------------------------------
//  Copyright (C) 2003-2007 -- Oliver Kohlbacher, Knut Reinert
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// --------------------------------------------------------------------------
// $Maintainer: Ole Schulz-Trieglaff$
// --------------------------------------------------------------------------

#ifndef OPENMS_TRANSFORMATIONS_FEATUREFINDER_MARRWAVELETSEEDER_H
#define OPENMS_TRANSFORMATIONS_FEATUREFINDER_MARRWAVELETSEEDER_H

#include <OpenMS/TRANSFORMATIONS/FEATUREFINDER/BaseSeeder.h>
#include <OpenMS/TRANSFORMATIONS/FEATUREFINDER/FeaFiTraits.h>
#include <OpenMS/TRANSFORMATIONS/RAW2PEAK/ContinuousWaveletTransformNumIntegration.h>

#include <OpenMS/DATASTRUCTURES/IsotopeCluster.h>

#include <OpenMS/CONCEPT/Exception.h>

namespace OpenMS
{
	/** 
		@brief Seeding module based on the Marr wavelet transform to detect (poorly resolved) isotopic pattern.
	  
 		Uses the continuous wavelet transform (and the Marr mother wavelet) to detect isotopic pattern
		in each scan. Patterns that occur in several consecutive scans are declared as seeding regions
		for the extension phase.
		
		The algorithm considers local maxima in the wavelet transform signal and checks for maxima
		with a distance corresponding to isotopic pattern (e.g. 1 Th, 1/2 Th etc).
		
		<table>
		 <tr><td></td><td></td><td>charge1_ub</td>
		 <td>upper bound for the distance between "charge one" maxima</td></tr>
		 <tr><td></td><td></td><td>charge1_lb</td>
		 <td>lower bound for charge one </td></tr>
		 <tr><td></td><td></td><td>charge2_ub</td>
		 <td>upper bound charge two </td></tr>
		 <tr><td></td><td></td><td>charge2_lb</td>
		 <td>lower bound charge two </td></tr>
		 <tr><td></td><td></td><td>charge3_ub</td>
		 <td>upper bound charge three </td></tr>
		 <tr><td></td><td></td><td>charge3_lb</td>
		 <td>lower bound charge three </td></tr>
		 <tr><td></td><td></td><td>tolerance_mz</td>
		 <td>mass tolerance for isotopic pattern in adjacent scans </td></tr>
		 <tr><td></td><td></td><td>cwt_scale</td>
		 <td>scale of Marr wavelet </td></tr>
		 <tr><td></td><td></td><td>noise_level_signal</td>
		 <td>intensity threshold for points in the current scan to be considered </td></tr>
		 <tr><td></td><td></td><td>noise_level_cwt</td>
		 <td>same as above for the wavelet transformed signal. </td></tr>
		 <tr><td></td><td></td><td>scans_to_sumup</td>
		 <td>number of scans for alignment </td></tr>
		 <tr><td></td><td></td><td>mass_tolerance_alignment</td>
		 <td>mass tolerance applied during alignment</td></tr>
		 <tr><td></td><td></td><td>min_number_scans</td>
		 <td>lower bound for the number of scans in which a isotopic pattern must occur 
		  before it is accepted as seed</td></tr>
		 <tr><td></td><td></td><td>min_number_peaks</td>
		 <td>min. number of data points for a seeding region</td></tr>
		  </table>		
		
		
		@ingroup FeatureFinder
	*/ 
  class MarrWaveletSeeder 
    : public BaseSeeder
  {
  	public:	
			typedef FeaFiTraits::IntensityType IntensityType;
		  typedef FeaFiTraits::CoordinateType CoordinateType;
		  typedef DoubleReal ProbabilityType;	
	
			typedef FeaFiTraits::MapType MapType;
			typedef MapType::SpectrumType SpectrumType;
			typedef	MapType::PeakType PeakType;
			typedef std::multimap<CoordinateType,IsotopeCluster> TableType;
	
		  /// Default constructor
	    MarrWaveletSeeder();
	
	    /// destructor 
	    virtual ~MarrWaveletSeeder();

	    /// Copy constructor
	    MarrWaveletSeeder(const MarrWaveletSeeder& rhs);
	    
	    /// Assignment operator
	    MarrWaveletSeeder& operator= (const MarrWaveletSeeder& rhs);
	
	    /// return next seed 
	    IndexSet nextSeed() throw (NoSuccessor);
	
	    static BaseSeeder* create()
	    {
	      return new MarrWaveletSeeder();
	    }
	
	    static const String getProductName()
	    {
	      return "MarrWaveletSeeder";
	    }
	
	  protected:
			
	  	virtual void updateMembers_();
	  	
	    /// Finds the neighbour of the peak denoted by @p current_mz in the previous scan
	    std::vector<double>::const_iterator searchInScan_(const std::vector<CoordinateType>::const_iterator& scan_begin, const std::vector<CoordinateType>::const_iterator& scan_end, CoordinateType current_mz)
	    {
	      // perform binary search to find the neighbour in rt dimension
	      // 	lower_bound finds the peak with m/z current_mz or the next larger peak if this peak does not exist.
	      std::vector<CoordinateType>::const_iterator insert_iter = lower_bound(scan_begin,scan_end,current_mz);
	
	      // the peak found by lower_bound does not have to be the closest one, therefore we have
	      // to check both neighbours
	      if ( insert_iter == scan_end ) // we are at the and have only one choice
	      {
	      	--insert_iter;
	      }
        // if the found peak is at the beginning of the spectrum,
        // there is not much we can do.
        else if ( insert_iter != scan_begin )
        {
          if ( *insert_iter - current_mz < current_mz - *(--insert_iter) )
          {
          	++insert_iter;    // peak to the right is closer
          }
	      }
				return insert_iter;
	    }

			/// Finds local maxima in the cwt
			void getMaxPositions_( const SpectrumType::const_iterator& first, 
														 const SpectrumType::const_iterator& last, 
														 const ContinuousWaveletTransform& wt,
														 std::vector<int>& localmax
#ifdef DEBUG_FEATUREFINDER
														 ,CoordinateType curr_peak
#endif
														 );
		 
		  /// Sums the intensities in adjacent scans
		  void sumUp_(SpectrumType& scan, UInt current_scan_index);
			
			///Aligns the two scans and increases intensities of peaks in @p scan if those peaks are present in @p neighbour
			void AlignAndSum_(SpectrumType& scan, const SpectrumType& neighbour);
		 	
	    /// Find out which charge state belongs to this distance
	    UInt distanceToCharge_(CoordinateType dist);
		
		  /// Sweeps through scans and detects isotopic patterns
		  void sweep_();
		
		  /// stores the retention time of each isotopic cluster
		  TableType iso_map_;
		
		  /// Pointer to the current region
		  TableType::const_iterator curr_region_;
		
		  /// indicates whether the extender has been initialized
		  bool is_initialized_;
			
			/// upper bound for distance between charge 1 peaks
			CoordinateType charge1_ub_;
			/// lower bound for distance between charge 1 peaks
			CoordinateType charge1_lb_;
			
			/// upper bound for distance between charge 2 peaks
			CoordinateType charge2_ub_;
			/// lower bound for distance between charge 2 peaks
			CoordinateType charge2_lb_;	
			
			/// upper bound for distance between charge 3 peaks
			CoordinateType charge3_ub_;
			/// lower bound for distance between charge 3 peaks
			CoordinateType charge3_lb_;	
			
			/// upper bound for distance between charge 4 peaks
			CoordinateType charge4_ub_;
			/// lower bound for distance between charge 4 peaks
			CoordinateType charge4_lb_;	
			
			/// upper bound for distance between charge 5 peaks
			CoordinateType charge5_ub_;
			/// lower bound for distance between charge 4 peaks
			CoordinateType charge5_lb_;
			
			/// Computes the wavelet transform for a given scan
			ContinuousWaveletTransformNumIntegration cwt_;
						
			/// Minimum ion count
			IntensityType noise_level_signal_;
		
	   	/// The min. intensity in the cwt 
	   	IntensityType noise_level_cwt_;
			
			/// Mass tolerance during scan alignment
			CoordinateType mass_tolerance_alignment_;
	 
  };
}
#endif // OPENMS_TRANSFORMATIONS_FEATUREFINDER_MARRWAVELETSEEDER_H
