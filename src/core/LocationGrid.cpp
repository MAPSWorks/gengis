//=======================================================================
// Author: Timothy Mankowski & Alexander Keddy
//
// Copyright 2013 Timothy Mankowski
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

#include "../core/App.hpp"
#include "../core/MapController.hpp"
#include "../core/LocationGrid.hpp"
#include "../core/TileModel.hpp"
#include "../core/LocationSetLayer.hpp"
#include "../core/SequenceModel.hpp"
#include "../utils/ColourMap.hpp"
#include "../utils/ColourMapManager.hpp"
#include "../utils/Colour.hpp"
#include "../core/LocationSetController.hpp"
#include "../utils/StringTools.hpp"
#include "../utils/ColourMapDiscrete.hpp"
#include "../core/StudyController.hpp"

#include "../utils/ErrorGL.hpp"

using namespace GenGIS;

LocationGrid::LocationGrid() :
	// General
	m_divideTilesAlong( LATITUDE ),
	m_divideTilesBy( AXIS ),
	m_divideTilesInto( DEGREE ),
	m_gridAlignmentStyle( ORIGIN ),
	m_mapOpenGLBoundaries( -1.0, -1.0, 1.0, 1.0 ),
	m_mapOffset( 0.0f, 0.0f ),
	m_divisions( 4 ),
	m_axisDivisions( 4 ),
	m_boxDivisions( 1 ),
	m_autoAdjustElevation( true ),
	m_elevation( 0.05f ),
	m_elevationUsed( 0.0f ),
	m_gridChanged( true ),

	// Tiles
	m_showTiles( true ),
	m_tileFillMode( UNIFORM ),
	m_uniformColourOfTiles( 0.0f, 0.5f, 0.0f, 0.3f ),
	m_defaultColourOfTiles( 0.5f, 0.5f, 0.5f, 0.3f ),
	m_combination ( TileModel::AVERAGE ),

	// Border
	m_showBorders( true ),
	m_colourOfBorders( 0.0f, 0.0f, 0.0f, 0.3f ),
	m_thicknessOfBorders( 1 ),
	m_styleOfBorders( VisualLine::SOLID ),
	
//	m_tilePokeTree(),
//	m_tileBST(),
	m_tileBounds()
{
	// Property of 'Layer' class
	m_bVisible = false;

	m_xCoordinates.clear();
	m_yCoordinates.clear();
	ColourMapManagerPtr colourMapManager = App::Inst().GetColourMapManager();
	ColourMapPtr defaultColourMap = colourMapManager->GetDefaultDiscreteColourMap();
	m_gridColourMap.reset(new ColourMapDiscrete(defaultColourMap));

}

template<class Archive> 
void LocationGrid::serialize(Archive & ar, const unsigned int version)
{
	// Serialize base class (View)
	ar & boost::serialization::base_object<View>(*this);

	// General variables
	ar & m_divideTilesAlong;     // DIVISION_AXIS
	ar & m_divideTilesBy;		 // DIVISION_TYPE
	ar & m_divideTilesInto;		 // DIVISION_BOX
	ar & m_gridAlignmentStyle;   // ALIGNMENT
	ar & m_mapOpenGLBoundaries;  // Box2D
	ar & m_mapOffset;            // Point2D
	ar & m_divisions;            // uint
	ar & m_axisDivisions;		 // uint
	ar & m_boxDivisions;		 // uint
	ar & m_autoAdjustElevation;  // bool
	ar & m_elevation;            // float
	ar & m_elevationUsed;        // float
	ar & m_xCoordinates;         // std::list<double>
	ar & m_yCoordinates;         // std::list<double>
	ar & m_locationSetLayer;     // LocationSetLayerPtr
	ar & m_gridChanged;			 // bool
	ar & m_field;				 // std::wstring
	ar & m_selectedFieldValues;	 // std::vector<double>
	ar & m_combination;			 // TileModel::DATA_COMBINE

	// Tile variables
	ar & m_showTiles;            // bool
	ar & m_tileFillMode;         // TILE_FILL
	ar & m_uniformColourOfTiles; // Colour
	ar & m_defaultColourOfTiles; // Colour
	ar & m_tileModels;           // std::vector<TileModelPtr>
//	ar & m_tileBST;		 // BSTPtr
	ar & m_gridColourMap;		 // ColourMapDiscretePtr

	// Border variables
	ar & m_showBorders;          // bool
	ar & m_colourOfBorders;      // Colour
	ar & m_thicknessOfBorders;   // uint
	ar & m_styleOfBorders;       // VisualLine::LINE_STYLE
}
template void LocationGrid::serialize<boost::archive::text_woarchive>(boost::archive::text_woarchive& ar, const unsigned int version); 
template void LocationGrid::serialize<boost::archive::text_wiarchive>(boost::archive::text_wiarchive& ar, const unsigned int version);


