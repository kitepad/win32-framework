#include "gdi.h"


namespace Win32xx
{

	///////////////////////////////////////////////
	// Definitions of the CDC class
	//
	CDC::CDC() : m_hDC(0), m_hBitmapOld(0), m_hBrushOld(0), m_hFontOld(0), m_hPenOld(0),
					m_hRgnOld(0), m_IsCopy(FALSE), m_pCopiedFrom(0), m_nCopies(0)
	{
	}

	CDC::CDC(HDC hDC) : m_hDC(0), m_hBitmapOld(0), m_hBrushOld(0), m_hFontOld(0), m_hPenOld(0),
						m_hRgnOld(0), m_IsCopy(FALSE), m_pCopiedFrom(0), m_nCopies(0)
	{
		// This constructor assigns an existing HDC to the CDC
		// The HDC WILL be released or deleted when the CDC object is destroyed
		if (!hDC) throw CWinException(_T("Can't assign a NULL hDC"));

		m_hDC = hDC;

		// Note: this constructor permits a call like this:
		// CDC MyCDC = SomeHDC;
		//  or
		// CDC MyCDC = ::CreateCompatibleDC(SomeHDC);
		//  or
		// CDC MyCDC = ::GetDC(SomeHWND);
	}

	CDC::CDC(const CDC& rhs)	// Copy constructor
	{
		// The copy constructor is called when a temporary copy of the CDC needs to be created.
		// Since we have two (or more) CDC objects looking after the same HDC, we need to
		//  take account of this in the destructor.

		// Note: Were it not for the peculiarities of Dev-C++, the copy constructor would
		//  simply have been disabled.
		m_hBitmapOld = rhs.m_hBitmapOld;
		m_hBrushOld  = rhs.m_hBrushOld;
		m_hDC		 = rhs.m_hDC;
		m_hFontOld	 = rhs.m_hFontOld;
		m_hPenOld    = rhs.m_hPenOld;
		m_hRgnOld    = rhs.m_hRgnOld;
		m_nCopies    = 0;

		// This CDC is a copy, so we won't need to delete GDI resources in the destructor
		m_IsCopy  = TRUE;
		m_pCopiedFrom = (CDC*)&rhs;
		m_pCopiedFrom->m_nCopies++;
	}

	void CDC::operator = (const HDC hDC)
	{
		AttachDC(hDC);
	}

	CDC::~CDC()
	{
		if (m_hDC)
		{
			if (m_IsCopy)
			{
				// This CDC is just a temporary clone, created by the copy constructor
                // so pass members back to the original
				m_pCopiedFrom->m_hPenOld	= m_hPenOld;
				m_pCopiedFrom->m_hBrushOld	= m_hBrushOld;
				m_pCopiedFrom->m_hBitmapOld	= m_hBitmapOld;
				m_pCopiedFrom->m_hFontOld	= m_hFontOld;
				m_pCopiedFrom->m_hRgnOld    = m_hRgnOld;
				m_pCopiedFrom->m_hDC		= m_hDC;
				m_pCopiedFrom->m_nCopies--;
			}
			else
			{
				// Assert that all CDC copies have been destroyed before destroying primary.
				// An assert here indicates a bug in user code! (somehow destroying the primary CDC before its copies)
				assert(m_nCopies == 0);

				// Delete any GDI objects belonging to this CDC
				if (m_hPenOld)    ::DeleteObject(::SelectObject(m_hDC, m_hPenOld));
				if (m_hBrushOld)  ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
				if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
				if (m_hFontOld)	  ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));
				if (m_hRgnOld)    ::DeleteObject(m_hRgnOld);

				// We need to release a Window DC, and delete a memory DC
	#ifndef _WIN32_WCE
				HWND hwnd = ::WindowFromDC(m_hDC);
				if (hwnd) ::ReleaseDC(hwnd, m_hDC);
				else      ::DeleteDC(m_hDC);
	#else
				::DeleteDC(m_hDC);
	#endif
			}
		}
	}

	void CDC::AttachDC(HDC hDC)
	{
		if (m_hDC) throw CWinException(_T("Device Context ALREADY assigned"));
		if (!hDC) throw CWinException(_T("Can't attach a NULL hDC"));

		m_hDC = hDC;
	}

	HDC CDC::DetachDC()
	{
		if (!m_hDC) throw CWinException(_T("No HDC assigned to this CDC"));
		if (m_hPenOld)    ::DeleteObject(::SelectObject(m_hDC, m_hPenOld));
		if (m_hBrushOld)  ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
		if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
		if (m_hFontOld)	  ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));

		HDC hDC = m_hDC;

		m_hDC = NULL;
		return hDC;
	}

	void CDC::DrawBitmap( int x, int y, int cx, int cy, HBITMAP hbmImage, COLORREF clrMask )
	// Draws the specified bitmap to the specified DC using the mask colour provided as the transparent colour
	// Suitable for use with a Window DC or a memory DC
	{
		// Create the Image memory DC
		CDC dcImage = ::CreateCompatibleDC(m_hDC);
		dcImage.AttachBitmap(hbmImage);

		// Create the Mask memory DC
		HBITMAP hbmMask = ::CreateBitmap(cx, cy, 1, 1, NULL);
		CDC dcMask = ::CreateCompatibleDC(m_hDC);
		dcMask.AttachBitmap(hbmMask);
		::SetBkColor(dcImage, clrMask);
		::BitBlt(dcMask, 0, 0, cx, cy, dcImage, 0, 0, SRCCOPY);

		// Mask the image to the DC provided
		::BitBlt(m_hDC, x, y, cx, cy, dcImage, 0, 0, SRCINVERT);
		::BitBlt(m_hDC, x, y, cx, cy, dcMask, 0, 0, SRCAND);
		::BitBlt(m_hDC, x, y, cx, cy, dcImage, 0, 0, SRCINVERT);

		// Detach the bitmap before the dcImage is destroyed
		dcImage.DetachBitmap();
	}

	void CDC::GradientFill( COLORREF Color1, COLORREF Color2, const RECT& rc, BOOL bVertical )
	// A simple but efficient Gradient Filler compatible with all Windows operating systems
	{
		int Width = rc.right - rc.left;
		int Height = rc.bottom - rc.top;

		int r1 = GetRValue(Color1);
		int g1 = GetGValue(Color1);
		int b1 = GetBValue(Color1);

		int r2 = GetRValue(Color2);
		int g2 = GetGValue(Color2);
		int b2 = GetBValue(Color2);

		COLORREF OldBkColor = ::GetBkColor(m_hDC);

		if (bVertical)
		{
			for(int i=0; i < Width; ++i)
			{
				int r = r1 + (i * (r2-r1) / Width);
				int g = g1 + (i * (g2-g1) / Width);
				int b = b1 + (i * (b2-b1) / Width);
				SetBkColor(RGB(r, g, b));
				CRect line( i + rc.left, rc.top, i + 1 + rc.left, rc.top+Height);
				ExtTextOut(0, 0, ETO_OPAQUE, line, NULL, 0, 0);
			}
		}
		else
		{
			for(int i=0; i < Height; ++i)
			{
				int r = r1 + (i * (r2-r1) / Height);
				int g = g1 + (i * (g2-g1) / Height);
				int b = b1 + (i * (b2-b1) / Height);
				SetBkColor(RGB(r, g, b));
				CRect line( rc.left, i + rc.top, rc.left+Width, i + 1 + rc.top);
				ExtTextOut(0, 0, ETO_OPAQUE, line, NULL, 0, 0);
			}
		}

		SetBkColor(OldBkColor);
	}

	void CDC::SolidFill( COLORREF Color, const RECT& rc )
	// Fills a rectangle with a solid color
	{
		COLORREF OldColor = SetBkColor(Color);
		ExtTextOut(0, 0, ETO_OPAQUE, rc, NULL, 0, 0);
		SetBkColor(OldColor);
	}

	// Bitmap functions
	void CDC::AttachBitmap(HBITMAP hBitmap)
	{
		// Use this to attach an existing bitmap.
		// The bitmap will be deleted for you, unless its detached

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!hBitmap) throw CWinException(_T("Can't attach a NULL HBITMAP"));

		// Delete any existing bitmap
		if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));

		m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
	}

	void CDC::CreateCompatibleBitmap(HDC hDC, int cx, int cy)
	{
		// Creates a compatible bitmap and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));

		HBITMAP hBitmap = ::CreateCompatibleBitmap(hDC, cx, cy);
		if (!hBitmap) throw CWinException(_T("CreateCompatibleBitmap failed"));

		m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
	}

	void CDC::CreateBitmap(int cx, int cy, UINT Planes, UINT BitsPerPixel, CONST VOID *pvColors)
	{
		// Creates a bitmap and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));

		HBITMAP hBitmap = ::CreateBitmap(cx, cy, Planes, BitsPerPixel, pvColors);
		if (!hBitmap) throw CWinException(_T("CreateBitmap failed"));

		m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
	}

