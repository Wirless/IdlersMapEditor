#include "main.h"
#include "color_palette_ctrl.h"
#include <wx/dcbuffer.h>

// Define the custom event
wxDEFINE_EVENT(wxEVT_COLOR_PALETTE_SELECTION, wxCommandEvent);

// Template outfit color lookup table (from graphics.cpp)
static uint32_t TemplateOutfitLookupTable[] = {
    0xFFFFFF, 0xFFD4BF, 0xFFE9BF, 0xFFFFBF, 0xE9FFBF, 0xD4FFBF, 0xBFFFBF, 0xBFFFD4, 0xBFFFE9, 0xBFFFFF,
    0xBFE9FF, 0xBFD4FF, 0xBFBFFF, 0xD4BFFF, 0xE9BFFF, 0xFFBFFF, 0xFFBFE9, 0xFFBFD4, 0xFFBFBF, 0xDADADA,
    0xBF9F8F, 0xBFAF8F, 0xBFBF8F, 0xAFBF8F, 0x9FBF8F, 0x8FBF8F, 0x8FBF9F, 0x8FBFAF, 0x8FBFBF, 0x8FAFBF,
    0x8F9FBF, 0x8F8FBF, 0x9F8FBF, 0xAF8FBF, 0xBF8FBF, 0xBF8FAF, 0xBF8F9F, 0xBF8F8F, 0xB6B6B6, 0xBF7F5F,
    0xBFAF8F, 0xBFBF5F, 0x9FBF5F, 0x7FBF5F, 0x5FBF5F, 0x5FBF7F, 0x5FBF9F, 0x5FBFBF, 0x5F9FBF, 0x5F7FBF,
    0x5F5FBF, 0x7F5FBF, 0x9F5FBF, 0xBF5FBF, 0xBF5F9F, 0xBF5F7F, 0xBF5F5F, 0x919191, 0xBF6A3F, 0xBF943F,
    0xBFBF3F, 0x94BF3F, 0x6ABF3F, 0x3FBF3F, 0x3FBF6A, 0x3FBF94, 0x3FBFBF, 0x3F94BF, 0x3F6ABF, 0x3F3FBF,
    0x6A3FBF, 0x943FBF, 0xBF3FBF, 0xBF3F94, 0xBF3F6A, 0xBF3F3F, 0x6D6D6D, 0xFF5500, 0xFFAA00, 0xFFFF00,
    0xAAFF00, 0x54FF00, 0x00FF00, 0x00FF54, 0x00FFAA, 0x00FFFF, 0x00A9FF, 0x0055FF, 0x0000FF, 0x5500FF,
    0xA900FF, 0xFE00FF, 0xFF00AA, 0xFF0055, 0xFF0000, 0x484848, 0xBF3F00, 0xBF7F00, 0xBFBF00, 0x7FBF00,
    0x3FBF00, 0x00BF00, 0x00BF3F, 0x00BF7F, 0x00BFBF, 0x007FBF, 0x003FBF, 0x0000BF, 0x3F00BF, 0x7F00BF,
    0xBF00BF, 0xBF007F, 0xBF003F, 0xBF0000, 0x242424, 0x7F2A00, 0x7F5500, 0x7F7F00, 0x557F00, 0x2A7F00,
    0x007F00, 0x007F2A, 0x007F55, 0x007F7F, 0x00547F, 0x002A7F, 0x00007F, 0x2A007F, 0x54007F, 0x7F007F,
    0x7F0055, 0x7F002A, 0x7F0000,
};

wxBEGIN_EVENT_TABLE(ColorPaletteCtrl, wxPanel)
    EVT_PAINT(ColorPaletteCtrl::OnPaint)
    EVT_LEFT_DOWN(ColorPaletteCtrl::OnLeftClick)
    EVT_SIZE(ColorPaletteCtrl::OnSize)
wxEND_EVENT_TABLE()

ColorPaletteCtrl::ColorPaletteCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE), 
      m_selectedColor(0), m_showNumbers(true), m_cellSize(MIN_CELL_SIZE), m_gridWidth(0), m_gridHeight(0)
{
    SetBackgroundColour(wxColour(240, 240, 240));
    SetBackgroundStyle(wxBG_STYLE_CUSTOM); // Enable double buffering
    RecalculateLayout();
}

ColorPaletteCtrl::~ColorPaletteCtrl() {
}

void ColorPaletteCtrl::SetSelectedColor(int colorIndex) {
    if (colorIndex >= 0 && colorIndex < GetColorCount()) {
        m_selectedColor = colorIndex;
        Refresh();
        
        // Send selection event
        wxCommandEvent event(wxEVT_COLOR_PALETTE_SELECTION, GetId());
        event.SetEventObject(this);
        event.SetInt(colorIndex);
        GetEventHandler()->ProcessEvent(event);
    }
}

wxColour ColorPaletteCtrl::GetColorFromIndex(int index) {
    if (index < 0 || index >= GetColorCount()) {
        return wxColour(255, 255, 255); // Default to white
    }
    
    uint32_t color = TemplateOutfitLookupTable[index];
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    return wxColour(r, g, b);
}

