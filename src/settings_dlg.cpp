/*
Stackistry - astronomical image stacking
Copyright (C) 2016 Filip Szczerek <ga.software@yahoo.com>

This file is part of Stackistry.

Stackistry is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Stackistry is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stackistry.  If not, see <http://www.gnu.org/licenses/>.

File description:
    Processing settings dialog implementation.
*/

#include <vector>

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/textview.h>

#include "config.h"
#include "settings_dlg.h"
#include "utils.h"

const char *stackingCriterionStr[] =
{
    "percent of the best fragments",      // SKRY_PERCENTAGE_BEST
    "percent or better relative quality", // SKRY_MIN_REL_QUALITY
    "best fragments",                     // SKRY_NUMBER_BEST
};

const char *outputSaveModeStr[] =
{
    "disabled",                   // OutputSaveMode::NONE
    "in source video's folder",   // OutputSaveMode::SOURCE_PATH
    "specify folder"              // OutputSaveMode::SPECIFIED_PATH
};

c_SettingsDlg::c_SettingsDlg(const std::vector<std::string> &jobNames)
{
    set_title("Processing settings");
    InitControls(jobNames);
    Utils::RestorePosSize(Configuration::SettingsDlgPosSize,  *this);
}

Gtk::HBox *PackIntoHBox(std::vector<Gtk::Widget*> widgets)
{
    auto box = Gtk::manage(new Gtk::HBox());
    for (auto &w: widgets)
    {
        box->pack_start(*w, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);
    }
    box->show();
    return box;
}

std::vector<Glib::RefPtr<Gtk::FileFilter>> GetInputImageFilters()
{
    decltype(GetInputImageFilters()) result;

    auto fltAll = Gtk::FileFilter::create();
    fltAll->add_pattern("*");
    fltAll->set_name("All files");
    result.push_back(fltAll);

    auto fltBmp = Gtk::FileFilter::create();
    fltBmp->add_pattern("*.tif");
    fltBmp->add_pattern("*.tiff");
    fltBmp->set_name("TIFF (*.tif, *.tiff)");
    result.push_back(fltBmp);

    return result;
}

