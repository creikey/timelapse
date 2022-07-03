#include "screenshot.h"
// #include "wcap_capture.h"
// #include "sokol_app.h"

// #include <stdio.h>
// #include <windows.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include "lodepng.h"

typedef HANDLE(__stdcall *Func_SetThreadDpiAwarenessContext)(HANDLE);

void screenshot(const char *output_path)
{
  char *why = ""; // for the error
  int x = 0, y = 0, width, height;
  int includeLayeredWindows = 1, all_screens = 1;
  HBITMAP bitmap;
  BITMAPCOREHEADER core;
  HDC screen, screen_copy;
  DWORD rop;
  unsigned char *buffer;
  HANDLE dpiAwareness;
  HMODULE user32;
  Func_SetThreadDpiAwarenessContext SetThreadDpiAwarenessContext_function;

  /* step 1: create a memory DC large enough to hold the
     entire screen */

  screen = CreateDC("DISPLAY", NULL, NULL, NULL);
  screen_copy = CreateCompatibleDC(screen);

  // added in Windows 10 (1607)
  // loaded dynamically to avoid link errors
  user32 = LoadLibraryA("User32.dll");
  SetThreadDpiAwarenessContext_function =
      (Func_SetThreadDpiAwarenessContext)GetProcAddress(
          user32, "SetThreadDpiAwarenessContext");
  if (SetThreadDpiAwarenessContext_function != NULL)
  {
    // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE = ((DPI_CONTEXT_HANDLE)-3)
    dpiAwareness = SetThreadDpiAwarenessContext_function((HANDLE)-3);
  }

  if (all_screens)
  {
    x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  }
  else
  {
    width = GetDeviceCaps(screen, HORZRES);
    height = GetDeviceCaps(screen, VERTRES);
  }

  if (SetThreadDpiAwarenessContext_function != NULL)
  {
    SetThreadDpiAwarenessContext_function(dpiAwareness);
  }

  FreeLibrary(user32);

  bitmap = CreateCompatibleBitmap(screen, width, height);
  if (!bitmap)
  {
    why = "Could not create bitmap";
    goto error;
  }

  if (!SelectObject(screen_copy, bitmap))
  {
    why = "Could not select object";
    goto error;
  }

  /* step 2: copy bits into memory DC bitmap */

  rop = SRCCOPY;
  if (includeLayeredWindows)
  {
    rop |= CAPTUREBLT;
  }
  if (!BitBlt(screen_copy, 0, 0, width, height, screen, x, y, rop))
  {
    why = "Could not blt screen to screen copy";
    goto error;
  }

  /* step 3: extract bits from bitmap */

  // buffer = PyBytes_FromStringAndSize(NULL, height * ((width * 3 + 3) & -4));
  buffer = malloc(height * width * 3);
  if (!buffer)
  {
    why = "Could not malloc buffer";
    goto error;
  }

  core.bcSize = sizeof(core);
  core.bcWidth = width;
  core.bcHeight = height;
  core.bcPlanes = 1;
  core.bcBitCount = 24;
  if (!GetDIBits(
          screen_copy,
          bitmap,
          0,
          height,
          buffer,
          (BITMAPINFO *)&core,
          DIB_RGB_COLORS))
  {
    why = "Could not dibits into the buffer";
    goto error;
  }

  // fix buffer, for some reason R and B channels are flipped and the image is upside down
  for (int i = 0; i < width * height * 3; i += 3)
  {
    unsigned char tmp = buffer[i];
    buffer[i] = buffer[i + 2];
    buffer[i + 2] = tmp;
  }

  // vertically flip buffer
  for (int line = 0; line < height / 2; line += 1)
  {
    for (int pixel = 0; pixel < width*3; pixel += 3)
    {
      int i = line * width*3 + pixel;
      int other_i = (height - line - 1) * width*3 + pixel;
      unsigned char tmp_r = buffer[i];
      unsigned char tmp_g = buffer[i + 1];
      unsigned char tmp_b = buffer[i + 2];

      buffer[i] = buffer[other_i];
      buffer[i + 1] = buffer[other_i + 1];
      buffer[i + 2] = buffer[other_i + 2];

      buffer[other_i] = tmp_r;
      buffer[other_i + 1] = tmp_g;
      buffer[other_i + 2] = tmp_b;
    }
  }

  // save buffer as png
  unsigned char *pngout;
  size_t pngsize;
  unsigned err = lodepng_encode24(&pngout, &pngsize, buffer, width, height);
  if (err)
  {
    why = "Failed to encode png";
    goto error;
  }
  lodepng_save_file(pngout, pngsize, output_path);

  DeleteObject(bitmap);
  DeleteDC(screen_copy);
  DeleteDC(screen);
  free(buffer);
  return;

error:
  printf("Failed to grab screen: %s\n", why);

  DeleteDC(screen_copy);
  DeleteDC(screen);
  DeleteObject(bitmap);
  free(buffer);
}

/*
#include <windows.h>
#define MAX_LOADSTRING 100

Capture cap;

void onClose(void) {
  printf("Closing\n");
}
void onFrame(ID3D11Texture2D* Texture, RECT Rect, UINT64 Time) {
  printf("Got frame\n");
  Capture_Stop(&cap);
}

void screenshot(const char *output_path)
{
  printf("Screenshotting to %s...\n", output_path);
  RECT r = (RECT){
    .left = 0,
    .top = 0,
    .right = 500,
    .bottom = 500,
  };
  Capture_CreateForMonitor(&cap, sapp_d3d11_get_device_context(), 0, &r);
  Capture_Init(&cap, onClose, onFrame);
}
*/