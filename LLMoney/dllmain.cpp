﻿// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "framework.h"
#include "Money.h"
#include <LLAPI.h>

//导入库
#pragma comment(lib,"../SDK/Lib/third-party/MySQL/libmysql.lib")

void entry();

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        LL::registerPlugin("LLMoney", "EconomyCore for LiteLoaderBDS", LL::Version{ 2,0, std::stoi(_ver) }, "https://github.com/LiteLDev/LiteLoaderPlugins", "GPLv3");
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" {
    _declspec(dllexport) void onPostInit() {
        std::ios::sync_with_stdio(false);
        entry();
    }
}