#include <windows.h>
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[]= TEXT("KeyView1");
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

	wndclass.style= CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc= WndProc;
	wndclass.cbClsExtra= 0;
	wndclass.cbWndExtra= 0;
	wndclass.hInstance= hInstance;
	wndclass.hIcon= LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor= LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground= (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszClassName= szAppName;
	wndclass.lpszMenuName= NULL;

	if(!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("WRONG!"), szAppName, MB_ICONERROR);
		return 0;
	}

	hwnd= CreateWindow(szAppName, TEXT("Keyboard Message Viewer 1"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int cxClientMax, cyClientMax, cxClient, cyClient, cxChar, cyChar;
	static int cLinesMax, cLines;
	static PMSG pmsg;
	static RECT rectScroll;
	static TCHAR szTop[]= TEXT("Message Key Char") TEXT("Repeat Scean Ext ALT Prev Tran");
	static TCHAR szUnd[]= TEXT("_______ ___ ____      ")
		TEXT("_____      ____   _____  _____ ____");
	static TCHAR* szFormat[2]= {
		TEXT("%-13s %3d %-15s%c%6u %4d %3s %3s %4s %4s"),
		TEXT("%-13s 0x%04X%1s%c %6u %4d %3s %3s %4s %4s")
	};
	static TCHAR* szYes= TEXT("Yes");
	static TCHAR* szNo= TEXT("No");
	static TCHAR* szDown= TEXT("Down");
	static TCHAR* szUp= TEXT("Up");

	static TCHAR* szMessage[]= {
		TEXT("WM_KEYDOWN"), TEXT("WM_KEYUP"), TEXT("WM_CHAR"),
		TEXT("WM_DEADCHAR"), TEXT("WM_SYSKEYDOWN"), TEXT("WM_SYSKEYUP"),
		TEXT("WM_SYSCHAR"), TEXT("WM_SYSDEADCHAR")
	};
	HDC hdc;
	int i, iType;
	PAINTSTRUCT ps;
	TCHAR szBuffer[128], szKeyName[32];
	TEXTMETRIC tm;

	switch(message)
	{
	case WM_CREATE:
	case WM_DISPLAYCHANGE:	//当改变了显示器的分辨率能获得的消息
		//Get maximum size of client area
		cxClientMax= GetSystemMetrics(SM_CXMAXIMIZED);
		cyClientMax= GetSystemMetrics(SM_CYMAXIMIZED);
		//Get character size for fixed-pitch font
		hdc= GetDC(hwnd);
		SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
		GetTextMetrics(hdc, &tm);
		cxChar= tm.tmAveCharWidth;
		cyChar= tm.tmHeight;

		ReleaseDC(hwnd, hdc);
		//Allocate memory for display lines
		if(pmsg)
			free(pmsg);
		cLinesMax= cyClientMax/cyChar;
		pmsg= malloc(cLinesMax*sizeof(MSG));
		cLines=0;
		//fall through
	case WM_SIZE:
		if(message== WM_SIZE)
		{
			cxClient= LOWORD(lParam);
			cyClient= HIWORD(lParam);
		}
		//Calculate scrolling rectangle
		rectScroll.left= 0;
		rectScroll.right= cxClient;
		rectScroll.top= cyChar;
		rectScroll.bottom= cyChar*(cyClient/cyChar);

		InvalidateRect(hwnd, NULL, TRUE);
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
		//Rearange storage array
		for(i=cLinesMax-1;i>0;i--)
		{
			pmsg[i]= pmsg[i-1];
		}
		//Store new message
		pmsg[0].hwnd= hwnd;
		pmsg[0].message= message;
		pmsg[0].wParam= wParam;
		pmsg[0].lParam= lParam;

		cLines= min(cLines+1, cLinesMax);
		//Scroll up the display
		ScrollWindow(hwnd, 0, -cyChar, &rectScroll, &rectScroll);
		break;
	case WM_PAINT:
		hdc= BeginPaint(hwnd, &ps);
		SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));	//使用等宽系统字体
		SetBkMode(hdc, TRANSPARENT);	//通过将背景设定为透明，使字体与字体可以重叠显示，这里用作字体和下划线重叠显示。
		//SetBkMode(hdc, OPAQUE);
		TextOut(hdc, 0, 0, szTop, lstrlen(szTop));
		TextOut(hdc, 0, 0, szUnd, lstrlen(szUnd));

		for(i=0;i<min(cLines, cyClient/cyChar-1);i++)
		{
			iType= pmsg[i].message==WM_CHAR||
				pmsg[i].message==WM_SYSCHAR||
				pmsg[i].message==WM_DEADCHAR||
				pmsg[i].message==WM_SYSDEADCHAR;

			GetKeyNameText(pmsg[i].lParam, szKeyName, sizeof(szKeyName)/sizeof(TCHAR));

			TextOut(hdc, 0, (cyClient/cyChar-1-i)*cyChar, szBuffer, 
				wsprintf(szBuffer, szFormat[iType], szMessage[pmsg[i].message- WM_KEYFIRST],
				pmsg[i].wParam, (PTSTR)(iType?TEXT(" "):szKeyName),
				(TCHAR)(iType?pmsg[i].wParam:' '),
				LOWORD(pmsg[i].lParam),
				HIWORD(pmsg[i].lParam)&0xFF,
				0x01000000&pmsg[i].lParam?szYes:szNo,
				0x20000000&pmsg[i].lParam?szYes:szNo,
				0x40000000&pmsg[i].lParam?szDown:szUp,
				0x80000000&pmsg[i].lParam?szUp:szDown));
		}
		EndPaint(hwnd, &ps);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//总结
//SetBkMode(hdc, TRANSPARENT);使输出字体的背景为透明，所以以下两行字体可以互相重叠。
//wsprintf函数即将需要的内容写到了szBuffer里面，又返回了大小，就是初次看的时候不太容易理解。