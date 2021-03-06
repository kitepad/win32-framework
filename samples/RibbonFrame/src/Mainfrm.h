/////////////////////////////
// Mainfrm.h
//

#ifndef MAINFRM_H
#define MAINFRM_H

#include "View.h"


////////////////////////////////////////////////////////
// CMainFrame manages the application's main window.
// The main window is a frame which has a statusbar and
// view window. It uses a ribbon in place of the menubar
// and toolbar.
class CMainFrame : public CRibbonFrame
{
public:
    CMainFrame();
    virtual ~CMainFrame();
    virtual HWND Create(HWND parent = 0);
    CDoc& GetDoc() { return m_view.GetDoc(); }
    void LoadFile(LPCTSTR str);
    void MRUFileOpen(UINT mruIndex);

    LRESULT OnDropFile(WPARAM wparam);
    BOOL OnFileExit();
    BOOL OnFileNew();
    BOOL OnFileOpen();
    BOOL OnFileSave();
    BOOL OnFileSaveAs();
    BOOL OnFilePrint();
    BOOL OnMRUList(const PROPERTYKEY* key, const PROPVARIANT* ppropvarValue);
    BOOL OnPenColor(const PROPVARIANT* ppropvarValue, IUISimplePropertySet* pCmdExProp);
    BOOL SetPenColor(COLORREF clr);

protected:
    virtual STDMETHODIMP Execute(UINT32 cmdID, UI_EXECUTIONVERB verb, const PROPERTYKEY* key, const PROPVARIANT* ppropvarValue, IUISimplePropertySet* pCmdExProp);
    virtual STDMETHODIMP UpdateProperty(UINT32 cmdID, __in REFPROPERTYKEY key, __in_opt  const PROPVARIANT *currentValue, __out PROPVARIANT *newValue);
    BOOL OnCommand(WPARAM wparam, LPARAM lparam);
    virtual void SetupToolBar();
    virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

private:
    CView m_view;
    CString m_pathName;
};


#endif //MAINFRM_H

