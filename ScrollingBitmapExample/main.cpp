#include <windows.h>
#include <thread>
#include <atomic>
#include "ScrollingBitmapExample.h"

HBITMAP hBitmap = NULL;
void* pBits = NULL;
const int WIDTH = 256;
const int HEIGHT = 300;
RECT dirtyRect = { 0, 0, WIDTH, HEIGHT };
std::atomic<bool> running{ true };

BYTE* circularBuffer = nullptr;      // Circular buffer to store row data
int currentRow = 0;                  // Tracks the current row to be written

void InitBitmap(HWND hwnd) {
    BITMAPINFO bmpInfo;
    ZeroMemory(&bmpInfo, sizeof(bmpInfo));

    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = WIDTH;
    bmpInfo.bmiHeader.biHeight = HEIGHT;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(hwnd);
    hBitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, &pBits, NULL, 0);
    ReleaseDC(hwnd, hdc);

    if (!hBitmap) {
        MessageBox(hwnd, L"Failed to create DIB section!", L"Error", MB_OK);
    }

    // Allocate memory for circular buffer (stores one row per cycle)
    circularBuffer = new BYTE[WIDTH * 3 * HEIGHT];
}

void GenerateNewRow() {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(0));
        seeded = true;
    }

    BYTE* newRow = circularBuffer + (currentRow * WIDTH * 3);

    for (int x = 0; x < WIDTH; ++x) {
        newRow[x * 3 + 0] = rand() % 256; // Blue
        newRow[x * 3 + 1] = rand() % 256; // Green
        newRow[x * 3 + 2] = rand() % 256; // Red
    }

    currentRow = (currentRow + 1) % HEIGHT;  // Update row in circular buffer
}

void BackgroundRowGenerator() {
    while (running) {
        GenerateNewRow();
        Sleep(0);  // Slight delay to reduce CPU load
    }
}

void UpdateBitmapData() {
    if (pBits == NULL || circularBuffer == NULL) return;

    int rowSize = WIDTH * 3;
    BYTE* bitmapData = (BYTE*)pBits;

    for (int y = 0; y < HEIGHT; ++y) {
        int bufferRow = (currentRow + y) % HEIGHT;
        BYTE* srcRow = circularBuffer + (bufferRow * rowSize);
        BYTE* destRow = bitmapData + (y * rowSize);

        memcpy(destRow, srcRow, rowSize);  // Copy row from circular buffer to bitmap
    }

    dirtyRect.left = 0;
    dirtyRect.right = WIDTH;
    dirtyRect.top = 0;
    dirtyRect.bottom = HEIGHT;
}

void PaintBitmap(HWND hwnd, HDC hdc) {
    BITMAPINFO bmpInfo;
    ZeroMemory(&bmpInfo, sizeof(bmpInfo));

    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = WIDTH;
    bmpInfo.bmiHeader.biHeight = HEIGHT;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(hdc, 0, 0, WIDTH, HEIGHT, 0, 0, 0, HEIGHT, pBits, &bmpInfo, DIB_RGB_COLORS);
}

void Cleanup() {
    running = false;  // Stop the background thread
    if (circularBuffer) {
        delete[] circularBuffer;
        circularBuffer = nullptr;
    }
    if (hBitmap) {
        DeleteObject(hBitmap);
        hBitmap = NULL;
    }
}
