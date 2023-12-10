#define _CRT_SECURE_NO_WARNINGS
#include "Utils.hpp"
#include "imgui/imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <unordered_set>

using namespace std;

constexpr wchar_t* SOFTWARE_NAME = L"ArmSim Pro";
const char* LOGO = "";
HWND hwnd = NULL;
//=======================================================================================================================================
static const char* Consolas_Font        = "../../../Utils/Fonts/Consolas.ttf";
static const char* DroidSansMono_Font   = "../../../Utils/Fonts/DroidSansMono.ttf";
static const char* Menlo_Regular_Font   = "../../../Utils/Fonts/Menlo-Regular.ttf";
static const char* MONACO_Font          = "../../../Utils/Fonts/MONACO.TTF";    

//======================================CLASS DECLARATION================================================================================
static ArmSimPro::ToolBar* vertical_tool_bar   = nullptr;
static ArmSimPro::ToolBar* horizontal_tool_bar = nullptr;

static ArmSimPro::StatusBar* status_bar = nullptr;
static ArmSimPro::CmdPanel* cmd_panel = nullptr;
//========================================================================================================================================
       ID3D11Device*            g_pd3dDevice = nullptr; //should be non-static. Other translation units will be using this
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    hwnd = ::CreateWindowW(wc.lpszClassName, 
                    SOFTWARE_NAME, 
                    WS_OVERLAPPEDWINDOW, 
                    0, 
                    0, 
                    1280, 
                    800, 
                    nullptr, 
                    nullptr, 
                    hInstance, 
                    nullptr 
                );
    {
        BOOL USE_DARK_MODE = true;
        BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(hwnd, 
                DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
                &USE_DARK_MODE, 
                sizeof(USE_DARK_MODE))
        );
    }
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); 
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
    io.IniFilename = "ArmSimPro_INI";
    //io.DisplaySize = ImVec2(1280, 720) / io.DisplayFramebufferScale;
    io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

//======================================Load Icons/Images/Fonts==========================================================================================================
    std::future<void> load_icons = std::async(std::launch::async, [&](){
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/Upload.png", &Compile_image.ON_textureID, &Compile_image.width, &Compile_image.height));
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/Upload.png", &Compile_image.OFF_textureID));

        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/Verify.png", &Verify_image.ON_textureID, &Verify_image.width, &Verify_image.height));
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/Verify.png", &Verify_image.OFF_textureID));

        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/Folder.png", &Folder_image.ON_textureID, &Folder_image.width, &Folder_image.height));
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/Folder.png", &Folder_image.OFF_textureID));

        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/Debug.png", &Debug_image.ON_textureID, &Debug_image.width, &Debug_image.height));
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/Debug.png", &Debug_image.OFF_textureID));

        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/RobotArm.png", &Robot_image.ON_textureID, &Robot_image.width, &Robot_image.height));
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/RobotArm.png", &Robot_image.OFF_textureID));

        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/Search.png", &Search_image.ON_textureID, &Search_image.width, &Search_image.height));
        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/Search.png", &Search_image.OFF_textureID));

        //IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/ON/Settings.png", &Settings_image.ON_textureID, &Settings_image.width, &Settings_image.height));
        //IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/OFF/Settings.png", &Settings_image.OFF_textureID));

        IM_ASSERT(LoadTextureFromFile("../../../Utils/icons/process-error.png", &ErroSymbol.textureID, &ErroSymbol.width, &ErroSymbol.height));
    });
    load_icons.get();

    float iconFontSize = 24; 
    static const ImWchar icons_ranges_CI[] = { ICON_MIN_CI, ICON_MAX_CI, 0 };
    static const ImWchar icons_ranges_MDI[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };

    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.GlyphMinAdvanceX = iconFontSize;

    std::future<void> font = std::async(std::launch::async, ([&](){
        ICFont = io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_CI, iconFontSize, &icons_config, icons_ranges_CI );
        IMDIFont = io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_MDI, iconFontSize, &icons_config, icons_ranges_MDI);
        
        DefaultFont         = io.Fonts->AddFontFromFileTTF(Consolas_Font     , 14);
        CodeEditorFont      = io.Fonts->AddFontFromFileTTF(DroidSansMono_Font, 24);
        FileTreeFont        = io.Fonts->AddFontFromFileTTF(Menlo_Regular_Font, 24);
        StatusBarFont       = io.Fonts->AddFontFromFileTTF(MONACO_Font       , 11);
        TextFont            = io.Fonts->AddFontFromFileTTF(Menlo_Regular_Font, 18);
    }));
    font.get();