LocationGrid::~LocationGrid()
{
}

bool LocationGrid::IsVisible() 
{ 
	return m_bVisible; 
}

void LocationGrid::SetSelectedFieldValues(std::vector<wxStaticText*> field)
{
	m_selectedFieldValues.clear();
	for( uint i = 0; i < field.size(); i++)
	{
		std::wstring fieldValue = field[i]->GetLabel().c_str();
		m_selectedFieldValues.push_back(  StringTools::ToDouble( fieldValue ) );
	}
}

void LocationGrid::GenerateTileCoordinates()
{
	if( !m_gridChanged )
		return;

	// Real terrain values of DEM map
	MapControllerPtr mapController = App::Inst().GetLayerTreeController()->GetMapLayer(0)->GetMapController();
	m_mapOpenGLBoundaries = Box2D( -( mapController->GetWidth() / 2 ), -( mapController->GetHeight() / 2 ),
		( mapController->GetWidth() / 2 ), ( mapController->GetHeight() / 2 ) );

	// Reset Origin if needed
	if ( GetGridAlignmentStyle() == ORIGIN )
		SetMapOffset( Point2D(0,0) );

	// Set the grid elevation
	if ( m_autoAdjustElevation )
	{
		m_elevationUsed = mapController->GetMaxElevationGridSpace()
		                * mapController->GetVerticalExaggeration()
		                + 0.005; // Add a minimal amount to avoid clashing
		                         // between colours of grid and map
	}
	else
	{
		m_elevationUsed = m_elevation;
	}

	// Determine appropriate tile size according to whether the user
	// wishes to divide the latitude or longitude 'n' number of times
	double tileSize;
	double tileSizeX;
	double tileSizeY;
	if (m_divideTilesBy == AXIS )
	{
		if ( m_divideTilesAlong == LATITUDE )
			tileSize = m_mapOpenGLBoundaries.Height() / m_divisions;
		else if ( m_divideTilesAlong == LONGITUDE )
			tileSize = m_mapOpenGLBoundaries.Width() / m_divisions;
		tileSizeX = tileSize;
		tileSizeY = tileSize;
	}
	else if(m_divideTilesBy == BOX )
	{
		// divide into degrees or pixels;
		if( m_divideTilesInto == DEGREE )
		{
			// convert map origin point to lat lon, add one (degree) 
			Point3D tileOrigin( m_mapOpenGLBoundaries.x, 0, m_mapOpenGLBoundaries.y); 
			GeoCoord tileCoord;
			App::Inst().GetMapController()->GetMapModel()->GridToGeo( tileOrigin, tileCoord );
			// go to next tile boundary
			Point3D nextTile;
			GeoCoord tileNext( tileCoord.easting + m_divisions, tileCoord.northing - m_divisions);
			App::Inst().GetMapController()->GetMapModel()->LatLongToGrid( tileNext, nextTile);
			
			tileSizeX = abs(nextTile.x - tileOrigin.x);
			tileSizeY = abs(tileOrigin.z- nextTile.z);			
		}
		else if(m_divideTilesInto == PIXEL)
		{
			float pixels = float(m_divisions) / 100.0f;
			Point3D tileOrigin( m_mapOpenGLBoundaries.x, 0, m_mapOpenGLBoundaries.y); 
			Point3D nextTile( m_mapOpenGLBoundaries.x , 0, m_mapOpenGLBoundaries.y );
			nextTile.x = nextTile.x + pixels;
			nextTile.z = nextTile.z + pixels;
			
			tileSizeX = abs(nextTile.x - tileOrigin.x);
			tileSizeY = abs(tileOrigin.z- nextTile.z);
		}
	}

	// Clear any previous tile coordinates
	m_xCoordinates.clear();
	m_yCoordinates.clear();

	double currentX = m_mapOpenGLBoundaries.x;
	double currentY = m_mapOpenGLBoundaries.y;

	// Set initial coordinates 'x' and 'y' to the map origin
	m_xCoordinates.push_back( currentX );
	m_yCoordinates.push_back( currentY );

	// [ OFFSET ] ( MIDDLE * N ) [ REMAINDER ] 

	// Calculate map OFFSET for OpenGL; fmod is modulus for floats
	double mapOffsetX = fmod( ( double )m_mapOffset.x, tileSizeX );
	if ( mapOffsetX > 0 )
	{
		currentX += mapOffsetX;
		m_xCoordinates.push_back( currentX );
	}

	double mapOffsetY = fmod( ( double )m_mapOffset.y, tileSizeY );
	if ( mapOffsetY > 0 )
	{
		currentY += mapOffsetY;
		m_yCoordinates.push_back( currentY );
	}

	uint numberOfMiddleTiles;
	
	// Calculate the number of MIDDLE divisions along each axis after the offset
	numberOfMiddleTiles = floor( abs( ( m_mapOpenGLBoundaries.Width() - mapOffsetX ) / tileSizeX ) );
	for ( uint col = 0; col < numberOfMiddleTiles; col++ )
	{
		currentX += tileSizeX;
		
		m_xCoordinates.push_back( currentX );
	}

	numberOfMiddleTiles = floor( abs( ( m_mapOpenGLBoundaries.Height() - mapOffsetY ) / tileSizeY ) );
	for ( uint row = 0; row < numberOfMiddleTiles; row++ )
	{
		currentY += tileSizeY;

		m_yCoordinates.push_back( currentY );
	}

	// Add the remainder if currentX is smaller than map width
	if ( m_divideTilesAlong == LATITUDE || m_divideTilesBy == BOX )
	{
		if ( currentX < m_mapOpenGLBoundaries.dx )
		{
			currentX += m_mapOpenGLBoundaries.dx - currentX;
			m_xCoordinates.push_back( currentX );
		}
	}
	if ( m_divideTilesAlong == LONGITUDE || m_divideTilesBy == BOX )
	{
		// Add the remainder if currentY is smaller than map height
		if ( currentY < m_mapOpenGLBoundaries.dy )
		{
			currentY += m_mapOpenGLBoundaries.dy - currentY;
			m_yCoordinates.push_back( currentY );
		}
	}

	if ( m_gridChanged )
	{
	//	InitTiles();
		InitBoundArray();
	}
}