#ifndef _WIN32_WCE
	void CDC::CreateBitmapIndirect(const BITMAP& bm)
	{
		// Creates a bitmap and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));

		HBITMAP hBitmap = ::CreateBitmapIndirect(&bm);
		if (!hBitmap) throw CWinException(_T("CreateBitmap failed"));

		m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
	}

	void CDC::CreateDIBitmap(HDC hdc, const BITMAPINFOHEADER& bmih, DWORD fdwInit, CONST VOID *lpbInit,
										BITMAPINFO& bmi,  UINT fuUsage)
	{
		// Creates a bitmap and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));

		HBITMAP hBitmap = ::CreateDIBitmap(hdc, &bmih, fdwInit, lpbInit, &bmi, fuUsage);
		if (!hBitmap) throw CWinException(_T("CreateDIBitmap failed"));

		m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
	}
#endif

	void CDC::CreateDIBSection(HDC hdc, const BITMAPINFO& bmi, UINT iUsage, VOID **ppvBits,
										HANDLE hSection, DWORD dwOffset)
	{
		// Creates a bitmap and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBitmapOld)::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));

		HBITMAP hBitmap = ::CreateDIBSection(hdc, &bmi, iUsage, ppvBits, hSection, dwOffset);
		if (!hBitmap) throw CWinException(_T("CreateDIBSection failed"));

		m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
	}

	HBITMAP CDC::DetachBitmap()
	{
		// Use this to detach the bitmap from the HDC.
		// You are then responible for deleting the detached bitmap

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!m_hBitmapOld) throw CWinException(_T("No Bitmap to detach"));

		HBITMAP hBitmap = (HBITMAP)::SelectObject(m_hDC, m_hBitmapOld);
		m_hBitmapOld = NULL;
		return hBitmap;
	}

	// Brush functions
	void CDC::AttachBrush(HBRUSH hBrush)
	{
		// Use this to attach an existing brush.
		// The brush will be deleted for you, unless its detached

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!hBrush) throw CWinException(_T("Can't attach a NULL HBRUSH"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}

#ifndef _WIN32_WCE
	void CDC::CreateBrushIndirect( const LOGBRUSH& lb)
	{
		// Creates the brush and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));

		HBRUSH hBrush = ::CreateBrushIndirect(&lb);
		if (!hBrush) throw CWinException(_T("CreateBrusIndirect failed"));

		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}

	void CDC::CreateDIBPatternBrush(HGLOBAL hglbDIBPacked, UINT fuColorSpec)
	{
		// Creates the brush and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));

		HBRUSH hBrush = ::CreateDIBPatternBrush(hglbDIBPacked, fuColorSpec);
		if (!hBrush) throw CWinException(_T("CreateDIBPatternBrush failed"));

		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}

	void CDC::CreateHatchBrush(int fnStyle, COLORREF rgb)
	{
		// Creates the brush and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));

		HBRUSH hBrush = ::CreateHatchBrush(fnStyle, rgb);
		if (!hBrush) throw CWinException(_T("CreateHatchBrush failed"));

		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}
#endif

	void CDC::CreateDIBPatternBrushPt(CONST VOID *lpPackedDIB, UINT iUsage)
	{
		// Creates the brush and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));

		HBRUSH hBrush = ::CreateDIBPatternBrushPt(lpPackedDIB, iUsage);
		if (!hBrush) throw CWinException(_T("CreateDIBPatternPrushPt failed"));

		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}

	void CDC::CreatePatternBrush(HBITMAP hbmp)
	{
		// Creates the brush and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));

		HBRUSH hBrush = ::CreatePatternBrush(hbmp);
		if (!hBrush) throw CWinException(_T("CreatePatternBrush failed"));

		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}

	void CDC::CreateSolidBrush(COLORREF rgb)
	{
		// Creates the brush and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));

		HBRUSH hBrush = ::CreateSolidBrush(rgb);
		if (!hBrush) throw CWinException(_T("CreateSolidBrush failed"));

		m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
	}

	HBRUSH CDC::DetachBrush()
	{
		// Use this to detach the brush from the HDC.
		// You are then responible for deleting the detached brush

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!m_hBrushOld) throw CWinException(_T("No Brush to detach"));

		HBRUSH hBrush = (HBRUSH)::SelectObject(m_hDC, m_hBrushOld);
		m_hBrushOld = NULL;
		return hBrush;
	}

	// Font functions
	void CDC::AttachFont(HFONT hFont)
	{
		// Use this to attach an existing font.
		// The font will be deleted for you, unless its detached

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!hFont) throw CWinException(_T("Can't attach a NULL HFONT"));
		if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));
		m_hFontOld = (HFONT)::SelectObject(m_hDC, hFont);
	}

