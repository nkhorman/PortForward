// portfoward.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "portfoward.h"

#include <iostream>
#include <set>
#include <map>
#include <iterator>
using namespace std;
using std::set;
using std::map;
using std::iterator;
using std::reverse_iterator;
using std::reverse_bidirectional_iterator;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND topWindow, status, redirs;
unsigned int TaskbarResetMsg = 0;

enum {
	WM_APP_1	= WM_APP,
	WM_APP_2,	// tray menu
	WM_APP_3,	// handle command line
};

// *** neal@wanlink.com - 2010/06/04
// Shell Tray API
#include <shellapi.h>
typedef struct _MYNOTIFYICONDATA
{ 
    DWORD cbSize; 
    HWND hWnd; 
    UINT uID; 
    UINT uFlags; 
    UINT uCallbackMessage; 
    HICON hIcon; 
    TCHAR szTip[128];	// Version 5.0 = 128 < 5.0 = 64
    DWORD dwState; //Version 5.0
    DWORD dwStateMask; //Version 5.0
    TCHAR szInfo[256]; //Version 5.0
    union
	{
        UINT  uTimeout; //Version 5.0
        UINT  uVersion; //Version 5.0
    };
    TCHAR szInfoTitle[64]; //Version 5.0
    DWORD dwInfoFlags; //Version 5.0
} MYNOTIFYICONDATA, *PMYNOTIFYICONDATA; 

#define NIM_SETFOCUS	0x00000003
#define NIM_SETVERSION	0x00000004
#define NOTIFYICON_VERSION 3

#define NIF_STATE		0x00000008
#define NIF_INFO		0x00000010

#define NIS_HIDDEN		0x00000001
#define	NIS_SHAREDICON	0x00000002

#define NIIF_INFO		0x00000001
#define NIIF_WARNING	0x00000002
#define NIIF_ERROR		0x00000003

#ifndef NIN_SELECT
#define NIN_SELECT				(WM_USER+0)
#define NINF_KEY				0x1
#define NIN_KEYSELECT			(NIN_SELECT|NINF_KEY)
#define NIN_BALLOONSHOW			(WM_USER+2)
#define NIN_BALLOONHIDE			(WM_USER+3)
#define NIN_BALLOONTIMEOUT		(WM_USER+4)
#define NIN_BALLOONUSERCLICK	(WM_USER+5)
#endif

BOOL SetTrayTip(MYNOTIFYICONDATA *ptnd, char *str, DWORD msg)
{	BOOL	bOk = FALSE;

	if(str != NULL && strlen(str))
	{
		ptnd->szInfo[0]			= 0;
		ptnd->szInfoTitle[0]	= 0;
		ptnd->uFlags			&= ~NIF_INFO;
		ptnd->uFlags			|= NIF_TIP;

		//sprintf(ptnd->szTip,"%s - %s",m_szAppName,str);
		strcpy(ptnd->szTip,str);
		bOk = Shell_NotifyIcon(msg, (NOTIFYICONDATA *)ptnd);
	}

	return(bOk);
}

BOOL SetTrayIcon(MYNOTIFYICONDATA *ptnd, HICON icon, DWORD msg)
{
	ptnd->hIcon		= icon;
	ptnd->uFlags	|= NIF_ICON;

	return(Shell_NotifyIcon (msg, (NOTIFYICONDATA *)ptnd));
}

BOOL SetTrayBalloon(MYNOTIFYICONDATA *ptnd, char *str, char *title, DWORD flags, UINT maxtime, DWORD msg)
{	BOOL	bOk = FALSE;

	if(str != NULL && strlen(str))
	{
		ptnd->dwInfoFlags	= flags;
		ptnd->uTimeout		= maxtime * 1000;
		ptnd->uFlags		|= NIF_INFO|NIF_TIP;
		strcpy(ptnd->szInfoTitle,title);
		strcpy(ptnd->szInfo,str);
		sprintf(ptnd->szTip,"%s - %s",title,str);
		bOk = Shell_NotifyIcon(msg, (NOTIFYICONDATA *)ptnd);
	}

	return(bOk);
}
// Shell Tray API
// *** neal@wanlink.com - 2010/06/04