void ColorPaletteCtrl::SetShowNumbers(bool show) {
    if (m_showNumbers != show) {
        m_showNumbers = show;
        Refresh();
    }
}

void ColorPaletteCtrl::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    DrawColorGrid(dc);
}

void ColorPaletteCtrl::OnLeftClick(wxMouseEvent& event) {
    int colorIndex = GetColorIndexFromPosition(event.GetPosition());
    if (colorIndex >= 0 && colorIndex < GetColorCount()) {
        SetSelectedColor(colorIndex);
    }
}

void ColorPaletteCtrl::OnSize(wxSizeEvent& event) {
    RecalculateLayout();
    Refresh();
    event.Skip();
}

void ColorPaletteCtrl::DrawColorGrid(wxDC& dc) {
    // Clear background
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    
    // Draw each color cell
    for (int colorIndex = 0; colorIndex < GetColorCount(); ++colorIndex) {
        int row = colorIndex / COLORS_PER_ROW;
        int col = colorIndex % COLORS_PER_ROW;
        
        int x = col * (m_cellSize + BORDER_SIZE) + BORDER_SIZE;
        int y = row * (m_cellSize + BORDER_SIZE) + BORDER_SIZE;
        
        DrawColorCell(dc, colorIndex, x, y, m_cellSize);
    }
}

void ColorPaletteCtrl::DrawColorCell(wxDC& dc, int colorIndex, int x, int y, int cellSize) {
    wxColour color = GetColorFromIndex(colorIndex);
    
    // Draw the color rectangle
    dc.SetBrush(wxBrush(color));
    dc.SetPen(wxPen(wxColour(128, 128, 128), 1));
    dc.DrawRectangle(x, y, cellSize, cellSize);
    
    // Draw selection border if this color is selected
    if (colorIndex == m_selectedColor) {
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(255, 0, 0), SELECTION_BORDER));
        dc.DrawRectangle(x - SELECTION_BORDER/2, y - SELECTION_BORDER/2, 
                        cellSize + SELECTION_BORDER, cellSize + SELECTION_BORDER);
    }
    
    // Draw the color index number in the top-left corner (if enabled)
    if (m_showNumbers) {
        dc.SetTextForeground(wxColour(255, 255, 255)); // White text
        dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        
        wxString numberText = wxString::Format("%d", colorIndex);
        wxSize textSize = dc.GetTextExtent(numberText);
        
        // Draw text with black outline for better visibility
        dc.SetTextForeground(wxColour(0, 0, 0)); // Black outline
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx != 0 || dy != 0) {
                    dc.DrawText(numberText, x + 2 + dx, y + 1 + dy);
                }
            }
        }
        
        // Draw white text on top
        dc.SetTextForeground(wxColour(255, 255, 255));
        dc.DrawText(numberText, x + 2, y + 1);
    }
}

int ColorPaletteCtrl::GetColorIndexFromPosition(const wxPoint& pos) {
    int col = (pos.x - BORDER_SIZE) / (m_cellSize + BORDER_SIZE);
    int row = (pos.y - BORDER_SIZE) / (m_cellSize + BORDER_SIZE);
    
    if (col < 0 || col >= COLORS_PER_ROW || row < 0 || row >= TOTAL_ROWS) {
        return -1;
    }
    
    int colorIndex = row * COLORS_PER_ROW + col;
    return (colorIndex < GetColorCount()) ? colorIndex : -1;
}

wxRect ColorPaletteCtrl::GetCellRect(int colorIndex) {
    if (colorIndex < 0 || colorIndex >= GetColorCount()) {
        return wxRect();
    }
    
    int row = colorIndex / COLORS_PER_ROW;
    int col = colorIndex % COLORS_PER_ROW;
    
    int x = col * (m_cellSize + BORDER_SIZE) + BORDER_SIZE;
    int y = row * (m_cellSize + BORDER_SIZE) + BORDER_SIZE;
    
    return wxRect(x, y, m_cellSize, m_cellSize);
}

void ColorPaletteCtrl::RecalculateLayout() {
    wxSize clientSize = GetClientSize();
    
    // Calculate optimal cell size based on available width
    int availableWidth = clientSize.GetWidth() - (COLORS_PER_ROW + 1) * BORDER_SIZE;
    int optimalCellSize = availableWidth / COLORS_PER_ROW;
    
    // Ensure minimum cell size
    m_cellSize = std::max(MIN_CELL_SIZE, optimalCellSize);
    
    // Calculate grid dimensions
    m_gridWidth = COLORS_PER_ROW * (m_cellSize + BORDER_SIZE) + BORDER_SIZE;
    m_gridHeight = TOTAL_ROWS * (m_cellSize + BORDER_SIZE) + BORDER_SIZE;
    
    // Set minimum size for the control
    SetMinSize(wxSize(m_gridWidth, m_gridHeight));
} 