#ifndef _WIN32_WCE
	void CDC::CreateFont(
					int nHeight,               // height of font
  					int nWidth,                // average character width
  					int nEscapement,           // angle of escapement
  					int nOrientation,          // base-line orientation angle
  					int fnWeight,              // font weight
  					DWORD fdwItalic,           // italic attribute option
  					DWORD fdwUnderline,        // underline attribute option
  					DWORD fdwStrikeOut,        // strikeout attribute option
  					DWORD fdwCharSet,          // character set identifier
  					DWORD fdwOutputPrecision,  // output precision
  					DWORD fdwClipPrecision,    // clipping precision
  					DWORD fdwQuality,          // output quality
  					DWORD fdwPitchAndFamily,   // pitch and family
  					LPCTSTR lpszFace           // typeface name
 					)

	{
		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));

		HFONT hFont = ::CreateFont(nHeight, nWidth, nEscapement, nOrientation, fnWeight,
								fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet,
								fdwOutputPrecision, fdwClipPrecision, fdwQuality,
								fdwPitchAndFamily, lpszFace);

		if (!hFont) throw CWinException(_T("CreateFont failed"));

		m_hFontOld = (HFONT)::SelectObject(m_hDC, hFont);
	}
#endif

	void CDC::CreateFontIndirect( const LOGFONT& lf)
	{
		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));

		HFONT hFont = ::CreateFontIndirect(&lf);
		if (!hFont) throw CWinException(_T("CreateFontIndirect failed"));

		m_hFontOld = (HFONT)::SelectObject(m_hDC, hFont);
	}

	HFONT CDC::DetachFont()
	{
		// Use this to detach the font from the HDC.
		// You are then responible for deleting the detached font

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!m_hFontOld) throw CWinException(_T("No Font to detach"));

		HFONT hFont = (HFONT)::SelectObject(m_hDC, m_hFontOld);
		m_hFontOld = NULL;
		return hFont;
	}

	// Pen functions
	void CDC::AttachPen(HPEN hPen)
	{
		// Use this to attach an existing pen.
		// The pen will be deleted for you, unless its detached

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!hPen) throw CWinException(_T("Can't attach a NULL HPEN"));
		if (m_hPenOld) ::DeleteObject(::SelectObject(m_hDC, m_hPenOld));
		m_hPenOld = (HPEN)::SelectObject(m_hDC, hPen);
	}

	void CDC::CreatePen(int nStyle, int nWidth, COLORREF rgb)
	{
		// Creates the pen and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hPenOld) ::DeleteObject(::SelectObject(m_hDC, m_hPenOld));

		HPEN hPen = ::CreatePen(nStyle, nWidth, rgb);
		if (!hPen) throw CWinException(_T("CreatePen failed"));

		m_hPenOld = (HPEN)::SelectObject(m_hDC, hPen);
	}

	void CDC::CreatePenIndirect( const LOGPEN& lgpn)
	{
		// Creates the pen and selects it into the device context

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hPenOld) ::DeleteObject(::SelectObject(m_hDC, m_hPenOld));

		HPEN hPen = ::CreatePenIndirect(&lgpn);
		if (!hPen) throw CWinException(_T("CreatePenIndirect failed"));

		m_hPenOld = (HPEN)::SelectObject(m_hDC, hPen);
	}

	HPEN CDC::DetachPen()
	{
		// Use this to detach the pen from the HDC.
		// You are then responible for deleting the detached pen

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!m_hPenOld) throw CWinException(_T("No Pen to detach"));

		HPEN hPen = (HPEN)::SelectObject(m_hDC, m_hPenOld);
		m_hPenOld = NULL;
		return hPen;
	}

	// Region functions
	void CDC::AttachClipRegion(HRGN hRegion)
	{
		// Use this to attach an existing region.
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!hRegion) throw CWinException(_T("Can't attach a NULL HRGN"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		::SelectClipRgn(m_hDC, hRegion);
		m_hRgnOld = hRegion;
	}
	void CDC::CreateRectRgn(int left, int top, int right, int bottom)
	{
		// Creates a rectangular region from the rectangle co-ordinates.
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::CreateRectRgn(left, top, right, bottom);
		if (!hRgn) throw CWinException(_T("CreateRectRgn failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}

	void CDC::CreateRectRgnIndirect( const RECT& rc)
	{
		// Creates a rectangular region from the rectangle co-ordinates.
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::CreateRectRgnIndirect(&rc);
		if (!hRgn) throw CWinException(_T("CreateRectRgnIndirect failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}

	void CDC::ExtCreateRegion( const XFORM& Xform, DWORD nCount, const RGNDATA *pRgnData)
	{
		// Creates a region from the specified region data and tranformation data.
		// The region will be deleted for you, unless its detached
		// Notes: The shape of a region cannot be changed while it is attached to a DC
		//        GetRegionData can be used to get a region's data
		//        If the XFROM pointer is NULL, the identity transformation is used.

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::ExtCreateRegion(&Xform, nCount, pRgnData);
		if (!hRgn) throw CWinException(_T("ExtCreateRegion failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}

	HRGN CDC::DetachClipRegion()
	{
		// Use this to detach the region from the HDC.
		// You are then responible for deleting the detached region

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (!m_hRgnOld) throw CWinException(_T("No Region to detach"));

		::SelectClipRgn(m_hDC, NULL);
		HRGN hRgn = m_hRgnOld;
		m_hRgnOld = NULL;
		return hRgn;
	}

#ifndef _WIN32_WCE
		void CDC::CreateEllipticRgn(int left, int top, int right, int bottom)
	{
		// Creates the ellyiptical region from the bounding rectangle co-ordinates
		// and selects it into the device context
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::CreateEllipticRgn(left, top, right, bottom);
		if (!hRgn) throw CWinException(_T("CreateEllipticRgn failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}

	void CDC::CreateEllipticRgnIndirect( const RECT& rc)
	{
		// Creates the ellyiptical region from the bounding rectangle co-ordinates
		// and selects it into the device context
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::CreateEllipticRgnIndirect(&rc);
		if (!hRgn) throw CWinException(_T("CreateEllipticRgnIndirect failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}

	void CDC::CreatePolygonRgn(const POINT* ppt, int cPoints, int fnPolyFillMode)
	{
		// Creates the polygon region from the array of points and selects it into
		// the device context. The polygon is presumed closed
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::CreatePolygonRgn(ppt, cPoints, fnPolyFillMode);
		if (!hRgn) throw CWinException(_T("CreatePolygonRgn failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}

	void CDC::CreatePolyPolygonRgn(const POINT* ppt, const int* pPolyCounts, int nCount, int fnPolyFillMode)
	{
		// Creates the polygon region from a series of polygons.The polygons can overlap.
		// The region will be deleted for you, unless its detached
		// Note: The shape of a region cannot be changed while it is attached to a DC

		if (!m_hDC) throw CWinException(_T("Device Context not assigned"));
		if (m_hRgnOld) ::DeleteObject(m_hRgnOld);

		HRGN hRgn = ::CreatePolyPolygonRgn(ppt, pPolyCounts, nCount, fnPolyFillMode);
		if (!hRgn) throw CWinException(_T("CreatePolyPolygonRgn failed"));

		::SelectClipRgn(m_hDC, hRgn);
		m_hRgnOld = hRgn;
	}
#endif


	// Wrappers for WinAPI functions

	// Initialization
	HDC CDC::CreateCompatibleDC( ) const
	{
		return ::CreateCompatibleDC( m_hDC );
	}
	HDC CDC::CreateDC( LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODE& dvmInit ) const
	{
		return ::CreateDC( lpszDriver, lpszDevice, lpszOutput, &dvmInit );
	}
	int CDC::GetDeviceCaps( int nIndex ) const
	{
		return ::GetDeviceCaps(m_hDC, nIndex);
	}
#ifndef _WIN32_WCE
	CWnd* CDC::WindowFromDC( ) const
	{
		return CWnd::FromHandle( ::WindowFromDC( m_hDC ) );
	}
	HDC CDC::CreateIC( LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, const DEVMODE& dvmInit ) const
	{
		return ::CreateIC( lpszDriver, lpszDevice, lpszOutput, &dvmInit );
	}
#endif

	// Point and Line Drawing Functions
	CPoint CDC::GetCurrentPosition( ) const
	{
		//  returns the current "MoveToEx" position
		CPoint pt;
		::MoveToEx( m_hDC, 0, 0, &pt );
		::MoveToEx( m_hDC, pt.x, pt.y, NULL);
		return pt;
	}
	CPoint CDC::MoveTo( int x, int y ) const
	{
		// Updates the current position to the specified point
		return ::MoveToEx( m_hDC, x, y, NULL );
	}
	CPoint CDC::MoveTo( POINT pt ) const
	{
		// Updates the current position to the specified point
		return ::MoveToEx( m_hDC, pt.x, pt.y, NULL );
	}
	BOOL CDC::LineTo( int x, int y ) const
	{
		// Draws a line from the current position up to, but not including, the specified point
		return ::LineTo( m_hDC, x, y );
	}
	BOOL CDC::LineTo( POINT pt ) const
	{
		// Draws a line from the current position up to, but not including, the specified point
		return ::LineTo( m_hDC, pt.x, pt.y );
	}

#ifndef _WIN32_WCE
	BOOL CDC::Arc( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 ) const
	{
		return ::Arc( m_hDC, x1, y1, x2, y2, x3, y3, x4, y4 );
	}
	BOOL CDC::Arc( RECT& rc, POINT ptStart, POINT ptEnd ) const
	{
		// Draws an elliptical arc
		return ::Arc( m_hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y );
	}
	BOOL CDC::ArcTo( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 ) const
	{
		// Draws an elliptical arc
		return ::ArcTo( m_hDC, x1, y1, x2, y2, x3, y3, x4, y4) ;
	}
	BOOL CDC::ArcTo( RECT& rc, POINT ptStart, POINT ptEnd ) const
	{
		// Draws an elliptical arc
		return ::ArcTo(  m_hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y );
	}
	BOOL CDC::AngleArc( int x, int y, int nRadius, float fStartAngle, float fSweepAngle ) const
	{
		// Draws a line segment and an arc
		return ::AngleArc( m_hDC, x, y, nRadius, fStartAngle, fSweepAngle);
	}
	int CDC::GetArcDirection( ) const
	{
		// Retrieves the current arc direction ( AD_COUNTERCLOCKWISE or AD_CLOCKWISE )
		return ::GetArcDirection( m_hDC );
	}
	int CDC::SetArcDirection( int nArcDirection ) const
	{
		// Sets the current arc direction ( AD_COUNTERCLOCKWISE or AD_CLOCKWISE )
		return ::SetArcDirection( m_hDC, nArcDirection );
	}
	BOOL CDC::PolyDraw( const POINT* lpPoints, const BYTE* lpTypes, int nCount ) const
	{
		// Draws a set of line segments and Bzier curves
		return ::PolyDraw( m_hDC, lpPoints, lpTypes, nCount );
	}
	BOOL CDC::Polyline( LPPOINT lpPoints, int nCount ) const
	{
		// Draws a series of line segments by connecting the points in the specified array
		return ::Polyline( m_hDC, lpPoints, nCount );
	}
	BOOL CDC::PolyPolyline( const POINT* lpPoints, const DWORD* lpPolyPoints, int nCount ) const
	{
		// Draws multiple series of connected line segments
		return ::PolyPolyline( m_hDC, lpPoints, lpPolyPoints, nCount );
	}
	BOOL CDC::PolylineTo( const POINT* lpPoints, int nCount ) const
	{
		// Draws one or more straight lines
		return ::PolylineTo( m_hDC, lpPoints, nCount );
	}
	BOOL CDC::PolyBezier( const POINT* lpPoints, int nCount ) const
	{
		// Draws one or more Bzier curves
		return ::PolyBezier( m_hDC, lpPoints, nCount );
	}
	BOOL CDC::PolyBezierTo( const POINT* lpPoints, int nCount ) const
	{
		// Draws one or more Bzier curves
		return ::PolyBezierTo(m_hDC, lpPoints, nCount );
	}
	COLORREF CDC::GetPixel( int x, int y ) const
	{
		// Retrieves the red, green, blue (RGB) color value of the pixel at the specified coordinates
		return ::GetPixel( m_hDC, x, y );
	}
	COLORREF CDC::GetPixel( POINT pt ) const
	{
		// Retrieves the red, green, blue (RGB) color value of the pixel at the specified coordinates
		return ::GetPixel( m_hDC, pt.x, pt.y );
	}
	COLORREF CDC::SetPixel( int x, int y, COLORREF crColor ) const
	{
		// Sets the pixel at the specified coordinates to the specified color
		return ::SetPixel( m_hDC, x, y, crColor );
	}
	COLORREF CDC::SetPixel( POINT pt, COLORREF crColor ) const
	{
		// Sets the pixel at the specified coordinates to the specified color
		return ::SetPixel( m_hDC, pt.x, pt.y, crColor );
	}
	BOOL CDC::SetPixelV( int x, int y, COLORREF crColor ) const
	{
		// Sets the pixel at the specified coordinates to the closest approximation of the specified color
		return ::SetPixelV( m_hDC, x, y, crColor );
	}
	BOOL CDC::SetPixelV( POINT pt, COLORREF crColor ) const
	{
		// Sets the pixel at the specified coordinates to the closest approximation of the specified color
		return ::SetPixelV( m_hDC, pt.x, pt.y, crColor );
	}
#endif

	// Shape Drawing Functions
	void CDC::DrawFocusRect( const RECT& rc ) const
	{
		// draws a rectangle in the style used to indicate that the rectangle has the focus
		::DrawFocusRect( m_hDC, &rc );
	}
	BOOL CDC::Ellipse( int x1, int y1, int x2, int y2 ) const
	{
		// Draws an ellipse. The center of the ellipse is the center of the specified bounding rectangle.
		return ::Ellipse( m_hDC, x1, y1, x2, y2 );
	}
	BOOL CDC::Ellipse( const RECT& rc ) const
	{
		// Draws an ellipse. The center of the ellipse is the center of the specified bounding rectangle.
		return ::Ellipse( m_hDC, rc.left, rc.top, rc.right, rc.bottom );
	}
	BOOL CDC::Polygon( LPPOINT lpPoints, int nCount ) const
	{
		// Draws a polygon consisting of two or more vertices connected by straight lines
		return ::Polygon( m_hDC, lpPoints, nCount);
	}
	BOOL CDC::Rectangle( int x1, int y1, int x2, int y2 ) const
	{
		// Draws a rectangle. The rectangle is outlined by using the current pen and filled by using the current brush.
		return ::Rectangle( m_hDC, x1, y1, x2, y2 );
	}
	BOOL CDC::Rectangle( const RECT& rc) const
	{
		// Draws a rectangle. The rectangle is outlined by using the current pen and filled by using the current brush.
		return ::Rectangle( m_hDC, rc.left, rc.top, rc.right, rc.bottom );
	}
	BOOL CDC::RoundRect( int x1, int y1, int x2, int y2, int nWidth, int nHeight ) const
	{
		// Draws a rectangle with rounded corners
		return ::RoundRect( m_hDC, x1, y1, x2, y2, nWidth, nHeight );
	}
	BOOL CDC::RoundRect( const RECT& rc, int nWidth, int nHeight ) const
	{
		// Draws a rectangle with rounded corners
		return ::RoundRect(m_hDC, rc.left, rc.top, rc.right, rc.bottom, nWidth, nHeight );
	}

#ifndef _WIN32_WCE
	BOOL CDC::Chord( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 ) const
	{
		// Draws a chord (a region bounded by the intersection of an ellipse and a line segment, called a secant)
		return ::Chord( m_hDC, x1, y1, x2, y2, x3, y3, x4, y4 );
	}
	BOOL CDC::Chord( const RECT& rc, POINT ptStart, POINT ptEnd ) const
	{
		// Draws a chord (a region bounded by the intersection of an ellipse and a line segment, called a secant)
		return ::Chord( m_hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y );
	}
	BOOL CDC::Pie( int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4 ) const
	{
		// Draws a pie-shaped wedge bounded by the intersection of an ellipse and two radials.
		return ::Pie( m_hDC, x1, y1, x2, y2, x3, y3, x4, y4 );
	}
	BOOL CDC::Pie( const RECT& rc, POINT ptStart, POINT ptEnd ) const
	{
		// Draws a pie-shaped wedge bounded by the intersection of an ellipse and two radials.
		return ::Pie( m_hDC, rc.left, rc.top, rc.right, rc.bottom,
			ptStart.x, ptStart.y, ptEnd.x, ptEnd.y );
	}
	BOOL CDC::PolyPolygon( LPPOINT lpPoints, LPINT lpPolyCounts, int nCount ) const
	{
		// Draws a series of closed polygons
		return ::PolyPolygon( m_hDC, lpPoints, lpPolyCounts, nCount );
	}
#endif

	// Fill and 3D Drawing functions
	BOOL CDC::FillRect( const RECT& rc, HBRUSH hbr ) const
	{
		// Fills a rectangle by using the specified brush
		return (BOOL)::FillRect( m_hDC, &rc, hbr );
	}
	BOOL CDC::InvertRect( const RECT& rc ) const
	{
		// Inverts a rectangle in a window by performing a logical NOT operation on the color values for each pixel in the rectangle's interior
		return ::InvertRect( m_hDC, &rc );
	}
	BOOL CDC::DrawIconEx( int xLeft, int yTop, HICON hIcon, int cxWidth, int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags ) const
	{
		// draws an icon or cursor, performing the specified raster operations, and stretching or compressing the icon or cursor as specified.
		return ::DrawIconEx( m_hDC, xLeft, yTop, hIcon, cxWidth, cyWidth, istepIfAniCur, hbrFlickerFreeDraw, diFlags );
	}
	BOOL CDC::DrawEdge( const RECT& rc, UINT nEdge, UINT nFlags ) const
	{
		// Draws one or more edges of rectangle
		return ::DrawEdge( m_hDC, (LPRECT)&rc, nEdge, nFlags );
	}
	BOOL CDC::DrawFrameControl( const RECT& rc, UINT nType, UINT nState ) const
	{
		// Draws a frame control of the specified type and style
		return ::DrawFrameControl( m_hDC, (LPRECT)&rc, nType, nState );
	}
	BOOL CDC::FillRgn( HRGN hrgn, HBRUSH hbr ) const
	{
		// Fills a region by using the specified brush
		return ::FillRgn( m_hDC, hrgn, hbr );
	}

#ifndef _WIN32_WCE
	BOOL CDC::DrawIcon( int x, int y, HICON hIcon ) const
	{
		// Draws an icon or cursor
		return ::DrawIcon( m_hDC, x, y, hIcon );
	}
	BOOL CDC::DrawIcon( POINT pt, HICON hIcon ) const
	{
		// Draws an icon or cursor
		return ::DrawIcon( m_hDC, pt.x, pt.y, hIcon );
	}
	BOOL CDC::FrameRect( const RECT& rc, HBRUSH hbr ) const
	{
		// Draws a border around the specified rectangle by using the specified brush
		return (BOOL)::FrameRect( m_hDC, &rc, hbr );
	}
	BOOL CDC::PaintRgn( HRGN hrgn ) const
	{
		// Paints the specified region by using the brush currently selected into the device context
		return ::PaintRgn( m_hDC, hrgn);
	}
#endif

	// Bitmap Functions
	int CDC::StretchDIBits( int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth,
		           int nSrcHeight, CONST VOID *lpBits, BITMAPINFO& bi, UINT iUsage, DWORD dwRop ) const
	{
		// Copies the color data for a rectangle of pixels in a DIB to the specified destination rectangle
		return ::StretchDIBits( m_hDC, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc, nSrcWidth, nSrcHeight, lpBits, &bi, iUsage, dwRop );
	}

	BOOL CDC::PatBlt( int x, int y, int nWidth, int nHeight, DWORD dwRop ) const
	{
		// Paints the specified rectangle using the brush that is currently selected into the device context
		return ::PatBlt( m_hDC, x, y, nWidth, nHeight, dwRop );
	}
	BOOL CDC::BitBlt( int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, DWORD dwRop ) const
	{
		// Performs a bit-block transfer of the color data corresponding to a rectangle of pixels from the specified source device context into a destination device context
		return ::BitBlt( m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, dwRop );
	}
	BOOL CDC::StretchBlt( int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop ) const
	{
		// Copies a bitmap from a source rectangle into a destination rectangle, stretching or compressing the bitmap to fit the dimensions of the destination rectangle, if necessary
		return ::StretchBlt( m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, nSrcWidth, nSrcHeight, dwRop );
	}

#ifndef _WIN32_WCE
	int CDC::GetDIBits( HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, LPBITMAPINFO lpbi, UINT uUsage ) const
	{
		// Retrieves the bits of the specified compatible bitmap and copies them into a buffer as a DIB using the specified format
		return ::GetDIBits( m_hDC, hbmp, uStartScan, cScanLines, lpvBits, lpbi, uUsage );
	}
	int CDC::SetDIBits( HBITMAP hbmp, UINT uStartScan, UINT cScanLines, CONST VOID *lpvBits, LPBITMAPINFO lpbi, UINT fuColorUse ) const
	{
		// Sets the pixels in a compatible bitmap (DDB) using the color data found in the specified DIB
		return ::SetDIBits( m_hDC, hbmp, uStartScan, cScanLines, lpvBits, lpbi, fuColorUse );
	}
	int CDC::GetStretchBltMode( ) const
	{
		// Retrieves the current stretching mode
		// Possible modes: BLACKONWHITE, COLORONCOLOR, HALFTONE, STRETCH_ANDSCANS, STRETCH_DELETESCANS, STRETCH_HALFTONE, STRETCH_ORSCANS, WHITEONBLACK
		return ::GetStretchBltMode( m_hDC );
	}
	int CDC::SetStretchBltMode( int iStretchMode ) const
	{
		// Sets the stretching mode
		// Possible modes: BLACKONWHITE, COLORONCOLOR, HALFTONE, STRETCH_ANDSCANS, STRETCH_DELETESCANS, STRETCH_HALFTONE, STRETCH_ORSCANS, WHITEONBLACK
		return ::SetStretchBltMode( m_hDC, iStretchMode );
	}
	BOOL CDC::FloodFill( int x, int y, COLORREF crColor ) const
	{
		// Fills an area of the display surface with the current brush
		return ::FloodFill(m_hDC, x, y, crColor );
	}
	BOOL CDC::ExtFloodFill( int x, int y, COLORREF crColor, UINT nFillType ) const
	{
		// Fills an area of the display surface with the current brush
		// Fill type: FLOODFILLBORDER or FLOODFILLSURFACE
		return ::ExtFloodFill(m_hDC, x, y, crColor, nFillType );
	}
#endif

	// Layout Functions
#if defined(WINVER) && (WINVER >= 0x0500)
	DWORD CDC::GetLayout() const
	{
		// Returns the layout of a device context (LAYOUT_RTL and LAYOUT_BITMAPORIENTATIONPRESERVED)
		return ::GetLayout(m_hDC);
	}
	DWORD CDC::SetLayout(DWORD dwLayout) const
	{
		// Sets the layout of a device context
		return ::SetLayout(m_hDC, dwLayout);
	}
#endif

	// Text Functions
	BOOL CDC::ExtTextOut( int x, int y, UINT nOptions, const RECT& rc, LPCTSTR lpszString, UINT nCount, LPINT lpDxWidths ) const
	{
		// Draws text using the currently selected font, background color, and text color
		return ::ExtTextOut(m_hDC, x, y, nOptions, &rc, lpszString, nCount, lpDxWidths );
	}

	int CDC::DrawText( LPCTSTR lpszString, int nCount, const RECT& rc, UINT nFormat ) const
	{
		// Draws formatted text in the specified rectangle
		return ::DrawText(m_hDC, lpszString, nCount, (LPRECT)&rc, nFormat );
	}
	UINT CDC::GetTextAlign( ) const
	{
		// Retrieves the text-alignment setting
		// Values: TA_BASELINE, TA_BOTTOM, TA_TOP, TA_CENTER, TA_LEFT, TA_RIGHT, TA_RTLREADING, TA_NOUPDATECP, TA_UPDATECP
		return ::GetTextAlign( m_hDC );
	}
	UINT CDC::SetTextAlign( UINT nFlags ) const
	{
		// Sets the text-alignment setting
		// Values: TA_BASELINE, TA_BOTTOM, TA_TOP, TA_CENTER, TA_LEFT, TA_RIGHT, TA_RTLREADING, TA_NOUPDATECP, TA_UPDATECP
		return ::SetTextAlign( m_hDC, nFlags );
	}
	int CDC::GetTextFace( int nCount, LPTSTR lpszFacename ) const
	{
		// Retrieves the typeface name of the font that is selected into the device context
		return ::GetTextFace( m_hDC, nCount, lpszFacename );
	}
	BOOL CDC::GetTextMetrics( TEXTMETRIC& Metrics ) const
	{
		// Fills the specified buffer with the metrics for the currently selected font
		return ::GetTextMetrics( m_hDC, &Metrics );
	}
	COLORREF CDC::GetBkColor( ) const
	{
		// Returns the current background color
		return ::GetBkColor( m_hDC );
	}
	COLORREF CDC::SetBkColor( COLORREF crColor ) const
	{
		// Sets the current background color to the specified color value
		return ::SetBkColor(m_hDC, crColor );
	}
	COLORREF CDC::GetTextColor( ) const
	{
		// Retrieves the current text color
		return ::GetTextColor( m_hDC);
	}
	COLORREF CDC::SetTextColor( COLORREF crColor ) const
	{
		// Sets the current text color
		return ::SetTextColor( m_hDC, crColor );
	}
	int CDC::GetBkMode( ) const
	{
		// returns the current background mix mode (OPAQUE or TRANSPARENT)
		return ::GetBkMode( m_hDC );
	}
	int CDC::SetBkMode( int iBkMode ) const
	{
		// Sets the current background mix mode (OPAQUE or TRANSPARENT)
		return ::SetBkMode( m_hDC, iBkMode);
	}
	CSize CDC::GetTextExtentPoint( LPCTSTR lpszString, int nCount ) const
	{
		// Computes the width and height of the specified string of text
		CSize sz;
		::GetTextExtentPoint(m_hDC, lpszString, nCount, &sz );
		return sz;
	}
#ifndef _WIN32_WCE
	BOOL CDC::TextOut( int x, int y, LPCTSTR lpszString, int nCount ) const
	{
		// Writes a character string at the specified location
		return ::TextOut( m_hDC, x, y, lpszString, nCount );
	}
	int CDC::DrawTextEx( LPTSTR lpszString, int nCount, const RECT& rc, UINT nFormat, const DRAWTEXTPARAMS& DTParams ) const
	{
		// Draws formatted text in the specified rectangle with more formatting options
		return ::DrawTextEx(m_hDC, lpszString, nCount, (LPRECT)&rc, nFormat, (LPDRAWTEXTPARAMS)&DTParams );
	}
	CSize CDC::TabbedTextOut( int x, int y, LPCTSTR lpszString, int nCount, int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin ) const
	{
		// Writes a character string at a specified location, expanding tabs to the values specified in an array of tab-stop positions
		DWORD dwSize = ::TabbedTextOut(m_hDC, x, y, lpszString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin );
		CSize sz(dwSize);
		return sz;
	}
	CSize CDC::GetTabbedTextExtent( LPCTSTR lpszString, int nCount, int nTabPositions, LPINT lpnTabStopPositions ) const
	{
		// Computes the width and height of a character string
		DWORD dwSize = ::GetTabbedTextExtent(m_hDC, lpszString, nCount, nTabPositions, lpnTabStopPositions );
		CSize sz(dwSize);
		return sz;
	}
	BOOL CDC::GrayString( HBRUSH hBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData, int nCount, int x, int y, int nWidth, int nHeight ) const
	{
		// Draws gray text at the specified location
		return ::GrayString(m_hDC, hBrush, lpOutputFunc, lpData, nCount, x, y, nWidth, nHeight );
	}
	int CDC::SetTextJustification( int nBreakExtra, int nBreakCount  ) const
	{
		// Specifies the amount of space the system should add to the break characters in a string of text
		return ::SetTextJustification( m_hDC, nBreakExtra, nBreakCount  );
	}
	int CDC::GetTextCharacterExtra( ) const
	{
		// Retrieves the current intercharacter spacing for the device context
		return ::GetTextCharacterExtra( m_hDC );
	}
	int CDC::SetTextCharacterExtra( int nCharExtra ) const
	{
		// Sets the intercharacter spacing
		return ::SetTextCharacterExtra( m_hDC, nCharExtra );
	}
	CSize CDC::GetTextExtentPoint32( LPCTSTR lpszString, int nCount ) const
	{
		// Computes the width and height of the specified string of text
		CSize sz;
		::GetTextExtentPoint32(m_hDC, lpszString, nCount, &sz );
		return sz;
	}
#endif



	/////////////////////////////////////////////////////////////////
	// Definitions for some global functions in the Win32xx namespace
	//

#ifndef _WIN32_WCE
	void TintBitmap( HBITMAP hbmSource, int cRed, int cGreen, int cBlue )
	// Modifies the colour of the supplied Device Dependant Bitmap, by the colour
	// correction values specified. The correction values can range from -255 to +255.
	// This function gains its speed by accessing the bitmap colour information
	// directly, rather than using GetPixel/SetPixel.
	{
		// Create our LPBITMAPINFO object
		CBitmapInfoPtr pbmi(hbmSource);
		pbmi->bmiHeader.biBitCount = 24;

		// Create the reference DC for GetDIBits to use
		CDC MemDC = CreateCompatibleDC(NULL);

		// Use GetDIBits to create a DIB from our DDB, and extract the colour data
		MemDC.GetDIBits(hbmSource, 0, pbmi->bmiHeader.biHeight, NULL, pbmi, DIB_RGB_COLORS);
		byte* lpvBits = new byte[pbmi->bmiHeader.biSizeImage];
		if (NULL == lpvBits) throw std::bad_alloc();
		MemDC.GetDIBits(hbmSource, 0, pbmi->bmiHeader.biHeight, lpvBits, pbmi, DIB_RGB_COLORS);
		UINT nWidthBytes = pbmi->bmiHeader.biSizeImage/pbmi->bmiHeader.biHeight;

		// Ensure sane colour correction values
		cBlue  = MIN(cBlue, 255);
		cBlue  = MAX(cBlue, -255);
		cRed   = MIN(cRed, 255);
		cRed   = MAX(cRed, -255);
		cGreen = MIN(cGreen, 255);
		cGreen = MAX(cGreen, -255);

		// Pre-calculate the RGB modification values
		int b1 = 256 - cBlue;
		int g1 = 256 - cGreen;
		int r1 = 256 - cRed;

		int b2 = 256 + cBlue;
		int g2 = 256 + cGreen;
		int r2 = 256 + cRed;

		// Modify the colour
		int yOffset = 0;
		int xOffset;
		int Index;
		for (int Row=0; Row < pbmi->bmiHeader.biHeight; Row++)
		{
			xOffset = 0;

			for (int Column=0; Column < pbmi->bmiHeader.biWidth; Column++)
			{
				// Calculate Index
				Index = yOffset + xOffset;

				// Adjust the colour values
				if (cBlue > 0)
					lpvBits[Index]   = (BYTE)(cBlue + (((lpvBits[Index] *b1)) >>8));
				else if (cBlue < 0)
					lpvBits[Index]   = (BYTE)((lpvBits[Index] *b2) >>8);

				if (cGreen > 0)
					lpvBits[Index+1] = (BYTE)(cGreen + (((lpvBits[Index+1] *g1)) >>8));
				else if (cGreen < 0)
					lpvBits[Index+1] = (BYTE)((lpvBits[Index+1] *g2) >>8);

				if (cRed > 0)
					lpvBits[Index+2] = (BYTE)(cRed + (((lpvBits[Index+2] *r1)) >>8));
				else if (cRed < 0)
					lpvBits[Index+2] = (BYTE)((lpvBits[Index+2] *r2) >>8);

				// Increment the horizontal offset
				xOffset += pbmi->bmiHeader.biBitCount >> 3;
			}

			// Increment vertical offset
			yOffset += nWidthBytes;
		}

		// Save the modified colour back into our source DDB
		MemDC.SetDIBits(hbmSource, 0, pbmi->bmiHeader.biHeight, lpvBits, pbmi, DIB_RGB_COLORS);

		// Cleanup
		delete []lpvBits;
	}

	void GrayScaleBitmap( HBITMAP hbmSource )
	{
		// Create our LPBITMAPINFO object
		CBitmapInfoPtr pbmi(hbmSource);

		// Create the reference DC for GetDIBits to use
		CDC MemDC = CreateCompatibleDC(NULL);

		// Use GetDIBits to create a DIB from our DDB, and extract the colour data
		MemDC.GetDIBits(hbmSource, 0, pbmi->bmiHeader.biHeight, NULL, pbmi, DIB_RGB_COLORS);
		byte* lpvBits = new byte[pbmi->bmiHeader.biSizeImage];
		if (NULL == lpvBits) throw std::bad_alloc();
		MemDC.GetDIBits(hbmSource, 0, pbmi->bmiHeader.biHeight, lpvBits, pbmi, DIB_RGB_COLORS);
		UINT nWidthBytes = pbmi->bmiHeader.biSizeImage/pbmi->bmiHeader.biHeight;

		int yOffset = 0;
		int xOffset;
		int Index;

		for (int Row=0; Row < pbmi->bmiHeader.biHeight; Row++)
		{
			xOffset = 0;

			for (int Column=0; Column < pbmi->bmiHeader.biWidth; Column++)
			{
				// Calculate Index
				Index = yOffset + xOffset;

				BYTE byGray = (BYTE) ((lpvBits[Index] + lpvBits[Index+1]*6 + lpvBits[Index+2] *3)/10);
				lpvBits[Index]   = byGray;
				lpvBits[Index+1] = byGray;
				lpvBits[Index+2] = byGray;

				// Increment the horizontal offset
				xOffset += pbmi->bmiHeader.biBitCount >> 3;
			}

			// Increment vertical offset
			yOffset += nWidthBytes;
		}

		// Save the modified colour back into our source DDB
		MemDC.SetDIBits(hbmSource, 0, pbmi->bmiHeader.biHeight, lpvBits, pbmi, DIB_RGB_COLORS);

		// Cleanup
		delete []lpvBits;
	}


	HIMAGELIST CreateDisabledImageList( HIMAGELIST himlNormal )
	// Returns a greyed image list, created from hImageList
	{
		int cx, cy;
		int nCount = ImageList_GetImageCount(himlNormal);
		if (0 == nCount)
			return NULL;

		ImageList_GetIconSize(himlNormal, &cx, &cy);

		// Create the disabled ImageList
		HIMAGELIST himlDisabled = ImageList_Create(cx, cy, ILC_COLOR24 | ILC_MASK, nCount, 0);

		// Process each image in the ImageList
		for (int i = 0 ; i < nCount; ++i)
		{
			CDC DesktopDC = ::GetDC(NULL);
			CDC MemDC = ::CreateCompatibleDC(NULL);
			MemDC.CreateCompatibleBitmap(DesktopDC, cx, cx);
			CRect rc;
			rc.SetRect(0, 0, cx, cx);

			// Set the mask color to grey for the new ImageList
			COLORREF crMask = RGB(200, 199, 200);
			if ( GetDeviceCaps(DesktopDC, BITSPIXEL) < 24)
			{
				HPALETTE hPal = (HPALETTE)GetCurrentObject(DesktopDC, OBJ_PAL);
				UINT Index = GetNearestPaletteIndex(hPal, crMask);
				if (Index != CLR_INVALID) crMask = PALETTEINDEX(Index);
			}

			MemDC.SolidFill(crMask, rc);

			// Draw the image on the memory DC
			ImageList_SetBkColor(himlNormal, crMask);
			ImageList_Draw(himlNormal, i, MemDC, 0, 0, ILD_NORMAL);

			// Convert colored pixels to gray
			for (int x = 0 ; x < cx; ++x)
			{
				for (int y = 0; y < cy; ++y)
				{
					COLORREF clr = ::GetPixel(MemDC, x, y);

					if (clr != crMask)
					{
						BYTE byGray = (BYTE) (95 + (GetRValue(clr) *3 + GetGValue(clr)*6 + GetBValue(clr))/20);
						MemDC.SetPixel(x, y, RGB(byGray, byGray, byGray));
					}

				}
			}

			// Detach the bitmap so we can use it.
			HBITMAP hbm = MemDC.DetachBitmap();
			ImageList_AddMasked(himlDisabled, hbm, crMask);
			::DeleteObject(hbm);
		}

		return himlDisabled;
	}
#endif


} // namespace Win32xx