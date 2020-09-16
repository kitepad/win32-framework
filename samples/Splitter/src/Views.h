/////////////////////////////
// Views.h
//

#ifndef VIEWS_H
#define VIEWS_H

///////////////////////////////////////
// CViewSimple manages a simple window.
class CViewSimple : public CWnd
{
public:
    CViewSimple() {}
    virtual ~CViewSimple() {}

protected:
    virtual void OnDraw(CDC& dc);
    virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);
};


//////////////////////////////////////////////
// CViewText manages an edit control.
// It is used as the view window by CDockText.
class CViewText : public CWnd
{
public:
    CViewText() {}
    virtual ~CViewText() {}

protected:
    virtual void OnAttach();
    virtual void PreCreate(CREATESTRUCT& cs);
};


//////////////////////////////////////////////
// CViewText manages a tree view control.
// It is used as the view window by CDockTree.
class CViewTree : public CTreeView
{
public:
    CViewTree();
    virtual ~CViewTree();
    virtual void OnAttach();
    virtual HTREEITEM AddItem(HTREEITEM hParent, LPCTSTR text, int image);

private:
    CImageList m_smallImages;
};

/////////////////////////////////////////////
// CViewList manages a list view control.
// It is used as the view window by CDockList.
class CViewList : public CListView
{
public:
    CViewList();
    virtual ~CViewList();
    virtual int  AddItem(LPCTSTR text, int image);
    virtual void InsertItems();
    virtual void OnAttach();
    virtual void SetColumns();

private:
    CImageList m_smallImages;
};

#endif // VIEWS_H