void LocationGrid::Render()
{
	Colour tileColour;

	if( !IsVisible() )
		return;

	ColourMapDiscretePtr m_colourMap = m_locationSetLayer->GetLocationSetController()->GetColourMap();

	// Colour palette for tiles (temporary)
	float alphaOfTile = m_uniformColourOfTiles.GetAlpha();
	error::ErrorGL::Check();
	glDisable( GL_LIGHTING );

	// Render borders (lines)
	if ( m_showBorders )
	{
		// Set OpenGL options
		glColor4f( m_colourOfBorders.GetRed(), m_colourOfBorders.GetGreen(),
			m_colourOfBorders.GetBlue(), m_colourOfBorders.GetAlpha() );
		glEnable (GL_LINE_SMOOTH);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
		glLineWidth( m_thicknessOfBorders * App::Inst().GetResolutionFactor() );
		glEnable( GL_LINE_STIPPLE );
		glLineStipple( 2, m_styleOfBorders );

		glBegin( GL_LINES );
		std::list<double>::iterator col;
		std::list<double>::iterator row;
		for ( col = m_xCoordinates.begin(); col != m_xCoordinates.end(); ++col )
		{
			// Draw column borders
			glVertex3f( *col,  m_elevationUsed, m_mapOpenGLBoundaries.y  );
			glVertex3f( *col,  m_elevationUsed, m_mapOpenGLBoundaries.dy );
		}
		for ( row = m_yCoordinates.begin(); row != m_yCoordinates.end(); ++row )
		{
			// Draw row borders
			glVertex3f( m_mapOpenGLBoundaries.x,  m_elevationUsed, *row );
			glVertex3f( m_mapOpenGLBoundaries.dx, m_elevationUsed, *row );
		}
		glEnd();
	}

	// Render tiles
	if ( m_showTiles )
	{
		if ( m_tileFillMode == UNIFORM )
		{
			glBegin( GL_QUADS );

			glColor4f( m_uniformColourOfTiles.GetRed(), m_uniformColourOfTiles.GetGreen(),
				m_uniformColourOfTiles.GetBlue(), m_uniformColourOfTiles.GetAlpha() );

			glVertex3f( m_mapOpenGLBoundaries.x,  m_elevationUsed, m_mapOpenGLBoundaries.y  );
			glVertex3f( m_mapOpenGLBoundaries.x,  m_elevationUsed, m_mapOpenGLBoundaries.dy );
			glVertex3f( m_mapOpenGLBoundaries.dx, m_elevationUsed, m_mapOpenGLBoundaries.dy );
			glVertex3f( m_mapOpenGLBoundaries.dx, m_elevationUsed, m_mapOpenGLBoundaries.y  );

			glEnd();
		}
		else if ( m_tileFillMode == MAPPED )
		{
			// Render tiles from a stored list of previously generated coordinates
			glBegin( GL_QUADS );
			
			int i = 0;
			MapModelPtr mapModel = App::Inst().GetMapController()->GetMapModel();
			std::list<double>::iterator row1, row2, col1, col2;
			row2 = m_yCoordinates.begin(); row2++;
			//row heads of each tile
			for ( row1 = m_yCoordinates.begin(); row2 != m_yCoordinates.end(); ++row1 )
			{
				col2 = m_xCoordinates.begin(); col2++;
					
				//column heads of each tile
				for ( col1 = m_xCoordinates.begin(); col2 != m_xCoordinates.end(); ++col1 )
				{
					std::set<int>::iterator it;
					it = m_occupiedTiles.find(i);
					if( it == m_occupiedTiles.end() )
						tileColour = m_defaultColourOfTiles;
					else
						tileColour = m_tileShortModels[i]->GetColour();
					
				//	tileColour = m_tileModels[i]->GetColour();	
					glColor4f( tileColour.GetRed(), tileColour.GetGreen(), tileColour.GetBlue(), tileColour.GetAlpha() );
					glVertex3f( *col1, m_elevationUsed, *row1 );
					glVertex3f( *col2, m_elevationUsed, *row1 );
					glVertex3f( *col2, m_elevationUsed, *row2 );
					glVertex3f( *col1, m_elevationUsed, *row2 );
					col2++;
					i++;
				}
				row2++;
			}	
			glEnd();
		}
	}
}

