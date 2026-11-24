#include "ScreenshotManager.h"

#include <Windows.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <vector>

namespace SystemInfoAgent::Screenshot 
{
    namespace {

        // Builds "<ComputerName>_<ddMMyyyy>_<HHmmss>.bmp" -- a filename unique
        // enough to avoid collisions between captures and across machines.
        std::wstring GenerateScreenshotFilename(size_t monitorIdx)
        {
            wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = {};
            DWORD nameSize = MAX_COMPUTERNAME_LENGTH + 1;
            ::GetComputerNameW(computerName, &nameSize);

            std::time_t now = std::time(nullptr);
            std::tm localTime{};
            localtime_s(&localTime, &now);

            wchar_t timestamp[32] = {};
            std::wcsftime(timestamp, sizeof(timestamp) / sizeof(wchar_t), L"%d%m%Y_%H%M%S", &localTime);

            return std::wstring(computerName) + L"_" + timestamp + L"_" + std::to_wstring(monitorIdx) + L".bmp";
        }

        bool SaveBitmap(HBITMAP bitmap, HDC hdc, int width, int height, size_t monitorIdx,const std::wstring& outputDirectory)
        {
            BITMAPINFOHEADER bi = {};
            bi.biSize = sizeof(BITMAPINFOHEADER);
            bi.biWidth = width;
            bi.biHeight = -height; // Top-down bitmap
            bi.biPlanes = 1;
            bi.biBitCount = 32;
            bi.biCompression = BI_RGB;

            DWORD imageSize = width * height * 4;

            BITMAPFILEHEADER fileHeader = {};
            fileHeader.bfType = 0x4D42; // BM
            fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            fileHeader.bfSize = fileHeader.bfOffBits + imageSize;

            //BYTE* pixels = new BYTE[imageSize];
            std::vector<BYTE> pixels(imageSize);

            if (!GetDIBits(
                hdc,
                bitmap,
                0,
                height,
                pixels.data(),
                (BITMAPINFO*)&bi,
                DIB_RGB_COLORS))
            {
                //delete[] pixels;
                return false;
            }

            std::wstring fileName = GenerateScreenshotFilename(monitorIdx);
            std::wstring fullPath = outputDirectory.empty() ? fileName : ( std::filesystem::path(outputDirectory) / fileName ).wstring();

            std::ofstream file(fullPath, std::ios::binary);

            if (!file)
            {
                //delete[] pixels;
                return false;
            }

            file.write((char*)&fileHeader, sizeof(fileHeader));
            file.write((char*)&bi, sizeof(bi));
            file.write((char*)pixels.data(), imageSize);

            file.close();
            //delete[] pixels;

            return true;
        }

        /*
        - EnumDisplayMonitors asks Windows "tell me about every monitor you know about." 
        - For each monitor it finds, Windows calls your MonitorCallback function once, 
           handing it that monitor's rectangle (its position and size in virtual-desktop coordinates). 
        - Your callback just appends that rectangle into monitorRects. 
        - After this line runs, monitorRects contains one RECT per connected monitor 
        — e.g. on a 2-monitor setup, monitorRects[0] might be {0, 0, 1920, 1080} (primary monitor) 
            and monitorRects[1] might be {1920, 0, 3840, 1080} (a second monitor to the right). 
        - As flagged, this data is gathered but not used yet.
        */