//================================================================================================================================================================
   
//==================================Initializations===============================================================================================================

    vertical_tool_bar = new ArmSimPro::ToolBar("Vertical", bg_col, 30, ImGuiAxis_Y);
    {
        vertical_tool_bar->AppendTool("Explorer", Folder_image, ImplementDirectoryNode);                
        vertical_tool_bar->AppendTool("Search", Search_image, SearchOnCodeEditor);                
        vertical_tool_bar->AppendTool("Debug", Debug_image, PrintOpenedTextEditor);                  
        vertical_tool_bar->AppendTool("Simulate", Robot_image, nullptr);                    
    }

    horizontal_tool_bar = new ArmSimPro::ToolBar("Horizontal", bg_col, 30, ImGuiAxis_X);
    {
        horizontal_tool_bar->AppendTool("verify", Verify_image, nullptr);   horizontal_tool_bar->SetPaddingBefore("verify", 10);
        horizontal_tool_bar->AppendTool("Upload", Compile_image, nullptr);  horizontal_tool_bar->SetPaddingBefore("Upload", 5);
    }

    status_bar = new ArmSimPro::StatusBar("status", 30, horizontal_tool_bar->GetbackgroundColor());
    cmd_panel = new ArmSimPro::CmdPanel("Command Line", status_bar->GetHeight(), bg_col, highlighter_col);

//==================================================Texture for File Dialog==============================================================  
    
    ArmSimPro::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt){
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = fmt==0 ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        ID3D11ShaderResourceView* out_srv=NULL;
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &out_srv);
        pTexture->Release();

        return (void*)out_srv;
    };

    ArmSimPro::FileDialog::Instance().DeleteTexture = [](void* tex){
        ID3D11ShaderResourceView* texture = static_cast<ID3D11ShaderResourceView*>(tex);
        texture->Release();
    };
