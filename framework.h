// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <atlstr.h>
#include <strsafe.h>
#include <wil/result.h>
#include <windows.h>
#include <wrl.h>
// C RunTime Header Files
#include <cpprest/json.h>
#include <malloc.h>
#include <memory.h>
#include <memory>
#include <stdlib.h>
#include <tchar.h>

// App specific includes
#include "resource.h"
#include "webview2.h"

#define DEFAULT_DPI 96
#define MIN_WINDOW_WIDTH 700
#define MIN_WINDOW_HEIGHT 400
#define MAX_LOADSTRING 256

#define MG_NAVIGATE 1
#define MG_UPDATE_URI 2
#define MG_GO_FORWARD 3
#define MG_GO_BACK 4
#define MG_NAV_STARTING 5
#define MG_NAV_COMPLETED 6
#define MG_RELOAD 7
#define MG_CANCEL 8
