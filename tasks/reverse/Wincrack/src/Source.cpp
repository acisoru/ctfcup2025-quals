#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include "AntiDebug.h"
#include "MainLogic.h"
#include "background_gif.h"
#include "background_wav.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

using namespace Gdiplus;

#define WINDOW_WIDTH 640
#define WINDOW_HIGHT 480

#define ID_INPUT_EDIT 1001
#define ID_CHECK_BUTTON 1002
#define ID_MUSIC_FILE 1004

HWND g_hMainWnd = nullptr;
HWND g_hInputEdit = nullptr;
HWND g_hCheckButton = nullptr;
HBITMAP g_hBackgroundBitmap = nullptr;
HINSTANCE g_hInst = nullptr;
ULONG_PTR g_gdiplusToken = 0;
std::wstring g_tempDir;

Image* g_pGifImage = nullptr;
int g_currentFrame = 0;
int g_frameCount = 0;
UINT g_animationTimer = 0;
const UINT ANIMATION_TIMER_ID = 1;
GUID g_timeDimensionID = {0};

std::wstring g_debugLogPath;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OnCheckButtonClick();
bool LoadBackgroundImage();
void PlayBackgroundMusic();
bool ExtractResourceToTemp();
std::wstring GetTempDirectory();
void CreateSimpleBackground();
void CreateSimpleWavFile();
void DebugListResources();
void StartGifAnimation();
void StopGifAnimation();
void OnAnimationTimer();
void DebugLog(const std::wstring& message);
//void InitDebugLog();

// ctfcup{0h_y0u_able_to_d3crypt_this_s00_its_n00t_s00_h4rd_Huh?}
void NTAPI func(PVOID DllHandle, DWORD Reason, PVOID Reserved) {
	if (Reason == DLL_PROCESS_ATTACH) {
		typedef HANDLE(WINAPI* pHeapCreate) (
			_In_ DWORD flOptions,
			_In_ SIZE_T dwInitialSize,
			_In_ SIZE_T dwMaximumSize
			);
		typedef HANDLE(WINAPI* pHeapDestroy) (
			_In_ HANDLE hHeap
			);
		typedef HANDLE(WINAPI* pHeapAlloc) (
			_In_ HANDLE hHeap,
			_In_ DWORD dwFlags,
			_In_ SIZE_T dwBytes
			);
		typedef HANDLE(WINAPI* pVirtualProtect) (
			_In_  LPVOID lpAddress,
			_In_  SIZE_T dwSize,
			_In_  DWORD flNewProtect,
			_Out_ PDWORD lpflOldProtect
			);

		DWORD kernel32Hash = 356045008; //HashStringW(L"KERNEL32.DLL"); // 356045008
		DWORD kernelBase = 4152772134; // HashStringW(L"KERNELBASE.dll");
		DWORD ntdll = 817310536;  //HashStringW(L"ntdll.dll");

		DWORD dwHeapCreate = 4010753586; // HashStringA("HeapCreate"); // 4010753586
		DWORD dwHeapAlloc = 663637877; // HashStringA("RtlAllocateHeap");
		DWORD dwVirtualProtect = 2011822024; /// HashStringA("VirtualProtect"); // 2011822024
		DWORD dwHeapDestroy = 10873992; // HashStringA("HeapDestroy"); // 10873992

		pHeapCreate pheapCreate = (pHeapCreate)FindFunctionByHash(kernel32Hash, dwHeapCreate);
		pHeapAlloc heapAlloc = (pHeapAlloc)FindFunctionByHash(ntdll, dwHeapAlloc);
		pVirtualProtect virtualProtect = (pVirtualProtect)FindFunctionByHash(kernelBase, dwVirtualProtect);
		pHeapDestroy heapDestroy = (pHeapDestroy)FindFunctionByHash(kernelBase, dwHeapDestroy);


		HANDLE hHeap = pheapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0x100000, 0x200000);

		uint64_t* seeds = (uint64_t*)heapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024 * 8);
	
		if (AntiDebug::IsDebugged()) {
			for (auto i = 0; i < 1024; ++i)
				seeds[i] = i;
		}

		for (auto i = 0; i < 1024; ++i) {
			seeds[i] += (i * 2);
		}

		uint64_t* consts = InitCryptoConsts(seeds, hHeap);
		uint64_t final[128];

		memcpy(final, consts, 128 * sizeof(uint64_t));
		heapDestroy(hHeap);

		uint8_t* PEB = (uint8_t*)__readgsqword(0x60);
		uint8_t* ptr = (PEB + 0x800);

		memcpy(ptr, final, 128 * sizeof(uint64_t));
		memset(final, 0x0, 128 * 8);

		DWORD old_prot = 0;
		DWORD exe_prot = PAGE_EXECUTE_READWRITE;

		uint64_t ImageBase = *(uint64_t*)(PEB + 0x10);
		uint8_t* code = (uint8_t*)ImageBase;

		code += 0x11590;
		virtualProtect(code, 0x1000, exe_prot, &old_prot);
		code++;
		code[0] = 0x0b;
		code[1] = 0x69;
		code[2] = 0xff;
		code[3] = 0xff;
		virtualProtect(code, 0x1000, old_prot, &exe_prot);
	}
}

