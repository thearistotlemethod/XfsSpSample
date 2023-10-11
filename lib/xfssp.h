/*
 *   Copyright (c) 2023 thearistotlemethod@gmail.com
 *   All rights reserved.

 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at

 *   http://www.apache.org/licenses/LICENSE-2.0

 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once
#include <xfsalm.h>
#include <xfsspi.h>
#include <map>
#include <deque>

static HPROVIDER g_hProvider = NULL;
static std::map<HSERVICE, bool> g_h_services;

struct WFS_EVENTS {
	DWORD dwEvent;
	HSERVICE hService;
};

static std::map<HWND, WFS_EVENTS> g_wfs_event;

struct WFS_MSG {
	HWND hWnd;
	LPWFSRESULT lpWFSResult;
	LPVOID lpDataReceived;
	BOOL bCancelled;
};

static std::deque<WFS_MSG*> g_wfs_msg_queue;
static HANDLE g_wfs_queue_mutex = NULL;
static HANDLE hExecuteThread = NULL;

static HANDLE g_serial_mutex;

#define UNLOCKED 0
#define LOCK_PENDING 1
#define LOCKED 2

static int g_lock_state = UNLOCKED;
static HSERVICE g_lock_hservice = NULL;
static HANDLE g_lock_mutex;

