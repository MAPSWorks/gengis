//=======================================================================
// Author: Alexander Keddy
//
// Copyright 2013 Alexander Keddy
//
// This file is part of GenGIS.
//
// GenGIS is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GenGIS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GenGIS.  If not, see <http://www.gnu.org/licenses/>.
//=======================================================================
#ifndef _LOCATION_MERGE_DLG_
#define _LOCATION_MERGE_DLG_

#include "../gui/LocationMergeLayout.h"

namespace GenGIS
{
	class LocationMergeDlg : public LocationMergeLayout
	{
	public:
		/**
		* @brief Constructor.
		*/
		explicit LocationMergeDlg(wxWindow* parent);

		/** Destructor **/
		~LocationMergeDlg();
	private:
		/** Initialize control values. */
		void Init();

		/** OK button event handler. */
		void OnOK( wxCommandEvent& event );

		/**Cancel button event handler. */
		void OnCancel( wxCommandEvent& event ){ Destroy();}

		/** Close dialog. */
		void OnClose( wxCommandEvent& event ) { Destroy(); }

		/** Combines multiple location sets into one */
		void LocationMergeDlg::CreateLocationSet( std::vector<LocationSetLayerPtr> LocationSets );

		void LocationMergeDlg::CreateLocationSet( std::vector<LocationModelPtr> locationModels, std::vector<ChartSetViewPtr> ChartViews );

		void LocationMergeDlg::CreateSequenceSet( std::vector<SequenceModelPtr> sequenceModels );
	};
}

#endif 