//==================================================================================================================================================================
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        { 
            // Create a project
            if(!SelectedProjectPath.empty() && project_root_node.FileName.empty() && project_root_node.FullPath.empty()) 
                project_root_node = CreateDirectryNodeTreeFromPath(SelectedProjectPath);

            ImGui::PushFont(DefaultFont);
                float main_menubar_height;
                if(ImGui::BeginMainMenuBar())
                {   
                    if (ImGui::BeginMenu("File"))
                    {
                        if (ImGui::MenuItem("\tNew Sketch", "CTRL+N")) {}
                        if (ImGui::MenuItem("\tNew file", "CTRL+Alt+Windows+N")) {}
                        if (ImGui::MenuItem("\tNew Window", "CTRL+Shift+N")) {}
                        ImGui::Separator();
                        if (ImGui::MenuItem("\tOpen", "CTRL+O")) 
                        {} 
                        if (ImGui::MenuItem("\tClose", "CTRL+W")) {}

                        static bool auto_save = false;
                        ImGui::MenuItem("\tAuto Save", "", &auto_save);
                        if (auto_save) {}

                        if (ImGui::MenuItem("\tSave", "CTRL+S")) 
                        {
                            //auto textToSave = focused_editor->second.GetText();
                            /// save text....
                        } 

                        if (ImGui::MenuItem("\tSave As", "CTRL+Shift+S")) {} 
                        ImGui::Separator();
                        if (ImGui::MenuItem("\tCut", "CTRL+X")) {}
                        if (ImGui::MenuItem("\tCopy", "CTRL+C")) {}

                        ImGui::Separator();
                        if (ImGui::MenuItem("\tQuit", "CTRL+Q")) 
                            break;
                        
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Edit"))
                    {   
                        static ArmSimPro::TextEditor *focused_editor = nullptr;
                        if((selected_window_path != prev_selected_window_path) && !Opened_TextEditors.empty() || Opened_TextEditors.size() != prev_number_docked_window){
                            prev_selected_window_path = selected_window_path;
                            auto iterator = std::find(Opened_TextEditors.begin(), Opened_TextEditors.end(), selected_window_path); 
                            if(iterator != Opened_TextEditors.end())
                                focused_editor = &(*iterator);
                        }
                        
                        bool IsWindowShowed = (focused_editor != nullptr)? focused_editor->IsWindowVisible() : false;
                        bool NoEditor_Selected = Opened_TextEditors.empty() || (focused_editor != nullptr && IsWindowShowed)? true : false;
                        bool ro = (focused_editor != nullptr && IsWindowShowed)? focused_editor->IsReadOnly() : true;

                    ArmSimPro::MenuItemData menu_item_arr[] = {
                        ArmSimPro::MenuItemData("\tUndo", "CTRL+Z", nullptr, (!ro && (focused_editor != nullptr && IsWindowShowed)? focused_editor->CanUndo() : false), [&](){focused_editor->Undo();}),
                        ArmSimPro::MenuItemData("\tRedo", "CTRL+Y", nullptr, (!ro && (focused_editor != nullptr && IsWindowShowed)? focused_editor->CanRedo() : false), [&](){focused_editor->Redo();}),
                        //, seperator here
                        ArmSimPro::MenuItemData("\tCut", "CTRL+X", nullptr, (!ro && (focused_editor != nullptr && IsWindowShowed)? focused_editor->HasSelection() : false), [&](){focused_editor->Cut();}),
                        ArmSimPro::MenuItemData("\tCopy", "CTRL+C", nullptr, (!ro && (focused_editor != nullptr && IsWindowShowed)?  focused_editor->HasSelection() : false), [&](){focused_editor->Copy();}),
                        ArmSimPro::MenuItemData("\tDelete", "Del", nullptr, (!ro && (focused_editor != nullptr && IsWindowShowed)? focused_editor->HasSelection() : false), [&](){focused_editor->Delete();}),
                        ArmSimPro::MenuItemData("\tPaste", "Ctrl+V", nullptr, (!ro && ImGui::GetClipboardText() != nullptr), [&](){focused_editor->Paste();}),
                        //,seperator here
                            ArmSimPro::MenuItemData("\tSelect all", nullptr, nullptr, focused_editor != nullptr && IsWindowShowed, [&](){focused_editor->SetSelection(ArmSimPro::TextEditor::Coordinates(), ArmSimPro::TextEditor::Coordinates(focused_editor->GetTotalLines(), 0));})
                    };
                        
                        for(unsigned int i = 0; i < IM_ARRAYSIZE(menu_item_arr); i++)
                        {
                            ArmSimPro::MenuItem(menu_item_arr[i], true);
                            if(i == 1 || i == 5)
                                ImGui::Separator();
                        }
                        ImGui::EndMenu();
                    }

                    main_menubar_height = ImGui::GetWindowHeight();
                    ImGui::EndMainMenuBar();
                }
                
                horizontal_tool_bar->SetToolBar(main_menubar_height + 10);
                vertical_tool_bar->SetToolBar(horizontal_tool_bar->GetThickness(), status_bar->GetHeight() + 17);

                status_bar->BeginStatusBar();
                {
                    float width = ImGui::GetWindowWidth();
                    char buffer[255];

                    if(!current_editor.empty())
                    {
                        static ImVec2 textSize; 
                        if(textSize.x == NULL)
                            textSize = ImGui::CalcTextSize(buffer); //this is a bottleneck function. should prevent it from always calculatin
                        ImGui::SetCursorPosX(width - (textSize.x - 450));
                        ImGui::Text(current_editor.c_str());
                    }
                }
                status_bar->EndStatusBar();

                cmd_panel->SetPanel(100, vertical_tool_bar->GetTotalWidth());
            ImGui::PopFont(); //default font

            //Code Editor dockable panel configuration
            {
                ImGuiViewport* viewport = ImGui::GetMainViewport();

                ImVec2 size, pos;
                {                       
                    const float menubar_toolbar_total_thickness = horizontal_tool_bar->GetThickness() + (main_menubar_height + 10);

                    pos[ImGuiAxis_X]  = viewport->Pos[ImGuiAxis_X] + vertical_tool_bar->GetTotalWidth() + 20;
                    pos[ImGuiAxis_Y]  = viewport->Pos[ImGuiAxis_Y] + menubar_toolbar_total_thickness + 8;

                    size[ImGuiAxis_X] = viewport->WorkSize.x - vertical_tool_bar->GetTotalWidth() - 20;
                    size[ImGuiAxis_Y] = viewport->WorkSize.y - (cmd_panel->GetCurretnHeight() + status_bar->GetHeight() + 47);
                }

                //Creates DockSpace
                DockSpace(size, pos);
            
                if(Opened_TextEditors.empty() && project_root_node.FileName.empty() && project_root_node.FullPath.empty())
                    WelcomPage();
                
                // Render Text Editors
                ImGui::PushFont(CodeEditorFont);
                if(!Opened_TextEditors.empty())
                {   
                    //make sure to have no duplicates                    
                    std::sort(Opened_TextEditors.begin(), Opened_TextEditors.end(), [](const ArmSimPro::TextEditor& e1, const ArmSimPro::TextEditor& e2){
                        return e1.GetPath() < e2.GetPath();
                    });

                    auto it = std::unique(Opened_TextEditors.begin(), Opened_TextEditors.end(), [](const ArmSimPro::TextEditor& e1, const ArmSimPro::TextEditor& e2){
                        return e1.GetPath() == e2.GetPath();
                    });
                    Opened_TextEditors.erase(it, Opened_TextEditors.end());
                    RenderTextEditors();
                }
                ImGui::PopFont();
            }
        }
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    SafeDelete<ArmSimPro::ToolBar>(vertical_tool_bar);
    SafeDelete<ArmSimPro::ToolBar>(horizontal_tool_bar);
    SafeDelete<ArmSimPro::CmdPanel>(cmd_panel);
    SafeDelete<ArmSimPro::StatusBar>(status_bar);

    SafeDelete<ImFont>(DefaultFont);
    SafeDelete<ImFont>(CodeEditorFont);
    SafeDelete<ImFont>(FileTreeFont);
    SafeDelete<ImFont>(StatusBarFont);
    SafeDelete<ImFont>(TextFont);
    return 0;
}

