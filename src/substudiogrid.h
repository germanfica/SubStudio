// src/substudiogrid.h
#ifndef SUBSTUDIO_GRID_H
#define SUBSTUDIO_GRID_H

// Evitar macros min/max de Windows que rompen std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <wx/wx.h>
#include <wx/grid.h>
#include <vector>
#include <algorithm>
#include <cmath>

#include "subtitle.h"         // Subtitles, SubtitleEntry
#include "substudio_time.h"   // SubstudioFormatTime, SubstudioParseTime, TimeFormat, TimeParseFlags, kDefaultParseFlags
#include "substudio_textfmt.h"// SubstudioFormatGridText, SubstudioParseGridText

class wxTextCtrl;

// Columnas de la grilla
enum SubstudioCol : int {
    COL_NUM = 0,
    COL_START,
    COL_END,
    COL_CPS,
    COL_TEXT,
    COL_COUNT
};

// Proveedor de atributos (zebra striping, columnas RO, alineaciones)
class SubstudioAttrProvider : public wxGridCellAttrProvider {
public:
    SubstudioAttrProvider();
    wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind) const override;
};

// Tabla wxGridTableBase: presenta y escribe sobre Subtitles (modelo)
class SubstudioGridTable : public wxGridTableBase {
public:
    explicit SubstudioGridTable(Subtitles& model);

    // wxGridTableBase
    int GetNumberRows() override;
    int GetNumberCols() override { return COL_COUNT; }

    wxString GetValue(int row, int col) override;
    void     SetValue(int row, int col, const wxString& value) override;

    wxString GetColLabelValue(int col) override;
    wxString GetRowLabelValue(int) override { return wxEmptyString; } // sin labels laterales
    bool     CanHaveAttributes() override { return true; }

    wxString GetTypeName(int row, int col) override;
    bool     CanGetValueAs(int row, int col, const wxString& typeName) override;
    bool     CanSetValueAs(int row, int col, const wxString& typeName) override;
    double   GetValueAsDouble(int row, int col) override;
    long     GetValueAsLong(int row, int col) override;
    bool     IsEmptyCell(int row, int col) override;

    // Cambios de tamaño (opcional si se usa SyncToModel de la grilla)
    bool     AppendRows(size_t numRows = 1) override;
    bool     DeleteRows(size_t pos = 0, size_t numRows = 1) override;

    // Asegura 1 fila visible al iniciar
    void     EnsureOneRowPresent();

private:
    void     Reindex(); // actualiza line_number = 1..N
    void     RepaintRow(int row);
    static   int ComputeCpsFromTextTime(const wxString& gridText, double start, double end);

private:
    Subtitles* model_ = nullptr; // no owning
};

// Grid principal: configura columnas, editores, apariencia y sincroniza con el modelo
class SubstudioGrid : public wxGrid {
public:
    SubstudioGrid(wxWindow* parent,
        Subtitles& model,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxWANTS_CHARS,
        const wxString& name = "SubstudioGrid");

    void EnsureOneRowPresent() { if (m_table) m_table->EnsureOneRowPresent(); }

    // Integración con editor externo
    void BindExternalEditor(wxTextCtrl* editor);
    int  GetCurrentRow() const { return GetGridCursorRow(); }
    wxString GetTextAt(int row) const {
        return (row >= 0 && row < GetNumberRows()) ? GetCellValue(row, COL_TEXT) : wxString();
    }
    void SetTextAt(int row, const wxString& txt) {
        if (row >= 0 && row < GetNumberRows()) SetCellValue(row, COL_TEXT, txt);
    }

    // Sincroniza la cantidad de filas mostradas con el tamaño del modelo
    void SyncToModel();

private:
    void ConfigureLook();
    void ConfigureColumns();
    void ApplyBrandColors();

    // Eventos
    void OnCellChanged(wxGridEvent& e);
    void OnRowSelected(wxGridEvent& e);
    void OnRangeSelected(wxGridRangeSelectEvent& e);
    void OnEditorTyped(wxCommandEvent& e);

private:
    SubstudioGridTable* m_table = nullptr;

    // Integración con editor externo
    wxTextCtrl* m_externalEditor = nullptr;
    bool        m_syncGuard = false;

    wxDECLARE_EVENT_TABLE();
};

#endif // SUBSTUDIO_GRID_H
