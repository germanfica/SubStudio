// src/substudiogrid.h
#ifndef SUBSTUDIO_GRID_H
#define SUBSTUDIO_GRID_H

#include <wx/wx.h>
#include <wx/grid.h>
#include <vector>
#include <algorithm>
#include <cmath>
// ---- Helpers de tiempo (SUBSTUDIO)
////#include "substudio_time.h"
//enum class TimeFormat : int;
//enum class TimeParseFlags : int;

class wxTextCtrl;

// ---- Modelo de datos de una fila de subtítulo (SUBSTUDIO)
struct SubstudioRow {
    int         index = 0;     // 1..N (solo display)
    double      start = 0.0;   // segundos
    double      end = 0.0;     // segundos
    int         cps = 0;       // chars por segundo (calculado)
    wxString    text;          // texto
};

// ---- Columnas
enum SubstudioCol : int {
    COL_NUM = 0,
    COL_START,
    COL_END,
    COL_CPS,
    COL_TEXT,
    COL_COUNT
};

// ---- Proveedor de atributos con zebra striping y celdas RO por columna (SUBSTUDIO)
class SubstudioAttrProvider : public wxGridCellAttrProvider {
public:
    SubstudioAttrProvider();
    wxGridCellAttr* GetAttr(int row, int col, wxGridCellAttr::wxAttrKind kind) const override;
};

// ---- Tabla wxGridTableBase: encapsula filas y recalcula CPS (SUBSTUDIO)
class SubstudioGridTable : public wxGridTableBase {
public:
    SubstudioGridTable();

    // wxGridTableBase
    int GetNumberRows() override { return static_cast<int>(m_rows.size()); }
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

    // Para grid->AppendRows/DeleteRows
    bool     AppendRows(size_t numRows = 1) override;
    bool     DeleteRows(size_t pos = 0, size_t numRows = 1) override;

    // Helpers convenientes
    void     AppendRow(const SubstudioRow& r);
    void     EnsureOneRowPresent(); // si está vacío, agrega una fila 0/0/0/""

    const std::vector<SubstudioRow>& Rows() const { return m_rows; }

private:
    static void ClearRow(SubstudioRow& r);
    void Reindex();
    void RecalcRow(int row);
    static int  ComputeCps(const wxString& text, double start, double end);

private:
    std::vector<SubstudioRow> m_rows;
};

// ---- Grid principal: configura columnas, editores y apariencia estilo SUBSTUDIO
class SubstudioGrid : public wxGrid {
public:
    SubstudioGrid(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxWANTS_CHARS,
        const wxString& name = "SubstudioGrid");

    void EnsureOneRowPresent() { m_table->EnsureOneRowPresent(); }

    // --- API para integrar editor externo ---
    void BindExternalEditor(wxTextCtrl* editor);
    int  GetCurrentRow() const { return GetGridCursorRow(); }
    // Evita ambigüedad del operador ?: devolviendo wxString explícito en ambas ramas.
    wxString GetTextAt(int row) const {
        return (row >= 0 && row < GetNumberRows())
            ? GetCellValue(row, COL_TEXT)
            : wxString(); // en lugar de wxEmptyString para evitar promoción ambigua
    }
    void SetTextAt(int row, const wxString& txt) { if (row >= 0 && row < GetNumberRows()) SetCellValue(row, COL_TEXT, txt); }

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
