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

#include "../core/Precompiled.hpp"
#include "../gui/LocationMergeDlg.hpp"
#include "../core/App.hpp"
#include "../utils/Log.hpp"
#include "../core/LocationSetLayer.hpp"
#include "../core/LocationLayer.hpp"
#include "../core/LayerTreeController.hpp"
#include "../core/LocationSetController.hpp"
#include "../core/StudyLayer.hpp"
#include "../core/SequenceLayer.hpp"
#include "../utils/ColourMapManager.hpp"
#include "../core/ChartSetView.hpp"

#include "../utils/StringTools.hpp"
#include "../utils/UniqueId.hpp"

using namespace GenGIS;

LocationMergeDlg::LocationMergeDlg(wxWindow* parent): LocationMergeLayout(parent)
{
	SetIcon( wxIcon( App::Inst().GetExeDir() + wxT( "images/CrazyEye.ico" ), wxBITMAP_TYPE_ICO ) );

	Init();
	Fit();

}

LocationMergeDlg::~LocationMergeDlg()
{

}

void LocationMergeDlg::Init()
{
	//get location layers
	//for each location layer create a checkbox for them
	LayerTreeControllerPtr layerTree = App::Inst().GetLayerTreeController();
	for (uint locSet = 0; locSet < layerTree->GetNumLocationSetLayers(); locSet++)
	{
		std::wstring layerName = layerTree->GetLocationSetLayer( locSet )->GetName();
		wxString const mystring(layerName);
		m_locationSetCheckbox->InsertItems(1,&mystring,0);
	}
	
}
/**
void LocationMergeDlg::OnOK(wxCommandEvent& event)
{
	int numItems = m_locationSetCheckbox->GetCount();
	if( numItems > 0)
	{
		int * checkedIndexes = new int [numItems];
		LayerTreeControllerPtr layerTree = App::Inst().GetLayerTreeController();
		std::vector<LocationSetLayerPtr> LocationSets;
		//Finds the indexes of all checked boxes
		for(int locSet = 0; locSet < numItems; locSet++)
		{
			if(m_locationSetCheckbox->IsChecked(locSet))
			{
				LocationSetLayerPtr locationSet = layerTree->GetLocationSetLayer( locSet );
				LocationSets.push_back(locationSet);
				for(uint i = 0; i < locationSet->GetNumLocationLayers(); i++)
				{
					std::vector<LocationLayerPtr> locations = locationSet->GetAllActiveLocationLayers();
				}
			}
		}
		CreateLocationSet(LocationSets);

		Destroy();
	}
}

void LocationMergeDlg::CreateLocationSet( std::vector<LocationSetLayerPtr> LocationSets )
{
	// stop refreshing the tree until all sequences are loaded (this is purely for efficency)
	App::Inst().GetLayerTreeController()->GetTreeCtrl()->Freeze();
	ChartSetViewPtr chartSetCtrl(new ChartSetView());
	LocationSetLayerPtr locationSet(new LocationSetLayer(UniqueId::Inst().GenerateId(), 
			App::Inst().GetLayerTreeController()->GetSelectedLayer(),
			chartSetCtrl));
	
	int numLayers = App::Inst().GetLayerTreeController()->GetNumLocationSetLayers();
	// create name for new layer
	std::string result = "Combined_" + boost::lexical_cast<std::string>(numLayers+1);
	wxString mystring(result.c_str(),wxConvUTF8);
	locationSet->SetName(mystring);
	locationSet->SetFullPath(mystring);
	std::vector<LocationLayerPtr> locationLayers;
	// add locations to new location set layer
	foreach(LocationSetLayerPtr locpoint, LocationSets)
	{
		for(uint i = 0; i < locpoint->GetNumLocationLayers(); i++)
		{
			LocationLayerPtr locationLayerPointer = locpoint->GetLocationLayer(i);
			LocationLayerPtr locationLayer(new LocationLayer(locationLayerPointer));
			locationLayer->SetName(locationLayerPointer->GetName());

			locationLayer->Render();
			locationSet->AddLocationLayer(locationLayer);
			locationLayers.push_back(locationLayer);
		}
	}
	// link everything to the display
	locationSet->GetLocationSetController()->SetLocationSetLayers(locationLayers);
	App::Inst().GetLayerTreeController()->AddLocationSetLayerAtPosition(locationSet,0);
	App::Inst().GetLayerTreeController()->AddSequence(locationSet);
	App::Inst().GetLayerTreeController()->GetTreeCtrl()->Thaw();
	App::Inst().GetLayerTreeController()->GetLocationSetLayer(0)->GetLocationSetController()->GetNumericMetadataFields();
}
*/
void LocationMergeDlg::OnOK(wxCommandEvent& event)
{
	int numItems = m_locationSetCheckbox->GetCount();
	if( numItems > 0)
	{
		int * checkedIndexes = new int [numItems];
		LayerTreeControllerPtr layerTree = App::Inst().GetLayerTreeController();
		std::vector<LocationSetLayerPtr> LocationSets;
		std::vector<LocationModelPtr> LocationModels;
		std::vector<SequenceModelPtr> SequenceModels;
		std::vector<ChartSetViewPtr> ChartViews;
		//Finds the indexes of all checked boxes
		for(int locSet = 0; locSet < numItems; locSet++)
		{
			if(m_locationSetCheckbox->IsChecked(locSet))
			{
				LocationSetLayerPtr locationSet = layerTree->GetLocationSetLayer( locSet );
				ChartSetViewPtr chartView = layerTree->GetLocationSetLayer( locSet ) -> GetChartSetView();
				ChartViews.push_back(chartView);
				LocationSets.push_back(locationSet);
				for(uint i = 0; i < locationSet->GetNumLocationLayers(); i++)
				{
					LocationLayerPtr locLayer = locationSet -> GetLocationLayer(i);// -> GetSequenceLayer() -> GetSequenceController() -> GetSequenceModel();
					LocationModelPtr locationModel = locationSet -> GetLocationLayer(i) -> GetLocationController() -> GetLocationModel();
					LocationModels.push_back(locationModel);
					for(uint j = 0; j < locLayer->GetNumSequenceLayers(); j++)
					{
						SequenceModelPtr seqMod = locLayer -> GetSequenceLayer(j) -> GetSequenceController() -> GetSequenceModel();
						SequenceModels.push_back(seqMod);
					}
				}
			}
		}
	//	CreateLocationSet(LocationSets);
		CreateLocationSet(LocationModels,ChartViews);
		CreateSequenceSet(SequenceModels);
		Destroy();
	}
}
void LocationMergeDlg::CreateLocationSet( std::vector<LocationModelPtr> locationModels, std::vector<ChartSetViewPtr> ChartViews )
{
	if ( App::Inst().GetLayerTreeController()->GetNumLocationSetLayers() > 0 )
	{
		//wxMessageBox( wxT( "GenGIS currently supports only a single location set." ),
		//	wxT( "Multiple location sets not supported" ), wxOK | wxICON_INFORMATION );
		//return;
	}

	// get selected layer
	LayerPtr selectedLayer = App::Inst().GetLayerTreeController()->GetSelectedLayer();
	if(selectedLayer == LayerPtr() || selectedLayer->GetType() != Layer::MAP)
	{
		if(App::Inst().GetLayerTreeController()->GetNumMapLayers() == 1)
		{
			// select the first map by default
			App::Inst().GetLayerTreeController()->SetSelection( App::Inst().GetLayerTreeController()->GetMapLayer(0) );
		}
	}

	selectedLayer = App::Inst().GetLayerTreeController()->GetSelectedLayer();
	if(selectedLayer != LayerPtr() && selectedLayer->GetType() == Layer::MAP)
	{
		std::vector<LocationModelPtr> copyLocationModels;
		StudyControllerPtr studyController = App::Inst().GetLayerTreeController()->GetStudyLayer(0)->GetStudyController();

//		if( LocationSetIO::Read( fullPath.GetFullPath(), studyController, locationModels ) )
//		{	
			// stop refreshing the tree until all sequences are loaded (this is purely for efficency)
			App::Inst().GetLayerTreeController()->GetTreeCtrl()->Freeze();

			ChartSetViewPtr chartSetCtrl(new ChartSetView());
			for(uint chartset =0; chartset < ChartViews.size(); chartset++)
			{
				for(uint views = 0; views < ChartViews[chartset]->GetSize(); views++)
				{
					ChartViewPtr cView = ChartViews[chartset]->GetChart(views);
					chartSetCtrl->AddChart(cView);
				}
			}
			LocationSetLayerPtr locationSet(new LocationSetLayer(UniqueId::Inst().GenerateId(), 
				App::Inst().GetLayerTreeController()->GetSelectedLayer(),
				chartSetCtrl));
			int numLayers = App::Inst().GetLayerTreeController()->GetNumLocationSetLayers();
			std::string result = "Combined_" + boost::lexical_cast<std::string>(numLayers+1);
			wxString mystring(result.c_str(),wxConvUTF8);
			locationSet->SetName(mystring);
			locationSet->SetFullPath(mystring);
		//	m_locationSetLayer = locationSet;

		//	LocationSetIO::ReadSourceFile( fullPath.GetFullPath(), locationSet );

			// assign default colour map to location
			ColourMapManagerPtr colourMapManager = App::Inst().GetColourMapManager();
			ColourMapPtr defaultColourMap = colourMapManager->GetDefaultDiscreteColourMap();
			ColourMapDiscretePtr newColourMap(new ColourMapDiscrete(defaultColourMap));

			std::vector<LocationLayerPtr> locationLayers;
			foreach(LocationModelPtr locationModel, locationModels)
			{
				LocationModelPtr copyLocationModel(new LocationModel(locationModel->GetId(),locationModel->GetNorthing(),locationModel->GetEasting(),locationModel->GetData()));
				LocationViewPtr locationView(new LocationView(copyLocationModel, App::Inst().GetViewport()->GetCamera(), UniqueId::Inst().GenerateId()));
				ChartViewPtr pieChartView(new ChartView(copyLocationModel, locationView, newColourMap));
				chartSetCtrl->AddChart(pieChartView);
				LocationControllerPtr locationController(new LocationController(copyLocationModel, locationView, pieChartView));

				LocationLayerPtr locationLayer(new LocationLayer(UniqueId::Inst().GenerateId(), locationSet, locationController));			
				locationLayer->SetName(wxString(locationController->GetId().c_str()));
				locationLayer->SetFullPath( mystring );
				locationLayers.push_back(locationLayer);

				locationView->SetLocationLayerId(locationLayer->GetId());
				pieChartView->SetLocationLayerId(locationLayer->GetId());

				locationSet->AddLocationLayer(locationLayer);
			}

			locationSet->GetLocationSetController()->SetLocationSetLayers(locationLayers);

			App::Inst().GetLayerTreeController()->AddLocationSetLayerAtPosition(locationSet,0);

			App::Inst().GetLayerTreeController()->GetTreeCtrl()->Thaw();

			// Update Samples legend on sidebar
//			FillSamplesLegend();

			App::Inst().GetLayerTreeController()->GetLocationSetLayer(0)->GetLocationSetController()->GetNumericMetadataFields();
//		}
//		else
//		{
//			wxMessageBox(wxT("Failed to read location sites from file. Check console window for warning messages."), 
//				wxT("Failed to read file"), wxOK | wxICON_INFORMATION);
//		}
	}
	else
	{
		wxMessageBox(wxT("Please select a map node before adding a location sites layer."),
			wxT("Select map node"), wxOK | wxICON_INFORMATION);
	}
}
void LocationMergeDlg::CreateSequenceSet( std::vector<SequenceModelPtr> sequenceModels )
{
	LayerPtr selectedLayer = App::Inst().GetLayerTreeController()->GetSelectedLayer();
	if(selectedLayer == LayerPtr() || selectedLayer->GetType() != Layer::LOCATION_SET)
	{
		if(App::Inst().GetLayerTreeController()->GetNumLocationSetLayers() == 1)
		{
			// select the first location set layer by default
			App::Inst().GetLayerTreeController()->SetSelection( App::Inst().GetLayerTreeController()->GetLocationSetLayer(0) );
		}
	}

	selectedLayer = App::Inst().GetLayerTreeController()->GetSelectedLayer();
	if(selectedLayer != LayerPtr() && selectedLayer->GetType() == Layer::LOCATION_SET)
	{
//		std::vector<SequenceModelPtr> sequenceModels;
//		bool bCancel;
//		if( SequenceIO::Read( fullPath.GetFullPath(), sequenceModels, bCancel ) )
//		{	
			wxBusyCursor wait;

			// stop refreshing the tree until all sequences are loaded (this is purely for efficency)
			App::Inst().GetLayerTreeController()->GetTreeCtrl()->Freeze();

			std::set<std::wstring> missingLocations;
			foreach(SequenceModelPtr sequenceModel, sequenceModels)
			{
				SequenceModelPtr copySequenceModel(new SequenceModel(sequenceModel->GetSequenceId(),sequenceModel->GetSiteId(),sequenceModel->GetData()));
				
				SequenceControllerPtr sequenceController(new SequenceController(copySequenceModel));

				SequenceLayerPtr sequenceLayer(new SequenceLayer(UniqueId::Inst().GenerateId(), selectedLayer, sequenceController));
				sequenceLayer->SetName(sequenceController->GetSequenceId().c_str());
				sequenceLayer->SetFullPath( wxT("blahblahblah") );

				if(!App::Inst().GetLayerTreeController()->AddSequence(sequenceLayer, copySequenceModel, missingLocations))
				{			
					// The above function should never return false!
					wxMessageBox(wxT("Critial error: Failed to load sequence data. Please report this error to the GenGIS team."), wxT("Failed to load sequence data."), wxOK | wxICON_ERROR);
					return;
				}
			}

			// report any locations that are specified in the sequence file, but do not exist
			foreach(std::wstring locationStr, missingLocations)
			{
				Log::Inst().Warning(wxT("(Warning) Site id '") + wxString(locationStr.c_str()) + wxT("' was not found in the location set. Sequences from this location will be ignored."));
			}

			// all tree to refresh
			App::Inst().GetLayerTreeController()->GetTreeCtrl()->Thaw();
	//	}
//		else
//		{
//			if(!bCancel)
//				wxMessageBox(wxT("Failed to read sequence data from file. Check console window for warning messages."), wxT("Failed to read file"), wxOK | wxICON_INFORMATION);
//		}
	}
	else
	{
		wxMessageBox(wxT("Please select a location set node before adding sequence data."), wxT("Select location set node"), wxOK | wxICON_INFORMATION);
	}

	App::Inst().GetViewport()->Refresh(false);
}