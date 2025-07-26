#ifndef RME_COLOR_PALETTE_CTRL_H_
#define RME_COLOR_PALETTE_CTRL_H_

#include "main.h"
#include <wx/wx.h>
#include <wx/panel.h>

// Forward declaration
class ColorPaletteCtrl;

// Event for color selection
wxDECLARE_EVENT(wxEVT_COLOR_PALETTE_SELECTION, wxCommandEvent);

class ColorPaletteCtrl : public wxPanel {
public:
    ColorPaletteCtrl(wxWindow* parent, wxWindowID id = wxID_ANY, 
                     const wxPoint& pos = wxDefaultPosition, 
                     const wxSize& size = wxDefaultSize,
                     long style = 0);
    
    virtual ~ColorPaletteCtrl();
    
    // Get/Set selected color index (0-132)
    int GetSelectedColor() const { return m_selectedColor; }
    void SetSelectedColor(int colorIndex);
    
    // Show/Hide numbers on color cells
    bool GetShowNumbers() const { return m_showNumbers; }
    void SetShowNumbers(bool show);
    
    // Get the RGB value for a color index
    static wxColour GetColorFromIndex(int index);
    
    // Get total number of colors
    static int GetColorCount() { return 133; }

private:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnLeftClick(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    
    // Helper methods
    void DrawColorGrid(wxDC& dc);
    void DrawColorCell(wxDC& dc, int colorIndex, int x, int y, int cellSize);
    int GetColorIndexFromPosition(const wxPoint& pos);
    wxRect GetCellRect(int colorIndex);
    void RecalculateLayout();
    
    // Layout parameters
    static const int COLORS_PER_ROW = 19;  // 0-18 per row
    static const int TOTAL_ROWS = 7;       // 133 colors = 7 rows (last row has 9 colors)
    static const int MIN_CELL_SIZE = 16;
    static const int BORDER_SIZE = 1;
    static const int SELECTION_BORDER = 2;
    
    // Member variables
    int m_selectedColor;
    bool m_showNumbers;
    int m_cellSize;
    int m_gridWidth;
    int m_gridHeight;
    
    DECLARE_EVENT_TABLE()
};

#endif // RME_COLOR_PALETTE_CTRL_H_ 