#pragma once

#include <windows.h>
#include <shlobj.h>
#include <sstream>
#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>
#include <filesystem>
#include <cstring>
#include <map>
#include <string_view>

#include <mutex>
#include <thread>
#include <numeric>
#include <future>

#include <fstream>
#include <streambuf>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>
#include <set>

#include "MenuBar/File.h"
#include "MenuBar/Edit.h"
#include "ToolBar/ToolBar.h"
#include "ImageHandler/ImageHandler.h"
#include "StatusBar/StatusBar.h"
#include "Editor/CmdPanel.h"
#include "Editor/TextEditor.h"
#include "FileDialog/FileDialog.h"

//=======================================================Variables==========================================================================
static std::map<std::string, ArmSimPro::TextEditor> Opened_TextEditors;  //Storage for all the instances of text editors that has been opened
static size_t prev_number_opened_texteditor;

static std::filesystem::path SelectedProjectPath; 
static std::filesystem::path NewProjectDir; 

static std::string Project_Name;        // name of the project
static bool UseDefault_Location = true;

const RGBA bg_col = RGBA(24, 24, 24, 255);
const RGBA highlighter_col = RGBA(0, 120, 212, 255);
//==========================================================================================================================================
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
//==================================================TREE VIEW OF DIRECTORY===================================================================
struct DirectoryNode
{
	std::string FullPath;
	std::string FileName;
	std::vector<DirectoryNode> Children;
	bool IsDirectory;
    bool Selected;
};

static DirectoryNode project_root_node;

static std::string Prev_Selected_File_Path;
static std::string Selected_File_Path;          
static std::string Selcted_File_Name;
static std::string Prev_Selected_File_Name;

static void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, std::filesystem::directory_iterator directoryIterator);
static DirectoryNode CreateDirectryNodeTreeFromPath(const std::filesystem::path& rootPath);
static void ImplementDirectoryNode();
static void SearchOnCodeEditor();

const char* ppnames[] = { "NULL", "PM_REMOVE",
    "ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };

const char* ppvalues[] = { 
    "#define NULL ((void*)0)", 
    "#define PM_REMOVE (0x0001)",
    "Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ", 
    "enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0", 
    "enum D3D_FEATURE_LEVEL", 
    "enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
    "#define WINAPI __stdcall",
    "#define D3D11_SDK_VERSION (7)",
    " #define assert(expression) (void)(                                                  \n"
    "    (!!(expression)) ||                                                              \n"
    "    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
    " )"
};

// set your own identifiers
const char* identifiers[] = {
    "HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
    "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
    "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
    "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
const char* idecls[] = 
{
    "typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
    "typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
    "ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
    "ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
    "IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
