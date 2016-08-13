#include <winsock2.h>
#include <wininet.h>
#include <shlwapi.h>
#include <unordered_map>
#include <string>

#define MSG_MINTRAYICON         (WM_USER + 1)
#define WND_CLASS_NAME          "fuch.si"
#define FOXYT_CHECKBOX           1000

#include "resource.h"

#include <ext/happyhttp.h>
#include <utils/http.h>

HWND checkbox;

using namespace std;

HINSTANCE instance;
HANDLE global_mutex;
HWND wndmain;
HDC hdc, hdcMem;
HBITMAP hBitmap;
BITMAP bitmap;
PAINTSTRUCT ps;

unordered_map<string, string> fuchsi_maps;

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

string str_replace(const string& source, const char* find, const char* replace)
{
	unsigned int find_len = strlen(find);
	unsigned int replace_len = strlen(replace);
	unsigned int pos = 0;

	string dest = source;

	while ((pos = dest.find(find, pos)) != string::npos)
	{
		dest.replace(pos, find_len, replace);
		pos += replace_len;
	}

	return dest;
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
	wnd = CreateWindowEx(0, WND_CLASS_NAME, WND_CLASS_NAME, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, (GetSystemMetrics(SM_CXSCREEN) / 2) - 105, (GetSystemMetrics(SM_CYSCREEN) / 2) - 105, 220, 270, HWND_DESKTOP, nullptr, instance, nullptr);
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
	static unsigned int last_draw = 0;

	switch (message)
	{
		case WM_CREATE:
			hwndNextViewer = SetClipboardViewer(hwnd);

			hBitmap = LoadBitmap(GetModuleHandle(nullptr), MAKEINTRESOURCE(FOXTACLES));
			GetObject(hBitmap, sizeof(BITMAP), &bitmap);

			checkbox = CreateWindowEx(0, "Button", "fox.yt", 0x50010003, 130, 205, 80, 32, hwnd, (HMENU) FOXYT_CHECKBOX, instance, nullptr);

			RegisterHotKey(hwnd, 0, MOD_SHIFT, VK_HOME);
			break;

		case WM_HOTKEY:
			last_draw = GetTickCount();

			if (OpenClipboard(hwnd))
			{
				HGLOBAL hglb = GetClipboardData(CF_TEXT);
				string content = (const char*) GlobalLock(hglb);
				GlobalUnlock(hglb);

				auto result = fuchsi_maps.find(content);
				if (result != fuchsi_maps.end())
				{
					const auto& data = result->second;
					EmptyClipboard();

					HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (data.length() + 1) * sizeof(char));
					LPSTR str = (LPSTR) GlobalLock(hglbCopy);
					memcpy(str, data.c_str(), data.length());
					str[data.length()] = '\0';
					GlobalUnlock(hglbCopy);
					SetClipboardData(CF_TEXT, hglbCopy);
				}

				CloseClipboard();
			}
			break;

		case WM_CHANGECBCHAIN:
			if ((HWND) wParam == hwndNextViewer)
				hwndNextViewer = (HWND) lParam;
			else if (hwndNextViewer != NULL)
				SendMessage(hwndNextViewer, message, wParam, lParam);

			break;

        case WM_DRAWCLIPBOARD:
		{
			if (GetTickCount() - last_draw < 1000)
			{
				SendMessage(hwndNextViewer, message, wParam, lParam);
				break;
			}

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
					string original_url = original;
					DWORD size = INTERNET_MAX_URL_LENGTH;

					bool success = PathIsURL(original) && !strstr(original, "fuch.si/") && !strstr(original, "fox.yt/") && UrlEscape(original, data, &size, URL_ESCAPE_SEGMENT_ONLY) == S_OK;

					GlobalUnlock(hglb);

					if (success)
					{
						last_draw = GetTickCount();
						query += data;

						// not covered by UrlEscape
						query = str_replace(query, "+", "%2B");

						try
						{
							Utils::http_request(remoteAddr, remotePort, globalHeaders, "/", "POST", reinterpret_cast<const unsigned char*>(query.c_str()), query.size(), [&original_url](signed int code, string& result)
							{
								switch (code)
								{
									case happyhttp::OK:
									{
										//Beep(1000,1000);
										if (!result.empty())
										{
											if (SendMessage(checkbox, BM_GETCHECK, 0, 0))
												result = str_replace(result, "fuch.si", "fox.yt");

											EmptyClipboard();

											HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (result.length() + 1) * sizeof(char));
											LPSTR str = (LPSTR) GlobalLock(hglbCopy);
											memcpy(str, result.c_str(), result.length());
											str[result.length()] = '\0';
											GlobalUnlock(hglbCopy);
											SetClipboardData(CF_TEXT, hglbCopy);

											fuchsi_maps[original_url] = result;
											fuchsi_maps[result] = original_url;
										}
										break;
									}

									default:
										break;
								}
							});
						}
						catch (happyhttp::Wobbly& e)
						{}
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