std::vector<std::wstring> LocationGrid::GetSelectedValues( std::wstring field )
{
	std::vector<std::wstring> values;
	std::map<int,TileModelPtr>::iterator it;
	for( it = m_tileShortModels.begin(); it != m_tileShortModels.end(); ++it )
	{

		std::map<std::wstring,std::wstring> data = (it->second)->GetData();
		if(! data[field].empty() )
			values.push_back(data[field]);
	}

	return values;
}


void LocationGrid::SetLocationColours()
{
	std::map<int,TileModelPtr>::iterator it;
	for( it = m_tileShortModels.begin(); it != m_tileShortModels.end(); ++it )
	{
		Colour colour;
		// find colour in colour map corrsponding to field value of location
		std::map<std::wstring,std::wstring> data = (it->second)->GetData();
		std::map<std::wstring,std::wstring>::const_iterator dataIt = data.find(m_field);

		if( dataIt == data.end() )
		{
			data = it->second->GetSequence(0)->GetData();
			dataIt = data.find(m_field);
		}
		if(!m_gridColourMap->GetColour(dataIt->second, colour))
		{
			Log::Inst().Error("(Error) LocationGrid::SetLocationColours(): no colour associated with name.");
		}

		// set alpha of colour
		if (colour == m_defaultColourOfTiles)
		{
			colour.SetAlpha( GetDefaultTileAlpha() );
		}
		else 
		{
			colour.SetAlpha( GetTileAlpha() );
		}
		
		it->second->SetColour(colour);
	}
}