void SearchOnCodeEditor()
{   
    static std::string buffer;

    ImGui::Dummy(ImVec2(0.0f, 13.05f));
    ImGui::Dummy(ImVec2(5.0f, 13.05f));
    ImGui::SameLine();

    ImGui::PushFont(TextFont);
    {   
        static bool Show_Replace_InputText = false;
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(49,49,49,255));
            if(ImGui::Button(">"))
                Show_Replace_InputText = true;
            ImGui::SetItemTooltip("Toggle Replace");
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(3.0f, 13.05f));
            ImGui::SameLine();
        ImGui::PopStyleColor(2);

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(49,49,49,255));
        ImGui::PushItemWidth(-1);
            ImGui::InputTextWithHint("##Search", "Search", &buffer);
            ImGui::SetItemTooltip("Search");
        ImGui::PopItemWidth();
        ImGui::PopStyleColor(2);
    }
    ImGui::PopFont();
}

//===============================================Tree View of directory Impl=====================================================

void RecursivelyAddDirectoryNodes(DirectoryNode& parentNode, std::filesystem::directory_iterator& directoryIterator)
{   
    for (const std::filesystem::directory_entry& entry : directoryIterator)
	{
		DirectoryNode& childNode = parentNode.Children.emplace_back();
		childNode.FullPath = entry.path().u8string();
		childNode.FileName = entry.path().filename().u8string();
        childNode.Selected = false;
		if (childNode.IsDirectory = entry.is_directory(); childNode.IsDirectory)
			RecursivelyAddDirectoryNodes(childNode, std::filesystem::directory_iterator(entry));
	}
	auto moveDirectoriesToFront = [](const DirectoryNode& a, const DirectoryNode& b) { return (a.IsDirectory > b.IsDirectory); };
	std::sort(parentNode.Children.begin(), parentNode.Children.end(), moveDirectoriesToFront);
}

