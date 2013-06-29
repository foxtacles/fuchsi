#include <winsock2.h>
#include <wininet.h>
#include <shlwapi.h>

#define MSG_MINTRAYICON         (WM_USER + 1)
#define WND_CLASS_NAME          "fuch.si"

#include "resource.h"

#include <ext/happyhttp.h>
#include <utils/http.h>

using namespace std;

HINSTANCE instance;
HANDLE global_mutex;
HWND wndmain;
HDC hdc, hdcMem;
HBITMAP hBitmap;
BITMAP bitmap;
PAINTSTRUCT ps;

HWND CreateMainWindow();
int RegisterClasses();
int MessageLoop();
void CleanUp();

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

const char* remoteAddr = "fuch.si";
const unsigned int remotePort = 80;

const char* globalHeaders[] =
{
	"Content-type", "application/x-www-form-urlencoded",
	"Accept", "text/plain",
	nullptr
};

void MinimizeToTray(HWND hwnd)
{
	NOTIFYICONDATA nid;

	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = ICON_MAIN;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = MSG_MINTRAYICON;
	nid.hIcon = (HICON) LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(ICON_MAIN), IMAGE_ICON, 16, 16, 0);
	strcpy(nid.szTip, WND_CLASS_NAME);

	Shell_NotifyIcon(NIM_ADD, &nid);

	ShowWindow(hwnd, SW_HIDE);
}

void Maximize(HWND hwnd)
{
	NOTIFYICONDATA nid;

	ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = ICON_MAIN;
	Shell_NotifyIcon(NIM_DELETE, &nid);
	ShowWindow(hwnd, SW_RESTORE);
	SetForegroundWindow(hwnd);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	global_mutex = CreateMutex(nullptr, TRUE, WND_CLASS_NAME);

	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return MessageBox(nullptr, "fuch.si is already running!", "Error", MB_OK | MB_ICONERROR);

	WSADATA data;
	WSAStartup(0x202, &data);

	instance = hInstance;

	RegisterClasses();

	wndmain = CreateMainWindow();

	return MessageLoop();
}

HWND CreateMainWindow()
{
	HWND wnd;
	wnd = CreateWindowEx(0, WND_CLASS_NAME, WND_CLASS_NAME, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, (GetSystemMetrics(SM_CXSCREEN) / 2) - 105, (GetSystemMetrics(SM_CYSCREEN) / 2) - 105, 220, 240, HWND_DESKTOP, nullptr, instance, nullptr);
	ShowWindow(wnd, SW_SHOWNORMAL);
	UpdateWindow(wnd);
	return wnd;
}

int RegisterClasses()
{
	WNDCLASSEX wc;

	wc.hInstance = instance;
	wc.lpszClassName = WND_CLASS_NAME;
	wc.lpfnWndProc = WindowProcedure;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(instance, MAKEINTRESOURCE(ICON_MAIN));
	wc.hIconSm = LoadIcon(instance, MAKEINTRESOURCE(ICON_MAIN));
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszMenuName = nullptr;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;

	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);

	return RegisterClassEx(&wc);
}

int MessageLoop()
{
	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CleanUp();

	return msg.wParam;
}

void CleanUp()
{
	DeleteObject(hBitmap);
	CloseHandle(global_mutex);
	WSACleanup();
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndNextViewer;

	switch (message)
	{
		case WM_CREATE:
			hwndNextViewer = SetClipboardViewer(hwnd);

			hBitmap = LoadBitmap(GetModuleHandle(nullptr), MAKEINTRESOURCE(FOXTACLES));
			GetObject(hBitmap, sizeof(BITMAP), &bitmap);
			break;

		case WM_CHANGECBCHAIN:
			if ((HWND) wParam == hwndNextViewer)
				hwndNextViewer = (HWND) lParam;
			else if (hwndNextViewer != NULL)
				SendMessage(hwndNextViewer, message, wParam, lParam);

			break;

        case WM_DRAWCLIPBOARD:
		{
			static UINT auPriorityList[] = {
				CF_TEXT,
			};

			UINT uFormat = GetPriorityClipboardFormat(auPriorityList, 1);

			if (uFormat == CF_TEXT)
			{
				if (OpenClipboard(hwnd))
				{
					string query = "url=";

					HGLOBAL hglb = GetClipboardData(uFormat);

					char data[INTERNET_MAX_URL_LENGTH];
					const char* original = (const char*) GlobalLock(hglb);
					DWORD size = INTERNET_MAX_URL_LENGTH;

					bool success = PathIsURL(original) && !strstr(original, "fuch.si/") && UrlEscape(original, data, &size, URL_ESCAPE_SEGMENT_ONLY) == S_OK;

					GlobalUnlock(hglb);

					if (success)
					{
						query += data;

						try
						{
							Utils::http_request(remoteAddr, remotePort, globalHeaders, "/", "POST", reinterpret_cast<const unsigned char*>(query.c_str()), query.size(), [](signed int code, const string& result)
							{
								switch (code)
								{
									case happyhttp::OK:
									{
										if (!result.empty())
										{
											EmptyClipboard();

											HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (result.length() + 1) * sizeof(char));
											LPSTR str  =(LPSTR) GlobalLock(hglbCopy);
											memcpy(str, result.c_str(), result.length());
											str[result.length()] = '\0';
											GlobalUnlock(hglbCopy);
											SetClipboardData(CF_TEXT, hglbCopy);
										}
										break;
									}

									default:
										break;
								}
							});
						}
						catch (happyhttp::Wobbly& e)
						{
							MessageBox(NULL,e.what(),e.what(),MB_OK);
						}
					}

					CloseClipboard();
				}
			}

            SendMessage(hwndNextViewer, message, wParam, lParam);
            break;
		}

		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			hdcMem = CreateCompatibleDC(hdc);

			SelectObject(hdcMem, hBitmap);
			BitBlt(hdc, 5, 5, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);

			DeleteDC(hdcMem);
			EndPaint(hwnd, &ps);
			break;

		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
				MinimizeToTray(hwnd);
			break;

		case MSG_MINTRAYICON:
			if (wParam == ICON_MAIN && lParam == WM_LBUTTONUP)
				Maximize(hwnd);
			break;

		case WM_DESTROY:
			ChangeClipboardChain(hwnd, hwndNextViewer);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}