void c_SettingsDlg::InitControls(const std::vector<std::string> &jobNames)
{
    auto lJobs = Gtk::manage(new Gtk::Label("Settings for job(s):", Gtk::Align::ALIGN_START, Gtk::Align::ALIGN_CENTER));
    lJobs->show();
    get_content_area()->pack_start(*lJobs, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto *jobsList = Gtk::manage(new Gtk::TextView());
    jobsList->set_cursor_visible(false);
    jobsList->set_editable(false);
    for (size_t i = 0; i < jobNames.size(); i++)
    {
        jobsList->get_buffer()->insert_at_cursor(jobNames[i] + "\n");
    }
    jobsList->show();
    auto jobsListScrWin = Gtk::manage(new Gtk::ScrolledWindow());
    jobsListScrWin->add(*jobsList);
    jobsListScrWin->show();
    get_content_area()->pack_start(*jobsListScrWin, Gtk::PackOptions::PACK_EXPAND_WIDGET, Utils::Const::widgetPaddingInPixels);

    auto *jobsSeparator = Gtk::manage(new Gtk::Separator());
    jobsSeparator->show();
    get_content_area()->pack_start(*jobsSeparator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto lStab = Gtk::manage(new Gtk::Label());
    lStab->set_text("Video stabilization anchors placement:");
    lStab->show();
    m_VideoStbAnchorsMode.append("automatic");
    m_VideoStbAnchorsMode.append("manual");
    m_VideoStbAnchorsMode.set_active(0);
    m_VideoStbAnchorsMode.show();
    get_content_area()->pack_start(*PackIntoHBox({ lStab, &m_VideoStbAnchorsMode }), Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto lRefPt = Gtk::manage(new Gtk::Label());
    lRefPt->set_text("Reference points placement:");
    lRefPt->show();
    m_RefPtPlacementMode.append("automatic");
    m_RefPtPlacementMode.append("manual");
    m_RefPtPlacementMode.set_tooltip_text("If set to manual, a selection dialog will be shown later during processing");
    m_RefPtPlacementMode.set_active(0);
    m_RefPtPlacementMode.signal_changed().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnRefPtMode));
    m_RefPtPlacementMode.show();
    m_RefPtSpacingLabel.set_text(", spacing in pixels:");
    m_RefPtSpacingLabel.show();
    m_RefPtSpacing.set_adjustment(Gtk::Adjustment::create(/*TODO: make the default a constant*/40, 20, 80, 5));
    m_RefPtSpacing.show();
    get_content_area()->pack_start(*PackIntoHBox({ lRefPt, &m_RefPtPlacementMode, &m_RefPtSpacingLabel, &m_RefPtSpacing}),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto lStack = Gtk::manage(new Gtk::Label("Stacking criterion:"));
    lStack->show();

    m_QualityThreshold.set_adjustment(CreatePercentageAdj());
    m_QualityThreshold.show();

    for (auto &criterionStr: stackingCriterionStr)
        m_QualityCriterion.append(criterionStr);

    m_QualityCriterion.set_active(SKRY_quality_criterion::SKRY_MIN_REL_QUALITY);
    m_QualityCriterion.signal_changed().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnStackingThresholdType));
    m_QualityCriterion.show();

    get_content_area()->pack_start(*PackIntoHBox({ lStack, &m_QualityThreshold, &m_QualityCriterion }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);


    auto lOutp = Gtk::manage(new Gtk::Label());
    lOutp->set_text("Save stacked image automatically:");
    lOutp->show();

    for (auto &saveModeStr: outputSaveModeStr)
        m_OutputSaveMode.append(saveModeStr);

    m_OutputSaveMode.signal_changed().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnOutputSaveMode));
    m_OutputSaveMode.set_active((int)Utils::Const::Defaults::saveMode);
    m_OutputSaveMode.show();
    m_DestFolderChooser.set_create_folders(true);
    m_DestFolderChooser.set_action(Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SELECT_FOLDER);
    m_DestFolderChooser.signal_file_set().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnDestPathSet));
    m_DestFolderChooser.set_sensitive(false);
    m_DestFolderChooser.show();
    get_content_area()->pack_start(*PackIntoHBox({ lOutp, &m_OutputSaveMode, &m_DestFolderChooser }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    m_OutpFmtLabel.set_text("Output format:");
    m_OutpFmtLabel.show();
    for (Utils::Vars::OutputFormatDescr_t &outFmt: Utils::Vars::outputFormatDescription)
    {
        m_AutoSaveOutputFormat.append(outFmt.name);
    }
    m_AutoSaveOutputFormat.show();
    get_content_area()->pack_start(*PackIntoHBox({&m_OutpFmtLabel, &m_AutoSaveOutputFormat }),
        Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    m_FlatFieldCheckBtn.set_label("Use flat-field for stacking:");
    m_FlatFieldCheckBtn.set_active(false);
    m_FlatFieldCheckBtn.signal_toggled().connect(sigc::mem_fun(*this, &c_SettingsDlg::OnFlatFieldToggle));
    m_FlatFieldCheckBtn.show();
    for (auto &filter: GetInputImageFilters())
    {
        m_FlatFieldChooser.add_filter(filter);
    }
    m_FlatFieldChooser.set_title("Select flat-field image");
    m_FlatFieldChooser.set_sensitive(false);
    m_FlatFieldChooser.show();
    get_content_area()->pack_start(*PackIntoHBox({ &m_FlatFieldCheckBtn, &m_FlatFieldChooser }),
                                   Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    auto separator = Gtk::manage(new Gtk::Separator());
    separator->show();
    get_content_area()->pack_end(*separator, Gtk::PackOptions::PACK_SHRINK, Utils::Const::widgetPaddingInPixels);

    add_button("OK",  Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
}

void c_SettingsDlg::OnDestPathSet()
{
    m_DestFolderChooser.set_tooltip_text(m_DestFolderChooser.get_filename());
}

void c_SettingsDlg::OnOutputSaveMode()
{
    m_DestFolderChooser.set_sensitive(m_OutputSaveMode.get_active_row_number() == 2);

    bool autoSaveDisabled =
        (m_OutputSaveMode.get_active_row_number() == (int)Utils::Const::OutputSaveMode::NONE);
    m_OutpFmtLabel.set_sensitive(!autoSaveDisabled);
    m_AutoSaveOutputFormat.set_sensitive(!autoSaveDisabled);
}

void c_SettingsDlg::SetDestinationDir(std::string dir)
{
    m_DestFolderChooser.set_filename(dir);
}

std::string c_SettingsDlg::GetDestinationDir() const
{
    return m_DestFolderChooser.get_filename();
}

void c_SettingsDlg::OnRefPtMode()
{
    bool isAuto = (m_RefPtPlacementMode.get_active_row_number() == 0);
    m_RefPtSpacingLabel.set_sensitive(isAuto);
    m_RefPtSpacing.set_sensitive(isAuto);
}

Glib::RefPtr<Gtk::Adjustment> c_SettingsDlg::CreatePercentageAdj()
{
    return Gtk::Adjustment::create(50, 1, 100);
}

void c_SettingsDlg::OnStackingThresholdType()
{
    if (m_QualityCriterion.get_active_row_number() == (int)SKRY_quality_criterion::SKRY_NUMBER_BEST)
        m_QualityThreshold.set_adjustment(Gtk::Adjustment::create(100, 1, 20000, 10, 50));
    else
        m_QualityThreshold.set_adjustment(CreatePercentageAdj());
}

Utils::Const::OutputSaveMode c_SettingsDlg::GetOutputSaveMode() const
{
    return (Utils::Const::OutputSaveMode)m_OutputSaveMode.get_active_row_number();
}

void c_SettingsDlg::SetOutputSaveMode(Utils::Const::OutputSaveMode mode)
{
    m_OutputSaveMode.set_active(mode);
}

c_SettingsDlg::~c_SettingsDlg()
{
    Utils::SavePosSize(*this, Configuration::SettingsDlgPosSize);
}

unsigned c_SettingsDlg::GetRefPointSpacing() const
{
    return (unsigned)m_RefPtSpacing.get_value_as_int();
}

void c_SettingsDlg::SetRefPtSpacing(unsigned sp)
{
    m_RefPtSpacing.set_value(sp);
}

std::string c_SettingsDlg::GetFlatFieldFileName() const
{
    return m_FlatFieldChooser.get_filename();
}

void c_SettingsDlg::SetFlatFieldFileName(std::string flatField)
{
    m_FlatFieldCheckBtn.set_active(!flatField.empty());
    if (!flatField.empty())
        m_FlatFieldChooser.set_filename(flatField);
}

void c_SettingsDlg::OnFlatFieldToggle()
{
    m_FlatFieldChooser.set_sensitive(m_FlatFieldCheckBtn.get_active());
}

bool c_SettingsDlg::GetAnchorsAutomatic() const
{
    return m_VideoStbAnchorsMode.get_active_row_number() == 0;
}

void c_SettingsDlg::SetAnchorsAutomatic(bool automatic)
{
    m_VideoStbAnchorsMode.set_active(automatic ? 0 : 1);
}

enum SKRY_quality_criterion c_SettingsDlg::GetQualityCriterion() const
{
    return (enum SKRY_quality_criterion)m_QualityCriterion.get_active_row_number();
}

void c_SettingsDlg::SetQualityCriterion(enum SKRY_quality_criterion qualCrit, unsigned threshold)
{
    m_QualityCriterion.set_active((int)qualCrit);
    m_QualityThreshold.set_value(threshold);
}

unsigned c_SettingsDlg::GetQualityThreshold() const
{
    return m_QualityThreshold.get_value_as_int();
}

enum SKRY_output_format c_SettingsDlg::GetAutoSaveOutputFormat() const
{
    return Utils::Vars::outputFormatDescription[m_AutoSaveOutputFormat.get_active_row_number()].skryOutpFmt;
}

void c_SettingsDlg::SetAutoSaveOutputFormat(enum SKRY_output_format outpFmt)
{
    size_t index = 0;
    for (auto &descr: Utils::Vars::outputFormatDescription)
    {
        if (descr.skryOutpFmt == outpFmt)
            m_AutoSaveOutputFormat.set_active(index);
        else
            index++;
    }
}

bool c_SettingsDlg::GetRefPointsAutomatic() const
{
    return m_RefPtPlacementMode.get_active_row_number() == 0;
}

void c_SettingsDlg::SetRefPointsAutomatic(bool automatic)
{
    m_RefPtPlacementMode.set_active(automatic ? 0 : 1);
}