#pragma comment (linker, "/INCLUDE:_tls_used")
#pragma comment (linker, "/INCLUDE:tls_callback_func")
#pragma const_seg(".CRT$XLF")
EXTERN_C const PIMAGE_TLS_CALLBACK tls_callback_func = func;
#pragma const_seg()

class PreInitExecutor {
public:
	PreInitExecutor() {	
		AntiDebug::InitializeAllProtections();
		AntiDebug::Defend();
	}
};

PreInitExecutor pei;

wchar_t* valid_string;
wchar_t* invalid_string;

uint64_t flag_checker_addr = *(uint64_t*)&RealFlagChecker;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	g_hInst = hInstance;

	valid_string = new wchar_t[sizeof("Correct flag!") * 2];
	StrCpyW(valid_string, L"Correct flag!");

	invalid_string = new wchar_t[sizeof("Correct flag!") * 2];
	StrCpyW(invalid_string, L"Incorrect flag");

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
	
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);
	
	if (!ExtractResourceToTemp()) {
		return 1;
	}
	
	LoadBackgroundImage();
	
	PlayBackgroundMusic();
	
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"Wincrack";
	wc.hIconSm = LoadIcon(nullptr, nullptr);
	
	if (!RegisterClassEx(&wc)) {
		MessageBox(nullptr, L"Failed to register window class", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	
	g_hMainWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"Wincrack",
		L"Wincrack",
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		WINDOW_WIDTH, WINDOW_HIGHT,
		nullptr, nullptr, hInstance, nullptr
	);
	
	if (!g_hMainWnd) {
		MessageBox(nullptr, L"Failed to create window", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	
	g_hInputEdit = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		L"EDIT",
		L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
		WINDOW_WIDTH / 3 - 20, WINDOW_HIGHT /  3 + 60, 250, 25,
		g_hMainWnd,
		(HMENU)ID_INPUT_EDIT,
		hInstance,
		nullptr
	);
	
	g_hCheckButton = CreateWindowEx(
		0,
		L"BUTTON",
		L"CHECK",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		WINDOW_WIDTH / 3 + 65, WINDOW_HIGHT / 3 + 90, 80, 30,
		g_hMainWnd,
		(HMENU)ID_CHECK_BUTTON,
		hInstance,
		nullptr
	);
	
	SetWindowPos(g_hInputEdit, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetWindowPos(g_hCheckButton, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	
	ShowWindow(g_hMainWnd, nCmdShow);
	UpdateWindow(g_hMainWnd);
	
	if (g_pGifImage && g_frameCount > 0) {
		StartGifAnimation();
	}
	
	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	StopGifAnimation();
	if (g_pGifImage) {
		delete g_pGifImage;
		g_pGifImage = nullptr;
	}
	if (g_hBackgroundBitmap) {
		DeleteObject(g_hBackgroundBitmap);
	}
	GdiplusShutdown(g_gdiplusToken);
	
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			
			static int paintCount = 0;
			paintCount++;
		
			if (g_pGifImage) {
				Graphics graphics(hdc);
				graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
				graphics.SetSmoothingMode(SmoothingModeAntiAlias);
				
				graphics.DrawImage(g_pGifImage, 0, 0, WINDOW_WIDTH, WINDOW_HIGHT);
			}
			
			else if (g_hBackgroundBitmap) {
				HDC hdcMem = CreateCompatibleDC(hdc);
				HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, g_hBackgroundBitmap);
				
				BITMAP bm;
				GetObject(g_hBackgroundBitmap, sizeof(bm), &bm);
				
				StretchBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HIGHT, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
				
				SelectObject(hdcMem, hOldBitmap);
				DeleteDC(hdcMem);
			}
			
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(255, 255, 0));
			HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
			HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
			
			TextOut(hdc, WINDOW_WIDTH / 3 + 30, WINDOW_HIGHT / 3 + 10, L"Enter your key:", 15);
			
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
			
			EndPaint(hwnd, &ps);
			return 0;
		}
		
		case WM_TIMER:
			if (wParam == ANIMATION_TIMER_ID) {
				OnAnimationTimer();
			}
			return 0;
		
		case WM_COMMAND:
			if (LOWORD(wParam) == ID_CHECK_BUTTON) {
				OnCheckButtonClick();
			}
			return 0;
			
		case WM_ERASEBKGND:
			return 1;
			
		case WM_DESTROY:
			StopGifAnimation();
			PostQuitMessage(0);
			return 0;
	}
	
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void OnCheckButtonClick() {
	wchar_t buffer[256];
	GetWindowText(g_hInputEdit, buffer, 256);
	
	std::wstring key(buffer);

	if (key.find(L"WINCRACK_FLAG", 0) != -1) {
		MessageBox(g_hMainWnd, valid_string, L"Success", MB_OK | MB_ICONINFORMATION);
	} else {
		MessageBox(g_hMainWnd, invalid_string, L"Error", MB_OK | MB_ICONERROR);
	}
}