void LocationGrid::UpdateGridColours() {

	//Calculates and prints max/min, used to test
	//InitTileMinMax();
	//Log::Inst().Error("Max: " + StringTools::ToString(m_maxFieldValue));
	//Log::Inst().Error("Min: " + StringTools::ToString(m_minFieldValue));

//	InitTiles();
//	FillTiles();
	InitBoundArray();
	MakeGrid();

	std::vector<LocationLayerPtr> locationLayers = m_locationSetLayer->GetAllActiveLocationLayers();
	if ( !locationLayers.empty()
		&& StringTools::IsDecimalNumber(locationLayers[0]->GetLocationController()->GetData()[m_field])
	    || m_combination == TileModel::GINI )  {
		
		UpdateNumericColourMap( m_minFieldValue, m_maxFieldValue );
		
	} else {
		UpdateQualitativeColourMap();
	
	}

	SetLocationColours();

}

void LocationGrid::InitTileMinMax() {

	if (!m_bVisible || m_tileFillMode == UNIFORM)
		return;

	std::vector<LocationLayerPtr> locationLayers = m_locationSetLayer->GetAllActiveLocationLayers();

	if ( !locationLayers.empty()
		&& StringTools::IsDecimalNumber(locationLayers[0]->GetLocationController()->GetData()[m_field]) 
		&& m_combination != TileModel::GINI ) {

	//	InitTiles();

		for( uint i = 0; i < locationLayers.size() ; i++)
		{
			std::map<std::wstring,std::wstring> data = locationLayers[i]->GetLocationController()->GetData();
			float easting = StringTools::ToDouble(data[StringTools::ToStringW("Longitude")]);
			float northing = StringTools::ToDouble(data[StringTools::ToStringW("Latitude")]);
			
			// check if long and lat values exist. if they don't use easting and westing
			if( easting != easting || 
				northing != northing ||
				!(easting<=DBL_MAX && easting >= -DBL_MAX) ||
				!(northing<=DBL_MAX && northing >= -DBL_MAX)||
				(!App::Inst().GetStudyController()->IsUsingProjection() &&
				!App::Inst().GetStudyController()->IsUsingGeographic() ) )
			{
				easting = locationLayers[i]->GetLocationController()->GetEasting();
				northing = locationLayers[i]->GetLocationController()->GetNorthing();
			}

			Point3D locationCoord;
			GeoCoord locationGeo( easting,northing );
			App::Inst().GetMapController()->GetMapModel()->LatLongToGrid(locationGeo,locationCoord);
			Point2D locPoint(locationCoord.x,locationCoord.z);
			TileModelPtr tile = Search(locPoint);
			tile->AddLocationLayer(locationLayers[i]);
		}

		m_minFieldValue = FLT_MAX;
		m_maxFieldValue = -FLT_MAX;

		if (m_combination == TileModel::SUM || m_combination == TileModel::AVERAGE || m_combination == TileModel::STDEV) {

			for (std::map<int, TileModelPtr>::iterator i = m_tileShortModels.begin(); i != m_tileShortModels.end(); ++i) {

				std::map<std::wstring, std::wstring> data = i->second->GetData();
				std::map<std::wstring, std::wstring>::const_iterator it = data.find(m_field);

				if (it != data.end()) {

					std::wstring field = it->first;
					std::wstring value = it->second;

					std::vector<std::wstring> seperatedValue;
					float sumOfPositiveValues = 0;
					float sumOfNegativeValues = 0;
					float minValue;
					float maxValue;
					boost::split(seperatedValue,value,boost::is_any_of("|"),boost::token_compress_on);

					// combination Methods that only handle numbers
					//		FIND A WAY TO EXCLUDE SEQUENCE ID!!!!!!!!!!
					if( ( StringTools::IsDecimalNumber( seperatedValue[0] ) )
						&& ( StringTools::ToLower(field).compare(_T("cell id")) != 0 )
						&& ( StringTools::ToLower(field).compare(_T("cellid")) != 0 ) )
					{
						// convert string values to double values. Non numeric already parsed out
						for( uint j = 0; j < seperatedValue.size(); j++)
						{	
							double doubValue = StringTools::ToDouble(seperatedValue[j]);

							if (j == 0) {
								minValue = doubValue;
								maxValue = doubValue;
							} else if (doubValue < minValue) {
								minValue = doubValue;
							} else if (doubValue > maxValue) {
								maxValue = doubValue;
							}

							if (m_combination == TileModel::SUM) {
								if (doubValue < 0) {
									sumOfNegativeValues += doubValue;
								} else {
									sumOfPositiveValues += doubValue;
								}
							}

						}
					}
					
					float tileMin = 0;
					float tileMax = 0;

					if (m_combination == TileModel::SUM) {

						//If there aren't any positive values then the maximum sum is the maximum negative value
						//otherwise, it's the accumulated positive values
						if (sumOfPositiveValues == 0) {
							tileMax = maxValue;
						} else {
							tileMax = sumOfPositiveValues;
						}

						//If there aren't any negative values then the minimum sum is the minimum positive value 
						//(default colour used when no points in tile) otherwise, it's the accumulated negative values
						if (sumOfNegativeValues == 0) {
							tileMin = minValue;
						} else {
							tileMin = sumOfNegativeValues;
						}

					} else if (m_combination == TileModel::AVERAGE) {

						//The average is between the min and max values
						tileMin = minValue;
						tileMax = maxValue;

					} else if (m_combination == TileModel::STDEV) {

						//Find max standard deviation by calculating stdev between min / max
						float mean = ( minValue + maxValue ) / 2;
						float sq_sum = minValue * minValue + maxValue * maxValue;
						tileMax = sqrt( sq_sum / 2 - mean * mean );

						//Tiles with one value are given a stdev of 0, so the min stdev is always 0
						tileMin = 0;

					}

					if (tileMin < m_minFieldValue) {
						m_minFieldValue = tileMin;
					}

					if (tileMax > m_maxFieldValue) {
						m_maxFieldValue = tileMax;
					}
				}
			}
		}
	} else if (m_combination == TileModel::GINI) {

		m_minFieldValue = 0;
		m_maxFieldValue = 1;

	}
}

