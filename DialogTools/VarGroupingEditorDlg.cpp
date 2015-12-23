/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <set>
#include <boost/foreach.hpp>
#include <boost/uuid/uuid.hpp>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "../FramesManager.h"
#include "../DbfFile.h"
#include "../DataViewer/DbfTable.h"
#include "../DataViewer/OGRTable.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../DataViewer/TimeState.h"
#include "../DialogTools/ExportDataDlg.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../ShapeOperations/GalWeight.h"
#include "../GeneralWxUtils.h"
#include "../GeoDa.h"
#include "../logger.h"
#include "../Project.h"
#include "../GdaConst.h"
#include "VarGroupingEditorDlg.h"
#include "TimeEditorDlg.h"

BEGIN_EVENT_TABLE( VarGroupingEditorDlg, wxDialog )
	EVT_CLOSE( VarGroupingEditorDlg::OnClose )

    EVT_BUTTON( XRCID("ID_SAVE_SPACETIME_TABLE"),
			   VarGroupingEditorDlg::OnSaveSpaceTimeTableClick )
    EVT_BUTTON( XRCID("ID_CREATE_GRP_BUTTON"),
			   VarGroupingEditorDlg::OnCreateGrpClick )
	EVT_BUTTON( XRCID("ID_UNGROUP_BUTTON"),
			   VarGroupingEditorDlg::OnUngroupClick )
	EVT_BUTTON( XRCID("ID_MOVE_UP_BUTTON"),
			   VarGroupingEditorDlg::OnMoveUpClick )
	EVT_BUTTON( XRCID("ID_MOVE_DOWN_BUTTON"),
			   VarGroupingEditorDlg::OnMoveDownClick )
	EVT_BUTTON( XRCID("ID_SORT_BUTTON"), VarGroupingEditorDlg::OnSortClick )
	EVT_BUTTON( XRCID("ID_PLACEHOLDER_BUTTON"),
			   VarGroupingEditorDlg::OnPlaceholderClick )
	EVT_BUTTON( XRCID("ID_ADD_TO_LIST_BUTTON"),
			   VarGroupingEditorDlg::OnAddToListClick )
	EVT_BUTTON( XRCID("ID_REMOVE_FR_LIST_BUTTON"),
			   VarGroupingEditorDlg::OnRemoveFrListClick )
	EVT_LIST_ITEM_ACTIVATED( XRCID("ID_UNGROUPED_LIST"),
                            VarGroupingEditorDlg::OnUngroupedListItemActivate )
	EVT_LIST_ITEM_SELECTED( XRCID("ID_UNGROUPED_LIST"),
						   VarGroupingEditorDlg::OnUngroupedListSelection )
	EVT_LIST_ITEM_DESELECTED( XRCID("ID_UNGROUPED_LIST"),
					   VarGroupingEditorDlg::OnUngroupedListSelection )
	EVT_LIST_ITEM_ACTIVATED( XRCID("ID_INCLUDE_LIST"),
							VarGroupingEditorDlg::OnIncludeListItemActivate )
	EVT_LIST_ITEM_SELECTED( XRCID("ID_INCLUDE_LIST"),
						   VarGroupingEditorDlg::OnIncludeListSelection )
	EVT_LIST_ITEM_DESELECTED( XRCID("ID_INCLUDE_LIST"),
							 VarGroupingEditorDlg::OnIncludeListSelection )
    EVT_LIST_BEGIN_LABEL_EDIT(XRCID("ID_INCLUDE_LIST"),
                            VarGroupingEditorDlg::OnIncludeListEdit)
    EVT_LIST_END_LABEL_EDIT(XRCID("ID_INCLUDE_LIST"),
                        VarGroupingEditorDlg::OnIncludeListEditEnd)
    EVT_LIST_BEGIN_DRAG(XRCID("ID_INCLUDE_LIST"),
                        VarGroupingEditorDlg::OnIncludeListBeginDrag)
EVT_LIST_BEGIN_DRAG(XRCID("ID_INCLUDE_LIST"),
                    VarGroupingEditorDlg::OnIncludeListBeginDrag)

	EVT_LISTBOX( XRCID("ID_GROUPED_LIST"),
				VarGroupingEditorDlg::OnGroupedListSelection )

	EVT_TEXT( XRCID("ID_NEW_GROUP_NAME_TXT_CTRL"),
			 VarGroupingEditorDlg::OnNewGroupNameChange )
	EVT_BUTTON( XRCID("ID_UNGROUPED_VARS_HELP"),
			   VarGroupingEditorDlg::OnUngroupedVarsHelp )
	EVT_BUTTON( XRCID("ID_NEW_GROUP_HELP"),
			   VarGroupingEditorDlg::OnNewGroupHelp )
	EVT_BUTTON( XRCID("ID_CUR_GROUPED_HELP"),
			   VarGroupingEditorDlg::OnCurGroupedHelp )
END_EVENT_TABLE()

VarGroupingEditorDlg::VarGroupingEditorDlg(Project* project,
										   wxWindow* parent,
										   const wxString& title,
										   const wxPoint& pos,
										   const wxSize& size, long style)
: table_int(project->GetTableInt()),
frames_manager(project->GetFramesManager()),
table_state(project->GetTableState()),
highlight_state(project->GetHighlightState()),
wmi(project->GetWManInt()),
common_empty(true), all_init(false)
{
	CreateControls();
	SetPosition(pos);
	SetTitle(title);
    Centre();
	
	frames_manager->registerObserver(this);
	//table_state->registerObserver(this);
	all_init = true;
}

VarGroupingEditorDlg::~VarGroupingEditorDlg()
{
	LOG_MSG("In ~VarGroupingEditorDlg::VarGroupingEditorDlg");
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
}

void VarGroupingEditorDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),  "ID_VAR_GROUPING_EDITOR");
	new_group_name_txt_ctrl = wxDynamicCast(FindWindow(XRCID("ID_NEW_GROUP_NAME_TXT_CTRL")), wxTextCtrl);
	new_field_type_stat_txt = wxDynamicCast(FindWindow(XRCID("ID_NEW_FIELD_TYPE_STAT_TXT")), wxStaticText);
	move_up_button = wxDynamicCast(FindWindow(XRCID("ID_MOVE_UP_BUTTON")),  wxButton);
	move_down_button = wxDynamicCast(FindWindow(XRCID("ID_MOVE_DOWN_BUTTON")), wxButton);
	sort_button = wxDynamicCast(FindWindow(XRCID("ID_SORT_BUTTON")), wxButton);

	placeholder_button =  wxDynamicCast(FindWindow(XRCID("ID_PLACEHOLDER_BUTTON")), wxButton);
	
	include_list = wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST")),  wxListCtrl);
    include_list->AppendColumn("Time");
    include_list->SetColumnWidth(0, 50);
	include_list->AppendColumn("Name");
	include_list->SetColumnWidth(1, 120);
	
		
	include_list_stat_txt =  wxDynamicCast(FindWindow(XRCID("ID_INCLUDE_LIST_STAT_TXT")),  wxStaticText);
	
	add_to_list_button = wxDynamicCast(FindWindow(XRCID("ID_ADD_TO_LIST_BUTTON")), wxButton);
	remove_fr_list_button = wxDynamicCast(FindWindow(XRCID("ID_REMOVE_FR_LIST_BUTTON")), wxButton);
	
	group_button = wxDynamicCast(FindWindow(XRCID("ID_CREATE_GRP_BUTTON")),  wxButton);
	ungroup_button = wxDynamicCast(FindWindow(XRCID("ID_UNGROUP_BUTTON")),   wxButton);
	
	ungrouped_list = wxDynamicCast(FindWindow(XRCID("ID_UNGROUPED_LIST")), wxListCtrl);
	ungrouped_list->AppendColumn("Name");
	ungrouped_list->SetColumnWidth(0, 120);
	ungrouped_list->AppendColumn("Type");
	ungrouped_list->SetColumnWidth(1, 50);

	grouped_list = wxDynamicCast(FindWindow(XRCID("ID_GROUPED_LIST")), wxListBox);

    
    save_spacetime_button =wxDynamicCast(FindWindow(XRCID("ID_SAVE_SPACETIME_TABLE")), wxButton);
    
	ResetAllButtons();
	UpdateCommonType();
		
	all_init = true;
	InitAll();
	UpdateButtons();
    
    orgin.x = 0;
    orgin.y = 0;
    
    include_list->Bind(wxEVT_LEFT_DCLICK, &VarGroupingEditorDlg::OnIncludeListDblClicked, this);
    include_list->Bind(wxEVT_RIGHT_UP, &VarGroupingEditorDlg::OnIncludeListRightUp, this);
    include_list->Bind(wxEVT_MOTION, &VarGroupingEditorDlg::OnIncludeListMouse, this);
    include_list->Bind(wxEVT_LEFT_UP, &VarGroupingEditorDlg::OnIncludeListMouse, this);
}

/** This should completely reset all info based on data from TableInterface.
 This is called when dialog is first constructed, and can be called again
 when changes TableState::update is called. */
void VarGroupingEditorDlg::InitAll()
{
	new_group_name_txt_ctrl->SetValue(wxEmptyString);
		
	std::set<wxString> empty_set;
	InitUngroupedList(empty_set);
	
	include_list->DeleteAllItems();
	int table_ts = table_int->GetTimeSteps();
	if (table_ts > 1) {
		for (int t=0; t<table_ts; ++t) {
            include_list->InsertItem(t, table_int->GetTimeString(t));
			include_list->SetItem(t, 1, wxEmptyString);
		}
	}
	UpdateTimeStepsTxt();
	
	InitGroupedList();
}

void VarGroupingEditorDlg::InitUngroupedList(std::set<wxString>& excl_nms)
{
	ungrouped_list->DeleteAllItems();
	std::vector<int> col_id_map;
	table_int->FillColIdMap(col_id_map);
	for (int i=0, ug_cnt=0, cid_sz=col_id_map.size(); i<cid_sz; ++i) {
		int col = col_id_map[i];
		if (table_int->IsColTimeVariant(col) ||
			excl_nms.find(table_int->GetColName(col)) != excl_nms.end()) {
			continue;
		}
		
		GdaConst::FieldType type = table_int->GetColType(col);
		wxString type_str;
		if (type == GdaConst::double_type || type == GdaConst::long64_type) {
			type_str << "num";
		} else if (type == GdaConst::string_type) {
			type_str << "str";
		} else if (type == GdaConst::date_type) {
			type_str << "date";
		}
		ungrouped_list->InsertItem(ug_cnt, wxEmptyString);
		ungrouped_list->SetItem(ug_cnt, 0, table_int->GetColName(col));
		ungrouped_list->SetItem(ug_cnt, 1, type_str);
		++ug_cnt;
	}
}

void VarGroupingEditorDlg::InitGroupedList()
{
	grouped_list->Clear();	
	std::vector<int> col_id_map;
	table_int->FillColIdMap(col_id_map);
	for (int i=0, ug_cnt=0, cid_sz=col_id_map.size(); i<cid_sz; ++i) {
		int col = col_id_map[i];
		if (table_int->IsColTimeVariant(col)) {
			grouped_list->Append(table_int->GetColName(col));
			continue;
		}
	}
}

void VarGroupingEditorDlg::OnClose(wxCloseEvent& event)
{
	// Note: it seems that if we don't explictly capture the close event
	//       and call Destory, then the destructor is not called.
	Destroy();
}

