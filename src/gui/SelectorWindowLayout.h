///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Sep  8 2010)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __SelectorWindowLayout__
#define __SelectorWindowLayout__

#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/listbox.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

namespace GenGIS
{
	
	///////////////////////////////////////////////////////////////////////////////
	/// Class SelectorWindowLayout
	///////////////////////////////////////////////////////////////////////////////
	class SelectorWindowLayout : public wxDialog 
	{
		private:
		
		protected:
			wxStaticText* m_staticText;
			wxListBox* m_listBoxSelect;
			wxButton* m_buttonSelect;
			wxButton* m_buttonCancel;
			
			// Virtual event handlers, overide them in your derived class
			virtual void OnSelectionDoubleClick( wxCommandEvent& event ) { event.Skip(); }
			virtual void OnSelectButton( wxCommandEvent& event ) { event.Skip(); }
			virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }
			
		
		public:
			
			SelectorWindowLayout( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 308,151 ), long style = wxDEFAULT_DIALOG_STYLE );
			~SelectorWindowLayout();
		
	};
	
} // namespace GenGIS

#endif //__SelectorWindowLayout__