void LocationGrid::UpdateNumericColourMap(float min, float max) {

	ColourMapPtr selectedColourMap = m_gridColourMap;

	std::vector<std::wstring> data = GetSelectedValues(m_field);
	StringTools::SortFieldValues(data);

	std::vector<std::wstring>::const_iterator setIt;
	for (setIt = data.begin(); setIt != data.end(); ++setIt) {

		double numberValue = StringTools::ToDouble( *setIt );

		Colour colour;
		if (numberValue < min && numberValue >= floor(min))
			min = numberValue;
		if (numberValue > max && numberValue <= ceil(max))
			max = numberValue;

		colour = selectedColourMap->GetInterpolatedColour( numberValue, min, max );
		m_gridColourMap->SetColour(*setIt, colour);

	}
}

void LocationGrid::UpdateQualitativeColourMap() {

	// duplicate current colourMap
//	ColourMapPtr selectedColourMap = m_gridColourMap;
	
	// Get tile values for current field
	std::vector<std::wstring> data = GetSelectedValues(m_field);
	// Sort data
	StringTools::SortFieldValues(data);

	std::vector<std::wstring>::const_iterator setIt;
	// Used so that an interpolated colour can be shown for incomplete tiles displayed by showspread
	int count = 0;

	for (setIt = data.begin(); setIt != data.end(); ++setIt) {
	
		Colour colour;

		if (!m_gridColourMap->GetColour(*setIt, colour))
			colour = m_gridColourMap->GetInterpolatedColour( ++count, 0, data.size() );

		m_gridColourMap->SetColour(*setIt, colour);

	}

}