bool LoadBackgroundImage() {
	std::wstring imagePath = g_tempDir + L"background.gif";
	
	
	if (PathFileExists(imagePath.c_str())) {
		
		g_pGifImage = new Image(imagePath.c_str());
		Status loadStatus = g_pGifImage->GetLastStatus();
		
		if (loadStatus == Ok) {
			UINT dimensionCount = g_pGifImage->GetFrameDimensionsCount();
			if (dimensionCount > 0) {
				GUID* dimensionIDs = new GUID[dimensionCount];
				g_pGifImage->GetFrameDimensionsList(dimensionIDs, dimensionCount);
				
				GUID timeDimension = {0x6aedbd6d, 0x3fb5, 0x418a, {0x83, 0xa6, 0x7f, 0x45, 0x22, 0x9d, 0xc8, 0x72}};
				
				for (UINT i = 0; i < dimensionCount; i++) {
					if (memcmp(&dimensionIDs[i], &timeDimension, sizeof(GUID)) == 0) {
						g_timeDimensionID = dimensionIDs[i];
						g_frameCount = g_pGifImage->GetFrameCount(&g_timeDimensionID);
						break;
					}
				}
				
				if (g_frameCount == 0 && dimensionCount > 0) {
					g_timeDimensionID = dimensionIDs[0];
					g_frameCount = g_pGifImage->GetFrameCount(&g_timeDimensionID);
				}
				
				delete[] dimensionIDs;
				
				if (g_frameCount > 0) {
					g_pGifImage->SelectActiveFrame(&g_timeDimensionID, 0);
				}
			}
			return true;
		}
		delete g_pGifImage;
		g_pGifImage = nullptr;
	} else {
	}
	
	CreateSimpleBackground();
	return true;
}