//static std::mutex dir_tree;
DirectoryNode CreateDirectryNodeTreeFromPath(const std::filesystem::path& rootPath)
{   
    //std::lock_guard<std::mutex> lock_dir_tree(dir_tree);
    
    DirectoryNode rootNode;
	rootNode.FullPath = rootPath.u8string();
	rootNode.FileName = rootPath.filename().u8string();

	if (rootNode.IsDirectory = std::filesystem::is_directory(rootPath); rootNode.IsDirectory)
        RecursivelyAddDirectoryNodes(rootNode, std::filesystem::directory_iterator(rootPath));

	return rootNode;
}

//static std::mutex m_mPreprocIdentifiers;
void SetupPreprocIdentifiers(ArmSimPro::TextEditor::LanguageDefinition& programming_lang, const char* value)
{
    //std::lock_guard<std::mutex> lock_mPreprocIdentifiers(m_mPreprocIdentifiers);

    ArmSimPro::TextEditor::Identifier id;
    id.mDeclaration = value;
    programming_lang.mPreprocIdentifiers.insert(std::make_pair(std::string(value), id));
}

//static std::mutex m_Identifiers;
void SetupIdentifiers(ArmSimPro::TextEditor::LanguageDefinition& programming_lang, const char* value, const char* idecls)
{
    //std::lock_guard<std::mutex> lock_Identifiers(m_Identifiers);

    ArmSimPro::TextEditor::Identifier id;
    id.mDeclaration = std::string(idecls);
    programming_lang.mIdentifiers.insert(std::make_pair(std::string(value), id));
}