void LocationGrid::MakeGrid()
{
	m_tileShortModels.clear();
	m_occupiedTiles.clear();
	int index = 0;

	std::vector<LocationLayerPtr> locationLayers = m_locationSetLayer->GetAllActiveLocationLayers();
	for( uint i = 0; i < locationLayers.size() ; i++)
	{
		std::map<std::wstring,std::wstring> data = locationLayers[i]->GetLocationController()->GetData();
		float easting = StringTools::ToDouble(data[StringTools::ToStringW("Longitude")]);
		float northing = StringTools::ToDouble(data[StringTools::ToStringW("Latitude")]);
		
		
		// check if long and lat values exist. if they don't use easting and westing
		if( easting != easting || 
			northing != northing ||
			!(easting<=DBL_MAX && easting >= -DBL_MAX) ||
			!(northing<=DBL_MAX && northing >= -DBL_MAX)||
			(!App::Inst().GetStudyController()->IsUsingProjection() &&
			!App::Inst().GetStudyController()->IsUsingGeographic() ) )
		{
			easting = locationLayers[i]->GetLocationController()->GetEasting();
			northing = locationLayers[i]->GetLocationController()->GetNorthing();
		}

		Point3D locationCoord;
		GeoCoord locationGeo( easting,northing );
		Point2D locPoint(easting, northing);

		locBound bound = m_tileBounds.Search( locPoint, &index );
		m_occupiedTiles.insert(index);
		std::wstring ID = StringTools::ToStringW(bound.GetTopLeft().x,2); 
		ID.append(StringTools::ToStringW("_"));
		ID.append(StringTools::ToStringW(bound.GetTopLeft().y,2));
		std::map<std::wstring,std::wstring> empty;
		TileModelPtr tile(new TileModel(ID,bound.GetTopLeft(),bound.GetBottomRight(),empty));
		tile->AddLocationLayer( locationLayers[i] );
		Insert( tile, locationLayers[i], index );
	}

	// now set tile true values for every tile
	std::map<int,TileModelPtr>::iterator it;
	for( it = m_tileShortModels.begin(); it != m_tileShortModels.end(); ++it )
	{
		if(it->second->GetNumLocations() > 0 )
		{
			it->second->CombineData();
			//combine sequence layers if there are any
			if(it->second->GetNumSequence() > 0)
				it->second->CombineSequenceData();
		}
	}
}

void LocationGrid::Insert( TileModelPtr tile, LocationLayerPtr loc, int tileNum )
{
	std::map<int, TileModelPtr>::iterator it;
	it = m_tileShortModels.find(tileNum);
	if ( m_tileShortModels.end() == it )
		m_tileShortModels[ tileNum ] = tile;
	else
		m_tileShortModels[tileNum]->AddLocationLayer( loc );
	
}

TileModelPtr LocationGrid::Search( Point2D point )
{
	int index;
	// don't care about the bounds
	m_tileBounds.Search( point, &index );
	return m_tileShortModels[index];
}

void LocationGrid::InitBoundArray()
{
	m_tileBounds.Clear();
	MapModelPtr mapModel = App::Inst().GetMapController()->GetMapModel();
	std::list<double>::iterator row1, row2, col1, col2;
	row2 = m_yCoordinates.begin(); row2++;
	//row heads of each tile
	for ( row1 = m_yCoordinates.begin(); row2 != m_yCoordinates.end(); ++row1 )
	{
		col2 = m_xCoordinates.begin(); col2++;
			
		//column heads of each tile
		for ( col1 = m_xCoordinates.begin(); col2 != m_xCoordinates.end(); ++col1 )
		{
			Point3D topLeftCoord(*col1,m_elevationUsed,*row1);
			Point3D bottomRightCoord(*col2,m_elevationUsed,*row2);
			GeoCoord topLeftGeo;
			GeoCoord bottomRightGeo;
			mapModel->GridToGeo(topLeftCoord,topLeftGeo);
			mapModel->GridToGeo(bottomRightCoord,bottomRightGeo);
			m_tileBounds.Add( Point2D(topLeftGeo.easting,topLeftGeo.northing), Point2D(bottomRightGeo.easting,bottomRightGeo.northing) );
				
			col2++;
		}
		row2++;
	}
}