void PlayBackgroundMusic() {
	std::wstring musicPath = g_tempDir + L"background.wav";
	
	if (PathFileExists(musicPath.c_str())) {
		PlaySound(musicPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_LOOP);
	}
}

bool ExtractResourceToTemp() {
	g_tempDir = GetTempDirectory();
	
	CreateDirectory(g_tempDir.c_str(), nullptr);
	
	std::wstring gifPath = g_tempDir + L"background.gif";
	std::ofstream file(gifPath, std::ios::binary);

	if (file.is_open()) {
		DWORD size = sizeof(background_gif_data);
		file.write((char*)background_gif_data, size);
		file.close();
	}
	
	std::wstring wavPath = g_tempDir + L"background.wav";
	std::ofstream file_music(wavPath, std::ios::binary);
	
	if (file_music.is_open()) {
		file_music.write((char*)background_wav_data, sizeof(background_wav_data));
		file_music.close();
	}
	
	return true;
}

std::wstring GetTempDirectory() {
	wchar_t tempPath[MAX_PATH];
	GetTempPath(MAX_PATH, tempPath);
	return std::wstring(tempPath) + L"WincrackTemp\\";
}

void CreateSimpleBackground() {
	HDC hdc = CreateCompatibleDC(nullptr);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HIGHT);
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
	
	for (int y = 0; y < WINDOW_WIDTH; y++) {
		for (int x = 0; x < WINDOW_HIGHT; x++) {
			COLORREF color = RGB(
				(255 * y) / WINDOW_WIDTH,
				(128 * x) / WINDOW_HIGHT,
				128
			);
			SetPixel(hdcMem, x, y, color);
		}
	}
	
	g_hBackgroundBitmap = hBitmap;
	SelectObject(hdcMem, hOldBitmap);
	DeleteDC(hdcMem);
	DeleteDC(hdc);
}

void CreateSimpleWavFile() {
	std::wstring wavPath = g_tempDir + L"background.wav";
	
	struct WAVHeader {
		char riff[4] = {'R', 'I', 'F', 'F'};
		uint32_t fileSize = 36 + 44100 * 2; 
		char wave[4] = {'W', 'A', 'V', 'E'};
		char fmt[4] = {'f', 'm', 't', ' '};
		uint32_t fmtSize = 16;
		uint16_t audioFormat = 1;
		uint16_t numChannels = 1;
		uint32_t sampleRate = 44100;
		uint32_t byteRate = 44100 * 2;
		uint16_t blockAlign = 2;
		uint16_t bitsPerSample = 16;
		char data[4] = {'d', 'a', 't', 'a'};
		uint32_t dataSize = 44100 * 2;
	};
	
	WAVHeader header;
	
	std::ofstream file(wavPath, std::ios::binary);
	if (file.is_open()) {
		file.write(reinterpret_cast<char*>(&header), sizeof(header));
		
		for (int i = 0; i < 44100; i++) {
			int16_t sample = static_cast<int16_t>(32767 * 0.1 * sin(2 * 3.14159 * 440 * i / 44100));
			file.write(reinterpret_cast<char*>(&sample), sizeof(sample));
		}
		
		file.close();
	}
}

void StartGifAnimation() {
	
	if (g_pGifImage && g_frameCount > 0 && g_hMainWnd) {
		UINT size = g_pGifImage->GetPropertyItemSize(PropertyTagFrameDelay);
		UINT delay = 100;
		
		if (size > 0) {
			PropertyItem* propItem = (PropertyItem*)malloc(size);
			if (g_pGifImage->GetPropertyItem(PropertyTagFrameDelay, size, propItem) == Ok) {

				if (propItem->length > 0) {
					delay = *((UINT*)propItem->value);
					delay = delay * 10;
					if (delay < 50) delay = 100;
				}
			}
			free(propItem);
		}
		
		g_animationTimer = SetTimer(g_hMainWnd, ANIMATION_TIMER_ID, delay, nullptr);
		g_currentFrame = 0;
		
	} else {
	}
}