MYNOTIFYICONDATA	TaskbarNotifyIconData;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
int WINAPI DlgProc_About(HWND, UINT, WPARAM, LPARAM);
int WINAPI DlgProc_Add(HWND, UINT, WPARAM, LPARAM);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD( 2, 2 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return 1;
	}

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PORTFOWARD, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

    INITCOMMONCONTROLSEX init;
    init.dwSize = sizeof(init);
    init.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&init);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PORTFOWARD));

	PostMessage(topWindow,WM_APP_3,0,0);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PORTFOWARD));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PORTFOWARD);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 400, 400, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   topWindow = hWnd;

   status = CreateWindowEx(0, STATUSCLASSNAME, "Stopped.", WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hWnd, NULL, hInstance, NULL);
   ShowWindow(status, nCmdShow);

   RECT rs;
   GetWindowRect(status, &rs);

   RECT r;
   GetClientRect(hWnd, &r);
   redirs = CreateWindowEx(0, WC_LISTVIEW, "Port Redirections", WS_CHILD | LVS_REPORT, 0, 0, r.right - r.left, r.bottom - (rs.bottom - rs.top), hWnd, NULL, hInstance, NULL);
   ListView_SetExtendedListViewStyle(redirs, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
   LVCOLUMN col;
   col.mask = LVCF_TEXT | LVCF_WIDTH;
   col.pszText = "Local Port";
   col.cx = 100;
   ListView_InsertColumn(redirs, 0, &col);
   col.pszText = "Destination";
   col.cx = r.right - r.left - 300;
   ListView_InsertColumn(redirs, 1, &col);
   col.mask |= LVCF_FMT;
   col.fmt = LVCFMT_CENTER;
   col.pszText = "Destination Port";
   col.cx = 100;
   ListView_InsertColumn(redirs, 2, &col);
   col.pszText = "# Connections";
   col.cx = 100;
   ListView_InsertColumn(redirs, 3, &col);
   ShowWindow(redirs, nCmdShow);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

int edit = -1;
bool delete_redir(int n);
void close_connections_for_redir(int idx);
int get_total_connections();
void load_redirFile(char *filename);
void load_redirs();
void save_redirs();

void ShowTrayContextMenu()
{	HMENU menu = CreatePopupMenu();
	POINT	pt;

	AppendMenu(menu,MF_STRING,IDM_EXIT,"E&xit");

	GetCursorPos(&pt);
	SetForegroundWindow(topWindow);

	TrackPopupMenu(menu,TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0,topWindow,NULL);
	DestroyMenu(menu);
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
	        TaskbarResetMsg = RegisterWindowMessage(TEXT("TaskbarCreated"));
			memset(&TaskbarNotifyIconData,0,sizeof(TaskbarNotifyIconData));
			TaskbarNotifyIconData.cbSize				= sizeof(TaskbarNotifyIconData);
			TaskbarNotifyIconData.hWnd					= hWnd;
			TaskbarNotifyIconData.uFlags				= NIF_MESSAGE;
			TaskbarNotifyIconData.uCallbackMessage		= WM_APP_2;
			TaskbarNotifyIconData.uFlags				= NIF_MESSAGE;
			PostMessage(hWnd,TaskbarResetMsg,0,0);
			break;
	case WM_CLOSE:
		Shell_NotifyIcon(NIM_DELETE, (NOTIFYICONDATA *)&TaskbarNotifyIconData);	// kill tray
		wParam = IDM_EXIT;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, (DLGPROC)DlgProc_About);
			break;
		case ID_FILE_LOAD:
			load_redirs();
			break;
		case ID_FILE_SAVE:
			save_redirs();
			break;
		case ID_REDIR_ADD:
			edit = -1;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDBOX), hWnd, (DLGPROC)DlgProc_Add);
			break;
		case ID_REDIR_CLOSECONNECTIONS:
			{
				for (int i = 0; i < ListView_GetItemCount(redirs); i++)
					if (ListView_GetItemState(redirs, i, LVIS_SELECTED))
						close_connections_for_redir(i);
			}
			break;
		case ID_REDIR_DELETE:
			{
				for (int i = 0; i < ListView_GetItemCount(redirs); )
					if (ListView_GetItemState(redirs, i, LVIS_SELECTED))
					{
						delete_redir(i);
					}
					else
						i++;
			}
			break;
		case ID_REDIR_EDIT:
			{
				edit = -1;
				for (int i = 0; i < ListView_GetItemCount(redirs); i++)
					if (ListView_GetItemState(redirs, i, LVIS_SELECTED)) {
						edit = i;
						break;
					}
				if (edit != -1)
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDBOX), hWnd, (DLGPROC)DlgProc_Add);
			}
			break;
		case IDM_EXIT:
			{
				int nconnections = get_total_connections();
				if (nconnections > 0) {
					char buf[1024];
					sprintf(buf,"There are %i active connections being redirected, they will all be disconnected.  Do you really want to quit?", nconnections);
					if (MessageBox(hWnd, buf, "Warning", MB_ICONHAND|MB_YESNO) == IDNO)
						break;
				}
				DestroyWindow(hWnd);
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_NOTIFY:
		{
			LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)lParam;
			if (lpnmlv->hdr.code == LVN_ITEMACTIVATE) {
				edit = lpnmlv->iItem;
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ADDBOX), hWnd, (DLGPROC)DlgProc_Add);
			} else if (lpnmlv->hdr.code == LVN_ITEMCHANGED) {
				EnableMenuItem(GetMenu(hWnd), ID_REDIR_DELETE, ListView_GetSelectedCount(redirs) != 0 ? MF_ENABLED : MF_GRAYED);
				EnableMenuItem(GetMenu(hWnd), ID_REDIR_CLOSECONNECTIONS, ListView_GetSelectedCount(redirs) != 0 ? MF_ENABLED : MF_GRAYED);
				EnableMenuItem(GetMenu(hWnd), ID_REDIR_EDIT, ListView_GetSelectedCount(redirs) == 1 ? MF_ENABLED : MF_GRAYED);
			}
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_SIZE:
		{
			RECT r;
			GetClientRect(status, &r);
			SetWindowPos(status, NULL, 0, HIWORD(lParam)-(r.bottom - r.top), LOWORD(lParam), r.bottom - r.top, SWP_NOZORDER);
			SetWindowPos(redirs, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam)-1 - (r.bottom - r.top), SWP_NOMOVE|SWP_NOZORDER);
			ListView_SetColumnWidth(redirs, 1, r.right - r.left - 300);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		switch(wParam)
		{
			case SC_MINIMIZE:
				ShowWindow(topWindow,SW_HIDE);
				SetWindowLong(topWindow,GWL_EXSTYLE,GetWindowLong(topWindow,GWL_EXSTYLE)|WS_EX_TOOLWINDOW);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
				break;
		}
		break;
	case WM_APP_2:	// tray popup menu
		switch(LOWORD(lParam))
		{
			case WM_LBUTTONDOWN:	// version < 5.0
				if(!IsWindowVisible(topWindow))
				{
					SetWindowLong(topWindow,GWL_EXSTYLE,GetWindowLong(topWindow,GWL_EXSTYLE)&(~WS_EX_TOOLWINDOW));
					ShowWindow(topWindow,SW_SHOWNORMAL);
				}
				else
				{
					ShowWindow(topWindow,SW_HIDE);
					SetWindowLong(topWindow,GWL_EXSTYLE,GetWindowLong(topWindow,GWL_EXSTYLE)|WS_EX_TOOLWINDOW);
				}
				break;
			case WM_RBUTTONDOWN:	// version < 5.0
				ShowTrayContextMenu();
				return FALSE;
				break;
		}
		break;
	case WM_APP_3:
		{	LPSTR pGCL = GetCommandLine();
			int count = 0;

			while(*pGCL && (count < 2 || *pGCL == ' ' || *pGCL == '\t'))
			{
				count += (*pGCL == '"');
				pGCL++;
			}

			if(*pGCL)
				load_redirFile(pGCL); //MessageBox(topWindow,pGCL,"The Command Line",MB_OK);
		}
		break;
	default:
		if(message == TaskbarResetMsg)
		{
			// add an icon to the tray
			SetTrayIcon(&TaskbarNotifyIconData,LoadIcon(hInst,MAKEINTRESOURCE(IDI_PORTFOWARD)),NIM_ADD);
			SetTrayTip(&TaskbarNotifyIconData,"PortForward",NIM_MODIFY);
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool setup_redir(int idx, int lport, const char *dest, int dport);

// Message handler for about box.
// *** neal@wanlink.com - 2010/06/04
int GetTextExtent(HWND hWnd, LPCTSTR str)
{	HDC		dc		= GetDC(hWnd);
	HFONT	font	= (HFONT)SendMessage(hWnd,WM_GETFONT,0,0);
	HFONT	oldfont	= NULL;
	SIZE	size;

	size.cx = 0;
	size.cy = 0;
	if(dc != NULL)
	{
		oldfont = (HFONT)SelectObject(dc,font);
		GetTextExtentPoint32(dc,str,strlen(str),&size);
		if (oldfont != NULL) 
			SelectObject(dc,oldfont);

		ReleaseDC(hWnd,dc);
	}

	return(size.cx);
}

static BOOL bHovering = FALSE;
static BOOL bVisited = FALSE;
static HCURSOR cursor = NULL;
int WINAPI DlgProc_About(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{//	RECT    rect;

    switch (message)
	{
		case WM_INITDIALOG:
			{	char	buf[255];
				HWND hCtrl = GetDlgItem(hWnd,IDC_HYPERLINK);

/*
				// Set dialog title
				sprintf(buf,"%s - About",m_szAppTitle);
				SetWindowText(hWnd,buf);
*/

/*
				// Center the dialog
				GetWindowRect( hWnd, &rect );
				MoveWindow( hWnd, 
					(GetSystemMetrics(SM_CXSCREEN) - (rect.right-rect.left)) / 2,
					(GetSystemMetrics(SM_CYSCREEN) - (rect.bottom-rect.top)) / 2,
					rect.right-rect.left,
					rect.bottom-rect.top,
					FALSE );
*/

				// Make sure the hyperlink notify's parent
				SetClassLong(hCtrl,GCL_STYLE,GetClassLong(hCtrl,GCL_STYLE)|SS_NOTIFY);

				// Make sure the hyperlink is underlined
				LOGFONT	lf;
				if(GetObject((HGDIOBJ)SendMessage(hCtrl,WM_GETFONT,0,0),sizeof(LOGFONT),&lf))
				{
				    lf.lfUnderline = TRUE;
					SendMessage(hCtrl,WM_SETFONT,(WPARAM)CreateFontIndirect(&lf),1);
				}

				// Setup the cursor for the hyperlink
				if(cursor == NULL)
					cursor = LoadCursor(hInst,MAKEINTRESOURCE(IDC_CURSOR1));
				SetClassLong(hCtrl,GCL_HCURSOR,(LONG)cursor);

				// Size the hyperlink window to the extent of the text
				RECT rect;

				GetWindowText(hCtrl,buf,sizeof(buf));
				GetWindowRect(hCtrl,&rect);
				ScreenToClient(hWnd,(LPPOINT)&rect);
				ScreenToClient(hWnd,((LPPOINT)&rect)+1);
				rect.right = rect.left + GetTextExtent(hCtrl,buf);
			    SetWindowPos(hCtrl, hWnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, SWP_NOZORDER);

				bHovering = FALSE;
			}
			return FALSE;
			break;	// case WM_INITDIALOG

		case WM_COMMAND:
			switch ( LOWORD(wParam) )
			{
				case IDCANCEL:
				case IDOK:
					EndDialog( hWnd, 0 );
					break;
				case IDC_HYPERLINK:
				case IDC_HYPERLINK2:
					{
						switch(HIWORD(wParam))
						{
							case STN_CLICKED:
								{	char	buf[256];

									GetWindowText((HWND)lParam,buf,sizeof(buf));
									bVisited = ((int)ShellExecute(NULL, "open", buf, NULL,NULL, SW_SHOW) > HINSTANCE_ERROR);
								}
								break;
						}
					}
					break;
				default:
					return FALSE;
					break;
			}
			return TRUE;
			break;	// case WM_COMAND

		case WM_MOUSEMOVE:
			{	HWND	hCtl = GetDlgItem(hWnd,IDC_HYPERLINK);
				POINT	pt;
				RECT	rect;

				pt.x	= LOWORD(lParam);
				pt.y	= HIWORD(lParam);

				if(hWnd != NULL && hCtl != NULL)
				{
					GetWindowRect(hCtl,&rect);
					ScreenToClient(hWnd,(LPPOINT)&rect);
					ScreenToClient(hWnd,((LPPOINT)&rect)+1);
					if(pt.x >= rect.left &&
						pt.x <= rect.right &&
						pt.y >= rect.top &&
						pt.y <= rect.bottom)
					{
						if(!bHovering)
						{
							bHovering = TRUE;
							RedrawWindow(hCtl,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
						}
					}
					else if(bHovering)
					{
						bHovering = FALSE;
						RedrawWindow(hCtl,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ERASE);
					}
				}
				return(FALSE);
			}
			break;

		case WM_CTLCOLORSTATIC:
			{	int idc = GetDlgCtrlID((HWND)lParam);

				if(idc == IDC_HYPERLINK || idc == IDC_HYPERLINK2)
				{
					if(bHovering)
						SetTextColor((HDC)wParam,RGB(238,0,0));
					else if(bVisited)
						SetTextColor((HDC)wParam,RGB(85,26,139));
					else
						SetTextColor((HDC)wParam,RGB(0,0,238));

					SetBkMode((HDC)wParam,TRANSPARENT);
					return((LRESULT)GetStockObject(NULL_BRUSH));
				}
				else
					return(FALSE);
			}
			break;

		default:
			return FALSE;
			break;
    }	// switch message

    return( TRUE );
}
// *** neal@wanlink.com - 2010/06/04


bool update_redir(int idx, const char *dest, int dport);

// Message handler for add box.
int WINAPI DlgProc_Add(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		if (edit != -1) {
			SetWindowText(hDlg, "Edit Port Redirection");
			char buf[1024];
			ListView_GetItemText(redirs, edit, 0, buf, 1024);
			SetDlgItemText(hDlg, IDC_LPORT, buf);
			EnableWindow(GetDlgItem(hDlg, IDC_LPORT), FALSE);
			ListView_GetItemText(redirs, edit, 1, buf, 1024);
			SetDlgItemText(hDlg, IDC_DEST, buf);
			ListView_GetItemText(redirs, edit, 2, buf, 1024);
			SetDlgItemText(hDlg, IDC_DPORT, buf);
		}
		return (int)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) 
		{
			char buf[1024];
			int lport, dport;
			char dest[1024];

			GetDlgItemText(hDlg, IDC_LPORT, buf, 1024);
			lport = atoi(buf);

			int n;
			if (edit == -1) {
				LVITEM item;
				item.iItem = ListView_GetItemCount(redirs) + 10;
				item.iSubItem = 0;
				item.mask = LVIF_TEXT;
				item.pszText = buf;
				n = ListView_InsertItem(redirs, &item);
			} else {
				n = edit;
				GetDlgItemTextA(hDlg, IDC_DEST, dest, 1024);
				GetDlgItemText(hDlg, IDC_DPORT, buf, 1024);
				dport = atoi(buf);
				if (!update_redir(n, dest, dport))
					return (int)TRUE;
			}

			GetDlgItemText(hDlg, IDC_DEST, buf, 1024);
			GetDlgItemTextA(hDlg, IDC_DEST, dest, 1024);
			ListView_SetItemText(redirs, n, 1, buf);

			GetDlgItemText(hDlg, IDC_DPORT, buf, 1024);
			ListView_SetItemText(redirs, n, 2, buf);
			dport = atoi(buf);

			if (edit != -1) {
				EndDialog(hDlg, LOWORD(wParam));
				return (int)TRUE;
			}

			ListView_SetItemText(redirs, n, 3, "0");

			if (!setup_redir(n, lport, dest, dport)) {
				ListView_DeleteItem(redirs, n);
				return (int)TRUE;
			}

			EndDialog(hDlg, LOWORD(wParam));
			return (int)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (int)TRUE;
		}
		break;
	}
	return (int)FALSE;
}

struct redir_info_t;

typedef struct conn_info_t {
	redir_info_t *info;
	const char *dest_host;
	int dport;
	SOCKET n, d;
	bool started;
	bool stop, stopped;
	bool writer_exited;
	bool reader_stopping;
} conn_info;

typedef struct redir_info_t {
	int idx;
	int lport;
	const char *dest_host;
	unsigned int dest;  // yeah, ipv4 only, sue me
	int dport;
	SOCKET s;
	std::set<conn_info*> connections;
} redir_info;
std::map<int, redir_info*> redir_infos;

DWORD WINAPI accepter(LPVOID lpThreadParameter);
unsigned int dnslookup(const char *dest);

bool bindToPort(SOCKET s, int port)
{
    sockaddr_in sin;
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    if (bind(s, (sockaddr*)&sin, sizeof(sin)) != 0) {
        MessageBox(NULL, "Cannot bind to the specified local port.  Do you already have a redirection on this port?  If not, another process is already bound to this port.", "Error", MB_OK);
        return false;
    }
	return true;
}

bool setup_redir(int idx, int lport, const char *dest, int dport)
{
	unsigned int dest_ip = dnslookup(dest);
	if (dest_ip == 0)
		return false;

	redir_info *info = new redir_info;
	info->idx = idx;
	info->lport = lport;
	info->dest_host = _strdup(dest);
	info->dest = dest_ip;
	info->dport = dport;

	info->s = socket(AF_INET, SOCK_STREAM, 0);

	if (!bindToPort(info->s, info->lport)) {
		delete info;
		return false;
	}

    listen(info->s, 5);
	
	redir_infos[idx] = info;

	DWORD id;
	CreateThread(NULL, 0, accepter, (LPVOID)idx, 0, &id);
	return true;		
}

void updateNumConnections(int idx);

int get_total_connections()
{
	int nconnections = 0;
	for (std::map<int, redir_info*>::iterator it = redir_infos.begin(); it != redir_infos.end(); it++)
		nconnections += it->second->connections.size();
	return nconnections;
}

void load_redirFile(char *filename)
{
    FILE *f = fopen(filename, "r");
	if (f == NULL) {
		MessageBox(topWindow, filename, "Error - Unable to open file", MB_OK);
		return;
	}

	char buf[1024];
	while (fgets(buf, 1024, f))
	{
		int lport, dport;
		char dest[1024];

		memset(dest,0,sizeof(dest));
		if (sscanf(buf, "%i %s %i", &lport, dest, &dport) == 3)
		{
			char buf[1024];

			sprintf(buf, "%i", lport);
			LVITEM item;
			item.iItem = ListView_GetItemCount(redirs) + 10;
			item.iSubItem = 0;
			item.mask = LVIF_TEXT;
			item.pszText = buf;
			int n = ListView_InsertItem(redirs, &item);

			//MultiByteToWideChar(CP_ACP, 0, dest, -1, buf, 1024);
			ListView_SetItemText(redirs, n, 1, dest);

			sprintf(buf, "%i", dport);
			ListView_SetItemText(redirs, n, 2, buf);
			ListView_SetItemText(redirs, n, 3, "0");

			if (!setup_redir(n, lport, dest, dport))
				ListView_DeleteItem(redirs, n);
		}
	}

	fclose(f);}

void load_redirs()
{
	char filename[MAX_PATH];
	OPENFILENAME of;

	memset(&filename,0,sizeof(filename));
	memset(&of,0,sizeof(of));

	of.lStructSize = sizeof(of);
	of.hwndOwner = topWindow;
	of.hInstance = hInst;
	of.lpstrFilter = "PortForward List - *.pfl\0*.pfl\0Text File - *.txt\0*.txt\0All Files - *.*\0*.*\0";
	of.lpstrCustomFilter = NULL;
	of.nFilterIndex = 1;
	of.lpstrFile = filename;
	of.nMaxFile = sizeof(filename);
	of.lpstrTitle = "Load redirections from..";
	of.lpstrDefExt = "pfl";
	if(GetOpenFileName(&of))
		load_redirFile(filename);
}

void save_redirs()
{
	char filename[MAX_PATH];
	OPENFILENAME of;

	memset(&filename,0,sizeof(filename));
	memset(&of,0,sizeof(of));

	of.lStructSize = sizeof(of);
	of.hwndOwner = topWindow;
	of.hInstance = hInst;
	of.lpstrFilter = "PortForward List - *.pfl\0*.pfl\0Text File - *.txt\0*.txt\0";
	of.nFilterIndex = 1;
	of.lpstrFile = filename;
	of.nMaxFile = sizeof(filename);
	of.lpstrTitle = "Save redirections to..";
	of.lpstrDefExt = "pfl";
	//of.Flags = OFN_OVERWRITEPROMPT;
	if (!GetSaveFileName(&of))
		return;

	FILE *f = fopen(filename, "w");

	if (f == NULL) {
		MessageBox(topWindow, "Cannot create save file", "Error", MB_OK);
		return;
	}

	for (std::map<int, redir_info*>::iterator it = redir_infos.begin(); it != redir_infos.end(); it++)
	{
		redir_info *info = it->second;
		
		fprintf(f,"%i %s %i\n", info->lport, info->dest_host, info->dport);
	}
	fclose(f);
}

void close_connections_for_redir(int idx)
{
	redir_info *info = redir_infos[idx];

	// close the ones that are open
	for (std::set<conn_info*>::iterator it = info->connections.begin(); it != info->connections.end(); it++)
	{
		conn_info *ci = *it;

		ci->stop = true;
		while (ci->started && !ci->stopped)
		{
			Sleep(10);
			// pay attention!
			closesocket(ci->d);
			closesocket(ci->n);
		}
		delete ci;
	}

	info->connections.clear();
	updateNumConnections(idx);
}

bool delete_redir(int n)
{
	redir_info *info = redir_infos[n];
	if (info->connections.size()) {
		char buf[1024];
		sprintf(buf, "There are %d connections currently being redirected from %d to %s:%d.\nThese connections will NOT be disconnected (at least not now).\nAre you sure you want to delete this redirection?", 
			info->connections.size(), info->lport, info->dest_host, info->dport);
		if (MessageBoxA(topWindow, buf, "Warning", MB_YESNO) == IDNO)
			return false;

		// dangle these connections
		for (std::set<conn_info*>::iterator it = info->connections.begin(); it != info->connections.end(); it++)
			(*it)->info = NULL;
	}

	// don't accept any more connections
	closesocket(info->s);

	redir_infos.erase(n);

	delete info;

	int count = ListView_GetItemCount(redirs);
	ListView_DeleteItem(redirs, n);
	if (count - 1 > n) {
		for (int i = n + 1; i < count; i++) {
			redir_infos[i - 1] = redir_infos[i];
			redir_infos[i - 1]->idx = i - 1;
		}
		redir_infos.erase(count - 1);
	}

	return true;
}

bool update_redir(int idx, const char *dest, int dport)
{
	redir_info *info = redir_infos[idx];
	info->dport = dport;
	if (strcmp(info->dest_host, dest)) {
		unsigned int dest_ip = dnslookup(dest);
		if (dest_ip == 0)
			return false;
		info->dest = dest_ip;
		info->dest_host = _strdup(dest);
	}
	return true;
}

unsigned int dnslookup(const char *dest)
{
	unsigned int a, b, c, d;
	if (sscanf(dest, "%d.%d.%d.%d", &a, &b, &c, &d) == 4)
		return (d << 24) | (c << 16) | (b << 8) | a;   // network order

	hostent *he = gethostbyname(dest);
	if (he == NULL)
		return 0;
	return *(unsigned int *)he->h_addr_list[0];
}

DWORD WINAPI reader(LPVOID lpThreadParameter);
DWORD WINAPI writer(LPVOID lpThreadParameter);

void updateNumConnections(int idx)
{
	char buf[100];
	sprintf(buf, "%d", redir_infos[idx]->connections.size());
	ListView_SetItemText(redirs, idx, 3, buf);
}

DWORD WINAPI accepter(LPVOID lpThreadParameter)
{
	redir_info *info = redir_infos[(int)lpThreadParameter];

	sockaddr_in sin;
	int ss = sizeof(sin);
	SOCKET n;

    while ((n = accept(info->s, (sockaddr*)&sin, &ss)) != -1)
	{
		char buf[1024];
        sprintf(buf, "received connection from %i.%i.%i.%i on port %i\n"
			, sin.sin_addr.S_un.S_un_b.s_b1, sin.sin_addr.S_un.S_un_b.s_b2, sin.sin_addr.S_un.S_un_b.s_b3, sin.sin_addr.S_un.S_un_b.s_b4, info->lport
			);
		SetWindowTextA(status, buf);

        SOCKET d = socket(AF_INET, SOCK_STREAM, 0);
        sin.sin_family = AF_INET;
        sin.sin_addr.S_un.S_addr = info->dest;
        sin.sin_port = htons(info->dport);
		
        if (connect(d, (sockaddr*)&sin, sizeof(sin)) != 0)
		{
            sprintf(buf, "received a connection but can't connect to %s:%i\n", info->dest_host, info->dport);
			SetWindowTextA(status, buf);
            closesocket(n);
        }
		else
		{
            sprintf(buf, "connection to %s:%i established\n", info->dest_host, info->dport);
			SetWindowTextA(status, buf);
			conn_info *cinfo = new conn_info;
			cinfo->info = info;
			cinfo->dest_host = info->dest_host;
			cinfo->dport = info->dport;
			cinfo->d = d;
			cinfo->n = n;
			cinfo->started = false;
			cinfo->stop = false;
			cinfo->stopped = false;
			cinfo->writer_exited = false;
			cinfo->reader_stopping = false;
			info->connections.insert(cinfo);
            DWORD id;
            CreateThread(NULL, 0, reader, cinfo, 0, &id);
            CreateThread(NULL, 0, writer, cinfo, 0, &id);

			updateNumConnections(info->idx);
        }
    }
    closesocket(info->s);

	return 0;
}

DWORD WINAPI reader(LPVOID lpThreadParameter)
{
	conn_info *ci = (conn_info*)lpThreadParameter;
    char buf[65536];
    int n;

	ci->started = true;

    while (
		(n = recv(ci->n, buf, sizeof(buf), 0)) > 0
		&& !ci->stop
		&& !ci->writer_exited
		)
	{
        send(ci->d, buf, n, 0);
	}

	ci->reader_stopping = true;

    closesocket(ci->n);

	while (!ci->writer_exited)
		Sleep(10);

    closesocket(ci->d);

	ci->stopped = true;

	// told to stop
	if (ci->stop)
		return 0;

	// otherwise, cleanup
	if (ci->info) {
		ci->info->connections.erase(ci);
		updateNumConnections(ci->info->idx);
	}

	sprintf(buf, "connection to %s:%i closed\n", ci->dest_host, ci->dport);
	SetWindowTextA(status, buf);

	delete ci;

	return 0;
}

DWORD WINAPI writer(LPVOID lpThreadParameter)
{
	conn_info *ci = (conn_info*)lpThreadParameter;

	char buf[65536];
    int n;
    while (
		(n = recv(ci->d, buf, sizeof(buf), 0)) > 0
		&& !ci->stop
		&& !ci->reader_stopping
		)
	{
        send(ci->n, buf, n, 0);
    }

	ci->writer_exited = true;

	return 0;
}