void VarGroupingEditorDlg::OnSaveSpaceTimeTableClick( wxCommandEvent& event )
{
    
	std::vector<wxString> tm_strs;
	table_int->GetTimeStrings(tm_strs);
    int n_obs = table_int->GetNumberRows();
    int n_ts = tm_strs.size();
    int n = n_ts * n_obs;
   
    std::vector<wxString> time_stack;
    std::vector<wxInt64> select_stack;
    std::vector<wxInt64> id_stack;
    std::vector<wxInt64> new_ids;
   
    bool has_highligh = false;
    const std::vector<bool>& hs(highlight_state->GetHighlight());
    
    int idx = 1;
    for (int t=0; t<n_ts; t++) {
        for (int j=0; j<n_obs; j++) {
            if (hs[j]) {
                select_stack.push_back(1);
                has_highligh = true;
            } else {
                select_stack.push_back(0);
            }
            time_stack.push_back(tm_strs[t]);
            id_stack.push_back(j);
            new_ids.push_back(idx);
            idx += 1;
        }
    }
    
    // create in-memory table
    OGRTable* mem_table_int = new OGRTable(n);
    
    OGRColumn* id_col = new OGRColumnInteger("STID", 18, 0, n);
    id_col->UpdateData(new_ids);
    mem_table_int->AddOGRColumn(id_col);
    
    if (!id_stack.empty()) {
        
        bool using_default_id = true;
        if (wmi) {
            boost::uuids::uuid default_wid = wmi->GetDefault();
            if (!default_wid.is_nil()) {
                GalWeight* gw = wmi->GetGal(default_wid);
                
                std::vector<wxString> id_vec;
                int c_id = table_int->FindColId(gw->id_field);
                if (c_id > 0) {
                    table_int->GetColData(c_id, 1, id_vec);
                    
                    std::vector<wxString> new_id_vec;
                    for (int ii=0; ii<n; ii++) {
                        new_id_vec.push_back(id_vec[id_stack[ii]]);
                    }
                    OGRColumn* id_col = new OGRColumnString(gw->id_field, 50, 0, n);
                    id_col->UpdateData(new_id_vec);
                    mem_table_int->AddOGRColumn(id_col);
                    using_default_id = false;
                }
            }
        }
        /*
        if (using_default_id) {
            // if no weights/id_field, then use 0,1,2,...
            OGRColumn* id_col = new OGRColumnInteger("ORIG_ID", 18, 0, n);
            id_col->UpdateData(id_stack);
            mem_table_int->AddOGRColumn(id_col);
        }
         */
    }
    
    if (!time_stack.empty()) {
        OGRColumn* time_col = new OGRColumnString("TIME", 50, 0, n);
        time_col->UpdateData(time_stack);
        mem_table_int->AddOGRColumn(time_col);
    }
    
    if (!select_stack.empty() && has_highligh) {
        OGRColumn* select_col = new OGRColumnInteger("SELECT", 18, 0, n);
        select_col->UpdateData(select_stack);
        mem_table_int->AddOGRColumn(select_col);
    }
   
    int n_col = table_int->GetNumberCols();
    for (int i=0; i<n_col; i++) {
        if (table_int->IsColTimeVariant(i)) {
            wxString col_name(table_int->GetColName(i));
            
            GdaConst::FieldType ft = table_int->GetColType(i);
            
            if (ft == GdaConst::long64_type) {
                std::vector<wxInt64> stack_dat;
                stack_dat.reserve(n);
                for (int t=0; t<n_ts; t++) {
                    std::vector<wxInt64> dat;
                    table_int->GetColData(i, t, dat);
                    stack_dat.insert(stack_dat.end(), dat.begin(), dat.end());
                }
                OGRColumn* var_col = new OGRColumnInteger(col_name, 18, 0, n);
                var_col->UpdateData(stack_dat);
                mem_table_int->AddOGRColumn(var_col);
                
            } else if (ft == GdaConst::double_type) {
                int n_decimal = -1;
                std::vector<double> stack_dat;
                stack_dat.reserve(n);
                for (int t=0; t<n_ts; t++) {
                    std::vector<double> dat;
                    table_int->GetColData(i, t, dat);
                    stack_dat.insert(stack_dat.end(), dat.begin(), dat.end());
                    int deci = table_int->GetColDecimals(i, t);
                    if (deci > n_decimal)
                        n_decimal = deci;
                }
                OGRColumn* var_col = new OGRColumnDouble(col_name, 18, n_decimal, n);
                var_col->UpdateData(stack_dat);
                mem_table_int->AddOGRColumn(var_col);
                
            }
        }
    }
    
    // export
    ExportDataDlg dlg(this, (TableInterface*)mem_table_int);
    if (dlg.ShowModal() == wxID_OK) {
        wxString ds_name = dlg.GetDatasourceName();
        wxFileName wx_fn(ds_name);
        
        wx_fn.SetExt("gal");
        wxString ofn(wx_fn.GetFullPath());
        
        // save weights
        if (wmi) {
            boost::uuids::uuid default_wid = wmi->GetDefault();
            if (!default_wid.is_nil()) {
                GeoDaWeight* w = wmi->GetWeights(default_wid);
                w->SaveSpaceTimeWeights(ofn, wmi, table_int);
            }
        }
    }
    
    // clean memory
    delete mem_table_int;
}