void RecursivelyDisplayDirectoryNode(DirectoryNode& parentNode)
{   
    static std::set<ImGuiID> selections_storage;
    static ImGuiID selection;
    
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_SpanFullWidth;
    ImGui::PushID(&parentNode);

    switch(parentNode.IsDirectory)
    {
    case true: //Node is directory
        {   
            ImGui::PushFont(FileTreeFont);
            if (ImGui::TreeNodeEx(parentNode.FileName.c_str(), node_flags))
            {   
                for (DirectoryNode& childNode : parentNode.Children)
                    RecursivelyDisplayDirectoryNode(childNode);
                
                ImGui::TreePop();
            }
            ImGui::PopFont();

            ImGui::PushFont(TextFont);
            if(ImGui::IsItemClicked(ImGuiMouseButton_Right) && (!ImGui::IsItemToggledOpen() || ImGui::IsItemToggledOpen()))
                ImGui::OpenPopup("Edit Folder");

            if(ImGui::BeginPopup("Edit Folder")) 
            {
                // s => Seperator
                const char* popup_items[]   = {"\tNew File...\t", "\tNew Folder...\t", "\tReveal in File Explorer\t", "s", "\tCut\t", "\tCopy\t", "s","\tCopy Relative Path\t", "s", "\tRename...\t", "\tDelete\t"};
                
                for(int i = 0; i < IM_ARRAYSIZE(popup_items); i++)
                {
                    if(strcmp(popup_items[i], "s") == 0){
                        ImGui::Separator();
                        continue;
                    }
                    ImGui::MenuItem(popup_items[i]);
                }
                ImGui::EndPopup();
            }
            ImGui::PopFont();
            
        } break;
    
    case false:
        {   
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGuiID pressed_id = window->GetID(parentNode.FullPath.c_str());

            if(pressed_id == selection || selections_storage.find(pressed_id) != selections_storage.end())
                node_flags |= ImGuiTreeNodeFlags_Selected;

            ImGui::PushFont(FileTreeFont);
            ImGui::TreeNodeEx(parentNode.FileName.c_str(), node_flags);
            ImGui::PopFont();
            if(ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
            {   
                if(ImGui::GetIO().KeyCtrl)
                    selections_storage.insert(pressed_id);
                else{
                    selection = pressed_id;
                    selections_storage.clear();
                }

                //if(ImGui::IsMouseDoubleClicked(0))
                {  
                    auto it = std::find(Opened_TextEditors.begin(), Opened_TextEditors.end(), parentNode.FullPath);
                    if(it == Opened_TextEditors.end())
                    {   
                        static unsigned int id = 0;
                        ArmSimPro::TextEditor editor(parentNode.FullPath, id,bg_col.GetCol());
                        auto programming_lang = ArmSimPro::TextEditor::LanguageDefinition::CPlusPlus();
                        ++id;
                        for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
                            std::future<void> PreprocIdentifier = std::async(std::launch::async, SetupPreprocIdentifiers, programming_lang, ppvalues[i]);
                        for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
                            std::future<void> Identifiers = std::async(std::launch::async, SetupIdentifiers, programming_lang, identifiers[i], idecls[i]);
                
                        editor.SetLanguageDefinition(programming_lang);

                        std::ifstream t(parentNode.FullPath.c_str());
                        if (t.good())
                        {
                            std::string str;
                            t.seekg(0, std::ios::end);
                                str.reserve(t.tellg());
                            t.seekg(0, std::ios::beg);
                            str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

                            editor.SetText(str);
                        }

                        if(!selected_editor_path.empty() && !Opened_TextEditors.empty() && selected_editor_path != prev_editor_path){
                            prev_editor_path = selected_editor_path;
                            int index = GetTextEditorIndex(selected_editor_path);
                            if(index != -1)
                                Opened_TextEditors.insert(Opened_TextEditors.begin() + index, editor);
                            else 
                                Opened_TextEditors.push_back(editor);
                        }
                        else if(Opened_TextEditors.empty())
                            Opened_TextEditors.push_back(editor);
                    }
                }
            }    
            else if(ImGui::IsItemClicked(ImGuiMouseButton_Right) && !ImGui::IsItemToggledOpen())
                ImGui::OpenPopup("Edit File");
            
            ImGui::PushFont(DefaultFont);
            if(ImGui::BeginPopup("Edit File"))
            {
                // s => Seperator
                const char* popup_items[]   = {"\tCut\t", "\tCopy\t", "s", "\tCopy Path\t", "\tCopy Relative Path\t", "s", "\tRename...\t", "\tDelete\t"};
                const char* key_shortcuts[] = {"Ctrl+X", "Ctrl+C", "Shift+Alt+C", "Ctrl+K Ctrl+Shift+C", "F2", "Delete"}; 
                int k = 0;
                for(int i = 0; i < IM_ARRAYSIZE(popup_items); i++)
                {
                    if(strcmp(popup_items[i], "s") == 0){
                        ImGui::Separator();
                        continue;
                    }
                    ImGui::MenuItem(popup_items[i], key_shortcuts[k]);
                    k += 1;
                }
                ImGui::EndPopup();
            }
            ImGui::PopFont();
        } break;
    }
	
	ImGui::PopID();
}

static std::mutex implement_directory_node;
void ImplementDirectoryNode()
{
    std::lock_guard<std::mutex> lock(implement_directory_node);
    static bool is_Open = true;
    ImGui::Dummy(ImVec2(0.0f, 13.05f));
    ImGui::Dummy(ImVec2(6.0f, 13.05f));
    ImGui::SameLine();

    float width = ImGui::GetWindowWidth();
    if(SelectedProjectPath.empty())
    {
        static const char* file_dialog_key = nullptr;
        ImGui::PushFont(TextFont);
        const int posX = 14;
        ImGui::SetCursorPosX(posX);
        ImGui::TextWrapped("You have not opened a project folder.\n\nYou can open an existing PlatformIO-based project (a folder that contains platformio.ini file).\n\n");
        ImGui::SetCursorPosX(posX);
        if(ImGui::Button("Open Folder", ImVec2(width - 30, 0)))
            ArmSimPro::FileDialog::Instance().Open("SelectProject", "Select project directory", "");
        
        OpenFileDialog(SelectedProjectPath, "SelectProject");

//=========================================================================Create New Project============================================================================================================================================================== 
        
        ImGui::SetCursorPosX(posX);
        ImGui::TextWrapped("\nYou can create a new PlatformIo based Project or explore the examples of ArmSim Kit\n\n");
        ImGui::SetCursorPosX(posX);
        if(ImGui::Button("Create New Project", ImVec2(width - 30, 0)))
            ImGui::OpenPopup("Project Wizard");

        ImGui::SetNextWindowSize(ImVec2(700, 300));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(50.0f, 10.0f));
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, bg_col.GetCol());
        ImGui::PushStyleColor(ImGuiCol_TitleBg, bg_col.GetCol());
        if(ImGui::BeginPopupModal("Project Wizard", &is_Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {   
            ProjectWizard();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::PopFont();
    }

//========================================================================================================================================================================
    //executed once
    if(!project_root_node.FileName.empty() && !project_root_node.FullPath.empty())
        RecursivelyDisplayDirectoryNode(project_root_node);    
}

void DockSpace(const ImVec2& size, const ImVec2& pos)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus; 
    
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 10)); //used to change window titlebar height;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, bg_col.GetCol());
    ImGui::PushStyleColor(ImGuiCol_TitleBg, bg_col.GetCol());
    ImGui::Begin("DockSpace", nullptr, window_flags);
    {    
        ImGuiIO& io = ImGui::GetIO();
        if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            static ImGuiID dockspace_id = 0;
            if(dockspace_id == 0) 
                dockspace_id = ImGui::GetID("MyDockspace");

            const ImVec2 dockspace_size = ImGui::GetContentRegionAvail();
            if(ShouldShowWelcomePage())
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags | ImGuiDockNodeFlags_NoWindowMenuButton);
            else
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

            static bool first_run = true;
            if(ShouldShowWelcomePage() && first_run){
                first_run = false;
                ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, dockspace_size);

                ImGui::DockBuilderDockWindow(WELCOME_PAGE, dockspace_id);

                ImGui::DockBuilderFinish(dockspace_id);
            }
            else if(Opened_TextEditors.size() != prev_number_docked_window && !Opened_TextEditors.empty())
            {   
                ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, dockspace_size);

                //Use this part to dock all selected window. Except to those window that are removed to the dock space
                prev_number_docked_window = Opened_TextEditors.size();
                unsigned int i = 0;
                for(const auto& window : Opened_TextEditors){
                    std::future<void> result = std::async(std::launch::async, [&](){
                        ImGui::DockBuilderDockWindow(window.GetTitle().c_str(), dockspace_id);
                    });
                }
                ImGui::DockBuilderFinish(dockspace_id);
            }
        }
    }
    ImGui::PopStyleColor(2);
    ImGui::End();
    ImGui::PopStyleVar(4);
    
}
//================================================================================================================================
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);

        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, nullptr, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}