void LocationGrid::SetOriginOffset( std::wstring selectedName )
{
	if( GetGridAlignmentStyle() == LOCATIONS )
	{
		float xAxisPosition = m_locationSetLayer->GetLocationLayer( selectedName )->GetLocationController()->GetEasting();
		float yAxisPosition = m_locationSetLayer->GetLocationLayer( selectedName )->GetLocationController()->GetNorthing();

		//convert axis positions to open gl coordinate
		Point3D locationCoord;
		GeoCoord locationGeo(xAxisPosition,yAxisPosition);
		App::Inst().GetMapController()->GetMapModel()->GeoToGrid(locationGeo,locationCoord);

		//find which tile this location belongs in
		TileModelPtr tile = Search( Point2D(locationCoord.x,locationCoord.z) );

		GeoCoord tileOrigin( tile->GetTopLeft().x, tile->GetTopLeft().y ); 
		Point3D tileCoord;
		App::Inst().GetMapController()->GetMapModel()->LatLongToGrid( tileOrigin, tileCoord );

		// negative offsets mean the wrong tile has been selected
		float xOffset =  locationCoord.x - tileCoord.x;
		float yOffset =  locationCoord.z - tileCoord.z;
	
		SetMapOffset( Point2D(xOffset,yOffset) );
	}
}

void LocationGrid::SetOriginOffset( Point2D coord )
{
	if( GetGridAlignmentStyle() == COORDINATES )
	{
		float xAxisPosition = coord.x;
		float yAxisPosition = coord.y;

		//convert axis positions to open gl coordinate
		Point3D locationCoord;
		GeoCoord locationGeo(xAxisPosition,yAxisPosition);
		App::Inst().GetMapController()->GetMapModel()->LatLongToGrid(locationGeo,locationCoord);

		
		double tileSize;
		if ( m_divideTilesAlong == LATITUDE )
			tileSize = m_mapOpenGLBoundaries.Height() / m_divisions;
		else if ( m_divideTilesAlong == LONGITUDE )
			tileSize = m_mapOpenGLBoundaries.Width() / m_divisions;

		// handle boundries
		if ((locationCoord.z > m_mapOpenGLBoundaries.dy) || (locationCoord.z < m_mapOpenGLBoundaries.y))	// lat boundary
		{		
			double tileSizeY = tileSize;
			if (locationCoord.z > m_mapOpenGLBoundaries.y)
				tileSizeY = tileSize * -1;
			while ( ( locationCoord.z > m_mapOpenGLBoundaries.dy) || (locationCoord.z < m_mapOpenGLBoundaries.y ) )
				locationCoord.z = locationCoord.z + tileSizeY;
		}
		if ( (locationCoord.x > m_mapOpenGLBoundaries.dx) || (locationCoord.x < m_mapOpenGLBoundaries.x) )	// lon boundary
		{		
			double tileSizeX = tileSize;
			if (locationCoord.x > m_mapOpenGLBoundaries.x)
				tileSizeX = tileSize * -1;
			while( (locationCoord.x > m_mapOpenGLBoundaries.dx) || (locationCoord.x < m_mapOpenGLBoundaries.x) )
				locationCoord.x = locationCoord.x + tileSizeX;
		}


		//find which tile this location belongs in
	//	TileModelPtr tile = FindLocationTile( Point2D(locationCoord.x,locationCoord.z) );
		TileModelPtr tile = Search( Point2D(locationCoord.x,locationCoord.z) );

		GeoCoord tileOrigin( tile->GetTopLeft().x, tile->GetTopLeft().y ); 
		Point3D tileCoord;
		App::Inst().GetMapController()->GetMapModel()->LatLongToGrid( tileOrigin, tileCoord );

		float xOffset =  locationCoord.x - tileCoord.x;
		float yOffset =  locationCoord.z - tileCoord.z;

		SetMapOffset( Point2D(xOffset,yOffset) );
	}
}

void LocationGrid::AssertCombinationMethod()
{
	std::map<int,TileModelPtr>::iterator it;
	for( it = m_tileShortModels.begin(); it != m_tileShortModels.end(); ++it )
	{
		it->second->SetCombinationMethod(m_combination);
	}
}


TileModel::DATA_COMBINE LocationGrid::GetCombinationMethod()
{
	return m_tileShortModels.begin()->second->GetCombinationMethod();
}


void LocationGrid::SetAxisDivisions( uint divisions )
{
	m_axisDivisions = divisions;
	SetDivisions( divisions );

}
void LocationGrid::SetBoxDivisions( uint divisions )
{
	m_boxDivisions = divisions;
	SetDivisions( divisions );

}