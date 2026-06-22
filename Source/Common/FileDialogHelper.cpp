#include "FileDialogHelper.h"

#include <windows.h>
#include <commdlg.h>
#include <cstdio>

//ファイルを開くダイアログを表示して絶対パスを返す
std::string FileDialogHelper::OpenFileDialog(const std::string& default_dir, const std::string& filter)
{
    OPENFILENAMEA ofn;
    char file_path_buffer[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = file_path_buffer;
    ofn.nMaxFile = sizeof(file_path_buffer);
    ofn.lpstrFilter = filter.c_str();

    const int default_filter_idx = 1;
    ofn.nFilterIndex = default_filter_idx;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = default_dir.c_str();

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        return std::string(file_path_buffer);
    }

    return "";
}

//ファイルを保存ダイアログを表示して絶対パスを返す
std::string FileDialogHelper::SaveFileDialog(const std::string& default_dir, const std::string& filter, const std::string& default_ext)
{
    OPENFILENAMEA ofn;
    char file_path_buffer[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = file_path_buffer;
    ofn.nMaxFile = sizeof(file_path_buffer);
    ofn.lpstrFilter = filter.c_str();

    const int default_filter_idx = 1;
    ofn.nFilterIndex = default_filter_idx;

    ofn.lpstrInitialDir = default_dir.c_str();

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = default_ext.c_str();

    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        return std::string(file_path_buffer);
    }

    return "";
}