void StopGifAnimation() {
	if (g_animationTimer) {
		KillTimer(g_hMainWnd, g_animationTimer);
		g_animationTimer = 0;
	}
}

void OnAnimationTimer() {
	static int timerCount = 0;
	timerCount++;
	
	if (g_pGifImage && g_frameCount > 0) {
		g_currentFrame = (g_currentFrame + 1) % g_frameCount;
		
		Status status = g_pGifImage->SelectActiveFrame(&g_timeDimensionID, g_currentFrame);
		
		HRGN hRgn = CreateRectRgn(0, 0, WINDOW_WIDTH, WINDOW_HIGHT);
		
		HRGN hEditRgn = CreateRectRgn(WINDOW_WIDTH / 3 - 20, WINDOW_HIGHT / 3 + 60, 
									  WINDOW_WIDTH / 3 - 20 + 250, WINDOW_HIGHT / 3 + 60 + 25);
		CombineRgn(hRgn, hRgn, hEditRgn, RGN_DIFF);
		DeleteObject(hEditRgn);
		
		HRGN hButtonRgn = CreateRectRgn(WINDOW_WIDTH / 3 + 65, WINDOW_HIGHT / 3 + 90,
										WINDOW_WIDTH / 3 + 65 + 80, WINDOW_HIGHT / 3 + 90 + 30);
		CombineRgn(hRgn, hRgn, hButtonRgn, RGN_DIFF);
		DeleteObject(hButtonRgn);
		
		HRGN hTitleRgn = CreateRectRgn(WINDOW_WIDTH / 3 + 30, WINDOW_HIGHT / 3 + 10,
									   WINDOW_WIDTH / 3 + 30 + 200, WINDOW_HIGHT / 3 + 10 + 30);
		CombineRgn(hRgn, hRgn, hTitleRgn, RGN_DIFF);
		DeleteObject(hTitleRgn);
		
		InvalidateRgn(g_hMainWnd, hRgn, FALSE);
		DeleteObject(hRgn);
		
		if (timerCount % 10 == 0) {
			ShowWindow(g_hInputEdit, SW_SHOW);
			ShowWindow(g_hCheckButton, SW_SHOW);
			SetWindowPos(g_hInputEdit, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(g_hCheckButton, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
	} else {
		static int testFrame = 0;
		testFrame++;
		
		HDC hdc = GetDC(g_hMainWnd);
		HDC hdcMem = CreateCompatibleDC(hdc);
		HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HIGHT);
		HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
		
		HBRUSH hBrush = CreateSolidBrush(RGB(
			(testFrame * 50) % 255,
			(testFrame * 30) % 255,
			(testFrame * 70) % 255 
		));
		RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HIGHT};
		FillRect(hdcMem, &rect, hBrush);
		
		BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HIGHT, hdcMem, 0, 0, SRCCOPY);
		
		SelectObject(hdcMem, hOldBitmap);
		DeleteObject(hBitmap);
		DeleteObject(hBrush);
		DeleteDC(hdcMem);
		ReleaseDC(g_hMainWnd, hdc);
		
	}
}

void DebugLog(const std::wstring& message) {
	SYSTEMTIME st;
	GetLocalTime(&st);
	
	std::wostringstream timestamp;
	timestamp << std::setfill(L'0') << std::setw(2) << st.wHour << L":"
			  << std::setfill(L'0') << std::setw(2) << st.wMinute << L":"
			  << std::setfill(L'0') << std::setw(2) << st.wSecond << L"."
			  << std::setfill(L'0') << std::setw(3) << st.wMilliseconds;

	std::wofstream logFile(g_debugLogPath, std::ios::out | std::ios::app);
	if (logFile.is_open()) {
		logFile << L"[" << timestamp.str() << L"] " << message << std::endl;
		logFile.close();
	}
	
	OutputDebugString(message.c_str());
}