void VarGroupingEditorDlg::OnCreateGrpClick( wxCommandEvent& event )
{
	if (!all_init) return;
	
	// check that new field name is valid
	wxString grp_nm = new_group_name_txt_ctrl->GetValue();
	grp_nm.Trim(true);
	grp_nm.Trim(false);
	
	if (!table_int->IsValidGroupName(grp_nm) ||
		table_int->DoesNameExist(grp_nm, false)) {
		wxString m;
		m << "Variable name \"" << grp_nm << "\" is either a duplicate\n";
		m << "or is invalid. Please enter an alternative,\n";
		m << "non-duplicate variable name.";
		wxMessageDialog dlg (this, m, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	int tot_items = include_list->GetItemCount();
	std::vector<int> cols(tot_items);
	for (int t=0; t<tot_items; t++) {
		wxString nm = include_list->GetItemText(t, 1);
		if (nm == GdaConst::placeholder_str) {
			cols[t] = -1;
		} else {
			cols[t] = table_int->FindColId(nm);
		}
	}
	
	if (table_int->GetTimeSteps() == 1) {
		if (table_int->GetTimeSteps() == 1 && tot_items > 1) {
			if (table_state->GetNumDisallowTimelineChanges() > 0) {
				wxString msg = table_state->GetDisallowTimelineChangesMsg();
				wxMessageDialog dlg (this, msg, "Warning",
									 wxOK | wxICON_INFORMATION);
				dlg.ShowModal();
				return;
			}
		}
		// This is a special case since we are increasing
		// the number of table time steps.
		for (int t=1; t<tot_items; ++t) {
			wxString s;
			s << "time " << t;
			table_int->InsertTimeStep(t, s);
		}
	}
	
	table_int->GroupCols(cols, grp_nm, cols[0]);
	UpdateButtons();
	GdaFrame::GetGdaFrame()->UpdateToolbarAndMenus();
}

void VarGroupingEditorDlg::OnUngroupClick( wxCommandEvent& event )
{
	using namespace std;
	wxString grp_nm = grouped_list->GetStringSelection();
	if (grp_nm == "") return;
	int col = table_int->FindColId(grp_nm);
	if (col < 0) return;
		
	int tms = table_int->GetColTimeSteps(col);
	LOG(tms);
	set<wxString> col_nms_set;
	vector<wxString> col_nms(tms);
	for (int t=0; t<tms; ++t) {
		wxString nm = table_int->GetColName(col, t);
		if (nm.IsEmpty()) nm = GdaConst::placeholder_str;
		col_nms[t] = nm;
		col_nms_set.insert(nm);
	}
	
	GdaConst::FieldType type = table_int->GetColType(col);
	
	table_int->UngroupCol(col);
	
	new_group_name_txt_ctrl->SetValue(grp_nm);
	if (type == GdaConst::double_type || type == GdaConst::long64_type) {
		common_type = "num";
	} else if (type == GdaConst::date_type) {
		common_type = "date";
	} else {
		common_type = "string";
	}
	common_empty = false;
	UpdateCommonType();
	
	include_list->DeleteAllItems();
	for (int t=0; t<tms; ++t) {
		include_list->InsertItem(t, wxEmptyString);
		include_list->SetItem(t, 1, col_nms[t]);
		include_list->SetItem(t, 0, table_int->GetTimeString(t));
	}

	InitUngroupedList(col_nms_set);
	UpdateTimeStepsTxt();

	UpdateButtons();
}

/** Shift selected items, including selection highlighting,
 backwards in time. */
void VarGroupingEditorDlg::OnMoveUpClick( wxCommandEvent& event )
{
	LOG_MSG("In VarGroupingEditorDlg::OnMoveUpClick");
	if (!all_init) return;
	int item_cnt = include_list->GetItemCount();
	int sel_count = include_list->GetSelectedItemCount();
	if (sel_count == 0) return;
	list<int> sel_pos = GetListSel(include_list);
	if (sel_pos.front() == 0) return;
	set<int> sel_pos_set;
	BOOST_FOREACH(int i, sel_pos) sel_pos_set.insert(i);
	list<int> unsel_pos;
	for (int i=0; i<item_cnt; ++i) {
		if (sel_pos_set.find(i) == sel_pos_set.end()) {
			unsel_pos.push_back(i);
			LOG(i);
		}
	}
	vector<wxString> orig(item_cnt);
	for (int i=0; i<item_cnt; i++) orig[i] = include_list->GetItemText(i, 1);
	UnselectAll(include_list);
	set<int> new_pos_set;
	BOOST_FOREACH(int i, sel_pos) {
		include_list->SetItem(i-1, 0, orig[i]);
		SelectItem(include_list, i-1);
		new_pos_set.insert(i-1);
	}
	int free_pos = 0;
	BOOST_FOREACH(int i, unsel_pos) {
		while (new_pos_set.find(free_pos) != new_pos_set.end()) ++free_pos;
		include_list->SetItem(free_pos, 0, orig[i]);
		++free_pos;
	}
	UpdateButtons();
}

/** Shift selected items, including selection highlighting,
 forwards in time */
void VarGroupingEditorDlg::OnMoveDownClick( wxCommandEvent& event )
{
	LOG_MSG("In VarGroupingEditorDlg::OnMoveDownClick");
	if (!all_init) return;
	int item_cnt = include_list->GetItemCount();
	int sel_count = include_list->GetSelectedItemCount();
	if (sel_count == 0) return;
	list<int> sel_pos = GetListSel(include_list);
	if (sel_pos.back() == item_cnt-1) return;
	set<int> sel_pos_set;
	BOOST_FOREACH(int i, sel_pos) sel_pos_set.insert(i);
	list<int> unsel_pos;
	for (int i=0; i<item_cnt; ++i) {
		if (sel_pos_set.find(i) == sel_pos_set.end()) {
			unsel_pos.push_back(i);
			LOG(i);
		}
	}
	vector<wxString> orig(item_cnt);
	for (int i=0; i<item_cnt; i++)
        orig[i] = include_list->GetItemText(i, 1);
    
	UnselectAll(include_list);
	set<int> new_pos_set;
	BOOST_FOREACH(int i, sel_pos) {
		include_list->SetItem(i+1, 0, orig[i]);
		SelectItem(include_list, i+1);
		new_pos_set.insert(i+1);
	}
	int free_pos = 0;
	BOOST_FOREACH(int i, unsel_pos) {
		while (new_pos_set.find(free_pos) != new_pos_set.end()) ++free_pos;
		include_list->SetItem(free_pos, 0, orig[i]);
		++free_pos;
	}
	UpdateButtons();
}

/** Sort items in place ignoring blanks and placeholders.  Highlighting
 moves with items. */
void VarGroupingEditorDlg::OnSortClick( wxCommandEvent& event )
{
	LOG_MSG("In VarGroupingEditorDlg::OnSortClick");
	if (!all_init) return;
	
	list<wxString> all_str = GetListAllStrs(include_list, 1);
	list<int> nm_locs;
	vector<wxString> sorted_nms;
	set<wxString> sel_nms;
	int loc=0;
	BOOST_FOREACH(const wxString& s, all_str) {
		if (!s.IsEmpty() && s != GdaConst::placeholder_str) {
			nm_locs.push_back(loc);
			sorted_nms.push_back(s);
			if (IsItemSel(include_list, loc)) {
				UnselectItem(include_list, loc);
				sel_nms.insert(s);
			}
		}
		++loc;
	}
	
	sort(sorted_nms.begin(), sorted_nms.end());
	list<int>::iterator pos = nm_locs.begin();
	BOOST_FOREACH(const wxString& s, sorted_nms) {
		include_list->SetItem(*pos, 1, s);
		if (sel_nms.find(s) != sel_nms.end())
            SelectItem(include_list, *pos);
		++pos;
	}
		
	UpdateButtons();
}

// NOTE: this "placeholder" button is changed to "Edit" button
void VarGroupingEditorDlg::OnPlaceholderClick( wxCommandEvent& event )
{
    wxPoint pt = GetPosition();
    std::list<FramesManagerObserver*> observers(frames_manager->getCopyObservers());
    std::list<FramesManagerObserver*>::iterator it;
    for (it=observers.begin(); it != observers.end(); ++it) {
        if (TimeEditorDlg* w = dynamic_cast<TimeEditorDlg*>(*it)) {
            LOG_MSG("TimeEditorDlg already opened.");
            w->Show(true);
            w->Maximize(false);
            w->Raise();
            w->SetPosition(wxPoint(pt.x + 50, pt.y + 30));
            return;
        }
    }
    
    LOG_MSG("Opening a new TimeEditorDlg");
    TimeEditorDlg* dlg = new TimeEditorDlg(0,frames_manager, table_state,                                       table_int);
    dlg->Show(true);
    dlg->SetPosition(wxPoint(pt.x + 50, pt.y + 30));

}

void VarGroupingEditorDlg::OnUngroupedListItemActivate( wxListEvent& event )
{
	wxCommandEvent ce;
	OnAddToListClick(ce);
}

void VarGroupingEditorDlg::OnAddToListClick( wxCommandEvent& event )
{
	using namespace std;
	LOG_MSG("In VarGroupingEditorDlg::OnAddToListClick");
	if (!all_init) return;
	
	int sel_cnt = ungrouped_list->GetSelectedItemCount();
	if (sel_cnt == 0) return;
	
	list<wxString> typs = GetListSelStrs(ungrouped_list, 1);
	set<wxString> pool;
	BOOST_FOREACH(const wxString& s, typs) pool.insert(s);
	if (pool.size() != 1) return;
	
	if (!common_empty && common_type != typs.front()) return;
	
	int cur_tm_steps = table_int->GetTimeSteps();
	if (cur_tm_steps > 1) {
		int empty_spots = cur_tm_steps - GetIncListNonPlaceholderCnt();
		if (sel_cnt > empty_spots) return;
	}
	
	// At this point, we know for sure operation is legal
	common_empty = false;
	common_type = typs.front();
	UpdateCommonType();
	
	// attempt to add after last selected item, but if that won't
	// fit, then add into empty spots from the beginning.  If
	// placeholders must be overwritten, then do so as well.
	
	bool overwrite_plhdr = false;
	int room_with_plhdr = -1;
	if (cur_tm_steps > 1) {
		room_with_plhdr = cur_tm_steps - GetIncListNameCnt();
		if (room_with_plhdr < sel_cnt) overwrite_plhdr = true;
	}
	
	list<int> inc_list_sel = GetListSel(include_list);
	int last_inc_list_sel = 0;
	if (inc_list_sel.size() > 0) last_inc_list_sel = inc_list_sel.back();
	
	bool fill_from_top = false;
	if (cur_tm_steps == 1) {
		fill_from_top = true;
		overwrite_plhdr = true;
	} else if (overwrite_plhdr) {
		fill_from_top = true;
	} else if (cur_tm_steps - last_inc_list_sel < sel_cnt) {
		fill_from_top = true;
	}
	
	if (cur_tm_steps == 1) {
		// add time periods as needed
		int item_cnt = include_list->GetItemCount();
		int room = item_cnt - GetIncListNonPlaceholderCnt();
		if (sel_cnt > room) {
			int diff = sel_cnt - room;
			for (int i=0; i<diff; ++i) {
				wxString t;
				if (item_cnt+i == 0) {
					t << table_int->GetTimeString(0);
				} else {
					t << "time " << item_cnt+i;
				}
                include_list->InsertItem(item_cnt+i, t);
                
				include_list->SetItem(item_cnt+i, 1, wxEmptyString);
			}
		}
	}
	
	list<wxString> ung_sel_strs = GetListSelStrs(ungrouped_list, 0);
	
	int insert_pos = 0;
	if (!fill_from_top) insert_pos = last_inc_list_sel;
	BOOST_FOREACH(const wxString& s, ung_sel_strs) {
		while (!((overwrite_plhdr && (include_list->GetItemText(insert_pos, 1) == GdaConst::placeholder_str)) || include_list->GetItemText(insert_pos, 1).IsEmpty()))
		{
			++insert_pos;
		}
		include_list->SetItem(insert_pos, 1, s);
		++insert_pos;
	}
	
	// Remove added items from ungrouped_list
	set<wxString> add_nms;
	BOOST_FOREACH(const wxString& s, ung_sel_strs) add_nms.insert(s);
	InitUngroupedList(add_nms);
	
	int inc_list_cnt = GetIncListNameCnt();
	UpdateTimeStepsTxt();
	
	if (new_group_name_txt_ctrl->GetValue().IsEmpty() && inc_list_cnt > 1) {
		vector<wxString> names(inc_list_cnt);
		for (int i=0; i<inc_list_cnt; ++i) {
			names[i] = include_list->GetItemText(i, 1);
		}
		wxString grp_nm = table_int->SuggestGroupName(names);
		new_group_name_txt_ctrl->SetValue(grp_nm);
	}
	
	UpdateButtons();
}

void VarGroupingEditorDlg::OnIncludePopupClick(wxCommandEvent &evt)
{
    int menu_id = evt.GetId();
    if (menu_id == XRCID("INCLUDE_ADD_TIME")) {
        includeListAddNewTime();
    } else if (menu_id == XRCID("INCLUDE_DELETE_TIME")) {
        includeListDeleteTime();
    }
}

void VarGroupingEditorDlg::OnIncludeListItemActivate( wxListEvent& event )
{
	//wxCommandEvent ce;
	//OnPlaceholderClick(ce);
    wxListItem item = event.GetItem();
    
    include_list->EditLabel(item.GetId());
}

void VarGroupingEditorDlg::includeListAddNewTime()
{
    int time_step = include_list->GetItemCount();
    wxString lbl;
    lbl << "time " << time_step;
    include_list->InsertItem(time_step, lbl);
        
    // sync TableState
    table_int->InsertTimeStep(time_step, lbl);
    
    UpdateTimeStepsTxt();
}

void VarGroupingEditorDlg::includeListDeleteTime()
{
    std::list<int> sels = GetListSel(include_list);
    if (!sels.empty()) {
        BOOST_FOREACH(int i, sels) {
            include_list->DeleteItem(i);
            table_int->RemoveTimeStep(i);
        }
    }
    
    list<wxString> inc_strs = GetListAllStrs(include_list, 1);
    set<wxString> excl;
    BOOST_FOREACH(const wxString& s, inc_strs) if (s!="") excl.insert(s);
    InitUngroupedList(excl);
    
    UpdateTimeStepsTxt();
}

void VarGroupingEditorDlg::OnIncludeListDblClicked( wxMouseEvent& event)
{
    list<int> inc_sel_pos = GetListSel(include_list);
    BOOST_FOREACH(int i, inc_sel_pos) {
        include_list->EditLabel(i);
        break;
    }
}


void VarGroupingEditorDlg::OnIncludeListRightUp( wxMouseEvent& event)
{
    wxMenu mnu;
    int id1 = XRCID("INCLUDE_ADD_TIME");
    mnu.Append(id1, 	"Add Time");
    mnu.Append(XRCID("INCLUDE_DELETE_TIME"), 	"Remove Time");
    mnu.Connect(wxEVT_COMMAND_MENU_SELECTED,
                wxCommandEventHandler(VarGroupingEditorDlg::OnIncludePopupClick),
                NULL, this);
    PopupMenu(&mnu);
}

void VarGroupingEditorDlg::OnRemoveFrListClick( wxCommandEvent& event )
{
	using namespace std;
	LOG_MSG("In VarGroupingEditorDlg::OnRemoveFrListClick");
	if (!all_init) return;
	
	list<int> inc_sel_pos = GetListSel(include_list);
	BOOST_FOREACH(int i, inc_sel_pos) {
		include_list->SetItem(i, 1, "");
		include_list->SetItemState(i, 1, wxLIST_STATE_SELECTED);
	}
	
	if (table_int->GetTimeSteps() == 1) {
		// remove any unused time periods at the end
		int inc_cnt = include_list->GetItemCount();
		bool done = false;
		for (int i=inc_cnt-1; i>=0 && !done; --i) {
			if (!include_list->GetItemText(i, 1).IsEmpty()) {
				done = true;
			} else {
				include_list->DeleteItem(i);
			}
		}
	}
	
	list<wxString> inc_strs = GetListAllStrs(include_list, 1);
	set<wxString> excl;
	BOOST_FOREACH(const wxString& s, inc_strs) if (s!="") excl.insert(s);
	InitUngroupedList(excl);
	
	UpdateTimeStepsTxt();
	
	if (GetIncListNonPlaceholderCnt() == 0) {
		common_empty = true;
		UpdateCommonType();
	}
	
	UpdateButtons();
}

void VarGroupingEditorDlg::OnUngroupedListSelection( wxListEvent& event )
{
	LOG_MSG("In VarGroupingEditorDlg::OnUngroupedListSelection");
	if (!all_init) return;
	UpdateButtons();
}

void VarGroupingEditorDlg::OnIncludeListSelection( wxListEvent& event )
{
	LOG_MSG("In VarGroupingEditorDlg::OnIncludeListSelection");
	if (!all_init) return;
     dSource = event.GetIndex();
	UpdateButtons();
}

void VarGroupingEditorDlg::OnIncludeListBeginDrag( wxListEvent& event )
{
    // only capture mouse events on drag
    CaptureMouse();
   
}

void VarGroupingEditorDlg::GetPlace( wxPoint mPoint)
{
    // gets point in respect to wxlistctrl
    offset= mPoint - orgin;
}

int VarGroupingEditorDlg::ArrangeList(void)
{
    if (dSource == dTarget) {return -1;}
    wxListItem SourceItem;
    
    //  have to setmask along with setid inorder to work??
    SourceItem.SetMask(wxLIST_MASK_TEXT);
    SourceItem.SetId(dSource);
    include_list->GetItem(SourceItem);
    include_list->DeleteItem(dSource);
    include_list->InsertItem(dTarget,SourceItem.GetText());

    //   wxMessageBox("Arrange List Done","WxListCtrl",wxOK |wxICON_INFORMATION );
    return 0;
}

void VarGroupingEditorDlg::OnIncludeListMouse( wxMouseEvent& event)
{
    if (event.LeftUp()) {
        offset = event.GetPosition();
        int flags;
        dTarget = include_list->HitTest( offset, flags );
        if ( dTarget> -1 && (flags & wxLIST_HITTEST_ONITEM)) {
            int a =ArrangeList();
        }
        //ReleaseMouse();
    }
}

void VarGroupingEditorDlg::OnIncludeListEdit( wxListEvent& event )
{
    wxListItem item = event.GetItem();
    wxString val = item.GetText();
}


void VarGroupingEditorDlg::OnIncludeListEditEnd( wxListEvent& event )
{
    if (event.IsEditCancelled())
        return;
    
    int id = event.GetItem().GetId();
    wxString item_new = event.GetLabel();
    item_new.Trim(true);
    item_new.Trim(false);
    
    // ensure that new time label is unique and not empty
    bool is_dup = false;
    for (int t=0, tms=table_int->GetTimeSteps(); t<tms && !is_dup; ++t) {
        if (t != id && table_int->GetTimeString(t) == item_new)
            is_dup = true;
    }
    
    bool is_disallow = table_state->GetNumDisallowTimelineChanges() > 0;
    
    if (item_new.IsEmpty() || is_dup || is_disallow) {
        if (is_disallow && !(item_new.IsEmpty() || is_dup)) {
            wxString msg = table_state->GetDisallowTimelineChangesMsg();
            wxMessageDialog dlg(this, msg, "Warning",
                                wxOK | wxICON_INFORMATION);
            dlg.ShowModal();
        }
        LOG_MSG("Restoring item to original.");
        event.Veto();
        return;
    }
    
    // item_new is valid, proceed with rename
    table_int->RenameTimeStep(id, item_new);
}



void VarGroupingEditorDlg::OnGroupedListSelection( wxCommandEvent& event )
{
	LOG_MSG("In VarGroupingEditorDlg::OnGroupedListSelection");
	if (!all_init) return;
	UpdateButtons();
}

void VarGroupingEditorDlg::OnNewGroupNameChange( wxCommandEvent& event )
{
	if (!all_init) return;
	UpdateGroupButton();
}


void VarGroupingEditorDlg::UpdateGroupButton()
{
	group_button->Enable(!new_group_name_txt_ctrl->GetValue().IsEmpty() &&
						 (GetIncListNameCnt() == table_int->GetTimeSteps() ||
						  table_int->GetTimeSteps() == 1) &&
						 GetIncListNonPlaceholderCnt() > 0);
}

void VarGroupingEditorDlg::UpdateAddToListButton()
{
	// Enable add to list button when following conditions are met:

	// 1) Non-empty selection
	int sel_cnt = ungrouped_list->GetSelectedItemCount();
	if (sel_cnt == 0) {
		add_to_list_button->Enable(false);
		return;
	}
	
	// 2) Currently selected items are type compatible with each other
	std::list<wxString> typs = GetListSelStrs(ungrouped_list, 1);
	std::set<wxString> pool;
	BOOST_FOREACH(const wxString& s, typs) pool.insert(s);
	if (pool.size() != 1) {
		add_to_list_button->Enable(false);
		return;
	}
	
	// 3) Currently selected items are type compatible with items in
	//    include list currently
	if (!common_empty && common_type != typs.front()) {
		add_to_list_button->Enable(false);
		return;
	}
	
	// 4) Number of selected items is <= number of empty or placeholder items
	//    but current number of time steps is > 1
	int cur_tm_steps = table_int->GetTimeSteps();
	if (cur_tm_steps > 1) {
		int empty_spots = cur_tm_steps - GetIncListNonPlaceholderCnt();
		if (sel_cnt > empty_spots) {
			add_to_list_button->Enable(false);
			return;
		}
	}
	
	add_to_list_button->Enable(true);
}

void VarGroupingEditorDlg::UpdateButtons()
{
	using namespace std;
	list<wxString> sel_strs = GetListSelStrs(include_list, 1);
	int non_empty_cnt = 0;
	BOOST_FOREACH(const wxString& s, sel_strs) if (s != "") ++non_empty_cnt;
	list<int> sel_pos = GetListSel(include_list);
	int sel_first = -1;
	int sel_last = -1;
	if (sel_pos.size() > 0) {
		sel_first = sel_pos.front();
		sel_last = sel_pos.back();
	}
	
	int cnt = GetIncListNameCnt();
	int inc_item_cnt = include_list->GetItemCount();
	int inc_sel_cnt = include_list->GetSelectedItemCount();
	
	move_up_button->Enable(inc_sel_cnt > 0 && sel_first != 0);
	move_down_button->Enable(inc_sel_cnt > 0 && sel_last != inc_item_cnt-1);
	sort_button->Enable(cnt > 1);
	//placeholder_button->Enable((table_int->GetTimeSteps() == 1 &&
	//							inc_item_cnt > 0) ||
	//						   (sel_strs.size()-non_empty_cnt > 0));
    placeholder_button->Enable(true);
	
	UpdateAddToListButton();
	remove_fr_list_button->Enable(non_empty_cnt > 0);
	
	ungroup_button->Enable(grouped_list->GetSelection() != wxNOT_FOUND);
    
	save_spacetime_button->Enable(grouped_list->GetCount() > 0);
	
	UpdateGroupButton();
}

void VarGroupingEditorDlg::ResetAllButtons()
{
	group_button->Disable();
	ungroup_button->Disable();
	move_up_button->Disable();
	move_down_button->Disable();
	sort_button->Disable();
	placeholder_button->Disable();
	add_to_list_button->Disable();
	remove_fr_list_button->Disable();
    save_spacetime_button->Disable();
}

void VarGroupingEditorDlg::UpdateCommonType()
{
	if (common_empty) {
		new_field_type_stat_txt->SetLabelText("");
	} else if (common_type == "str") {
		new_field_type_stat_txt->SetLabelText("string");
	} else if (common_type == "date") {
		new_field_type_stat_txt->SetLabelText("date");
	} else if (common_type == "num") {
		new_field_type_stat_txt->SetLabelText("numeric");
	}
}

void VarGroupingEditorDlg::UpdateTimeStepsTxt()
{
	wxString s;
	int cur_tm_steps = table_int->GetTimeSteps();
	if (cur_tm_steps > 1) {
		s << GetIncListNameCnt() << " of " << cur_tm_steps;
	} else {
		s << GetIncListNameCnt();
	}
	s << " variables to include";
	include_list_stat_txt->SetLabelText(s);
	//wxSizer* szr = include_list_stat_txt->GetContainingSizer();
	//if (szr) szr->Layout();
	//Fit();
	Refresh();
}

void VarGroupingEditorDlg::OnUngroupedVarsHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "List of existing variables that are not part of a group. ";
	msg << "Move variables that measure the same property, ";
	msg << "but at different times into the list to the right.\n\n";
	msg << "For example, to combine the four population variables ";
	msg << "POP_2000, POP_2001, POP_2002, and POP_2003 into one ";
	msg << "grouped variable called Population, move each of these to the ";
	msg << "list on the right and specify Population for the new grouped ";
	msg << "variable name.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void VarGroupingEditorDlg::OnNewGroupHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "Choose existing non-grouped variables, ";
	msg << "one variable for each time period. ";
	msg << "\n\n";
	msg << "All variables to be grouped together must be of similar types.";
	msg << "Enter desired name for new grouped variable. For example, ";
	msg << "if grouping the four existing variables ";
	msg << "POP_2000, POP_2001, POP_2002, and POP_2003, ";
	msg << "a logical name-choice could be Population.";
	msg << "\n\n";
	msg << "Time periods with missing data can be denoted ";
	msg << "by selecting the empty time-slot and pressing ";
	msg << "the Placeholder button.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void VarGroupingEditorDlg::OnCurGroupedHelp( wxCommandEvent& event )
{
	wxString msg;
	msg << "This is the list of existing grouped variables for ";
	msg << "reference purposes. As new groups are ";
	msg << "created, they will appear on this list.";
	wxMessageDialog dlg (this, msg, "Help", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void VarGroupingEditorDlg::update(FramesManager* o)
{	
}

void VarGroupingEditorDlg::update(TableState* o)
{
	TableState::EventType ev_type = o->GetEventType();
	if (ev_type != TableState::empty &&
		ev_type != TableState::col_data_change)
	{
		common_empty = true;
		InitAll();
		ResetAllButtons();
		UpdateCommonType();
	}
}

std::list<int> VarGroupingEditorDlg::GetListSel(wxListCtrl* lc)
{
	std::list<int> l;
	if (!lc) return l;
	long item = -1;
	for ( ;; ) {
		item = lc->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if ( item == -1 ) break;
		l.push_back(item);
	}
	return l;
}

std::list<wxString> VarGroupingEditorDlg::GetListSelStrs(wxListCtrl* lc,
														 int col)
{
	std::list<wxString> l;
	if (!lc) return l;
	long item = -1;
	for ( ;; ) {
		item = lc->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if ( item == -1 ) break;
		l.push_back(lc->GetItemText(item, col));
	}
	return l;
}

std::list<wxString> VarGroupingEditorDlg::GetListAllStrs(wxListCtrl* lc,
														 int col)
{
	std::list<wxString> l;
	if (!lc) return l;
	long item = -1;
	for ( ;; ) {
		item = lc->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_DONTCARE);
		if ( item == -1 ) break;
		l.push_back(lc->GetItemText(item, col));
	}
	return l;
}

void VarGroupingEditorDlg::UnselectAll(wxListCtrl* lc)
{
	if (!lc) return;
	int cnt = lc->GetItemCount();
	for (int i=0; i<cnt; ++i) {
		lc->SetItemState(i, 0, wxLIST_STATE_SELECTED);
	}
}

void VarGroupingEditorDlg::SelectItem(wxListCtrl* lc, int i)
{
	if (!lc || i<0 || i>=lc->GetItemCount()) return;
	lc->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void VarGroupingEditorDlg::UnselectItem(wxListCtrl* lc, int i)
{
	if (!lc || i<0 || i>=lc->GetItemCount()) return;
	lc->SetItemState(i, 0, wxLIST_STATE_SELECTED);
}

bool VarGroupingEditorDlg::IsItemSel(wxListCtrl* lc, int i)
{
	if (!lc || i<0 || i>=lc->GetItemCount()) return false;
	return lc->GetItemState(i, wxLIST_STATE_SELECTED) != 0;
}

int VarGroupingEditorDlg::GetIncListNameCnt()
{
	if (!all_init || !include_list) return 0;
	int nm_cnt = 0;
	for (int i=0, il_sz=include_list->GetItemCount(); i<il_sz; ++i) {
		if (!include_list->GetItemText(i, 1).IsEmpty()) ++nm_cnt;
	}
	return nm_cnt;
}

int VarGroupingEditorDlg::GetIncListNonPlaceholderCnt()
{
	if (!all_init || !include_list) return 0;
	int nm_cnt = 0;
	for (int i=0, il_sz=include_list->GetItemCount(); i<il_sz; ++i) {
		if (!include_list->GetItemText(i, 1).IsEmpty() &&
			include_list->GetItemText(i, 1) != GdaConst::placeholder_str )
		{
			++nm_cnt;
		}
	}
	return nm_cnt;
}