        BOOL CALLBACK MonitorCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT rect, LPARAM data)
        {
            auto* monitors = reinterpret_cast<std::vector<RECT>*>(data);
            monitors->push_back(*rect);
            return TRUE;
        }
    }

    bool ScreenshotManager::capture(const std::wstring& outputPath)
    {
        bool allMonitorScreenshotTaken = true;

        std::vector<RECT> monitorRects;
        EnumDisplayMonitors(nullptr, nullptr, MonitorCallback, reinterpret_cast<LPARAM>(&monitorRects));

        // permission/access to a drawing surface
        HDC screenDC = GetDC(nullptr);  

        for(size_t i=0; i<monitorRects.size(); i++)
        {
            const RECT& rect = monitorRects[i];

            // Get the screen dimensions
            int virtualLeft = rect.left;  // For extra added monitors
            int virtualTop = rect.top;   // For extra added monitors
            int width = rect.right - rect.left;   // full virtual desktop width
            int height = rect.bottom - rect.top;   // full virtual desktop height

            // Create a temporary memory drawing area where you can store the captured screen image
            HDC memoryDC = CreateCompatibleDC(screenDC);

            // This creates a bitmap with dimensions (e.g 1920 × 1080 pixels)
            HBITMAP bitmap = CreateCompatibleBitmap(screenDC, width, height);   // At this stage, the bitmap exists, but it does not contain your screenshot yet.

            // Attaches the newly created bitmap to your memory DC
            HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap);

            // Copies the screen pixels into selected bitmap
            BitBlt(
                memoryDC, 
                0, 0, 
                width, height, 
                screenDC, 
                virtualLeft, virtualTop, 
                SRCCOPY
            );

            /*
                Windows does not allow you to delete a bitmap while it is still selected into a device context.
                so, Restore the original bitmap -> Delete your screenshot bitmap. -> Delete the memory DC.
            */
            SelectObject(memoryDC, oldBitmap);

            // Save screenshot
            if (!SaveBitmap(bitmap, screenDC, width, height, i, outputPath))
            {
                //std::cout << "Screenshot saved failed!" << std::endl;
                allMonitorScreenshotTaken = false;
            }

            DeleteObject(bitmap);
            DeleteDC(memoryDC);
        }

        // screenDC is not "one monitor's resource" — it's a single handle representing the whole virtual desktop, acquired once.
        // whereas bitmap and memoryDC is per monitor resource, that why we released bitmap and memoryDC inside loop, and screenDC outside loop.
        ReleaseDC(NULL, screenDC);
        
        return allMonitorScreenshotTaken; // Capture component not configured.
    }
}   // SystemInfoAgent::Screenshot


/*
* This stores the ss of all windows into one file, suppose 2 monitors then both monitors ss store in one single .bmp file
 bool ScreenshotManager::capture(const std::wstring& outputPath)
    {
        bool isScreenshotTaken = false;

        std::vector<RECT> monitorRects;
        EnumDisplayMonitors(nullptr, nullptr, MonitorCallback, reinterpret_cast<LPARAM>(&monitorRects));

        // permission/access to a drawing surface
        HDC screenDC = GetDC(nullptr);  

        // Get the screen dimensions
        int virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);  // For extra added monitors
        int virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);   // For extra added monitors
        int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);   // full virtual desktop width
        int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);   // full virtual desktop height
        //int width = GetDeviceCaps(screenDC, HORZRES);   //  primary-monitor-only
        //int height = GetDeviceCaps(screenDC, VERTRES);  //  primary-monitor-only

        // Create a temporary memory drawing area where you can store the captured screen image
        HDC memoryDC = CreateCompatibleDC(screenDC);

        // This creates a bitmap with dimensions (e.g 1920 × 1080 pixels)
        HBITMAP bitmap = CreateCompatibleBitmap(screenDC, width, height);   // At this stage, the bitmap exists, but it does not contain your screenshot yet.

        // Attaches the newly created bitmap to your memory DC
        HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap);

        // Copies the screen pixels into selected bitmap
        BitBlt(
            memoryDC, 
            0, 0, 
            width, height, 
            screenDC, 
            virtualLeft, virtualTop, 
            SRCCOPY
        );

        //   Windows does not allow you to delete a bitmap while it is still selected into a device context.
        //    so, Restore the original bitmap -> Delete your screenshot bitmap. -> Delete the memory DC.
        
        SelectObject(memoryDC, oldBitmap);

        // Save screenshot
        if (SaveBitmap(bitmap, screenDC, width, height, outputPath))
        {
            //std::cout << "Screenshot saved successfully!" << std::endl;
            isScreenshotTaken = true;
        }
        else
        {
            //std::cout << "Screenshot saving failed!" << std::endl;
            isScreenshotTaken = false;
        }

        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        ReleaseDC(NULL, screenDC);

       if (outputPath.empty())
            return false;

        return isScreenshotTaken; // Capture component not configured.
    }

*/