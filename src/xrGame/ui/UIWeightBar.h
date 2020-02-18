#pragma once

class CUIWeightBar : public CUIWindow
{
    CUIStatic* m_BottomInfo{};
    CUITextWnd* m_Weight{};
    CUITextWnd* m_WeightMax{};
    float m_Weight_end_x{};

public:
    void init_from_xml(CUIXml& uiXml, pcstr path);
    void UpdateData(float weight);
    void UpdateData(CInventoryOwner* pInvOwner);
};
