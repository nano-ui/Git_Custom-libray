#include "FileDialogHelper.h"

#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <algorithm>
#include <cstdio>

//各種パス情報を返す
PathResult FileDialogHelper::OpenGenericFileDialog()
{
    PathResult result_path;
    OPENFILENAMEA ofn;
    char file_path_buffer[MAX_PATH] = "";
    const char* all_files_filter = "All Files (*.*)\0*.*\0";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = file_path_buffer;
    ofn.nMaxFile = sizeof(file_path_buffer);
    ofn.lpstrFilter = all_files_filter;

    const int default_filter_idx = 1;
    ofn.nFilterIndex = default_filter_idx;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;

    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    //ダイアログを表示し、ファイルを選択したか判定
    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        std::filesystem::path absolute_path_obj(file_path_buffer);

        result_path.absolute_path = absolute_path_obj.string();

        std::filesystem::path current_work_dir = std::filesystem::current_path();
        std::filesystem::path relative_path_obj = std::filesystem::relative(absolute_path_obj, current_work_dir);

        result_path.relative_path = relative_path_obj.string();

        std::string ext_str = absolute_path_obj.extension().string();

        //拡張子が存在し、先頭にドットが含まれているか確認
        if (!ext_str.empty() && ext_str.front() == '.')
        {
            ext_str.erase(ext_str.begin());
        }

        //拡張子の大文字小文字を区別せず扱いやすくするため小文字化を行う
        for (size_t i = 0; i < ext_str.length(); i++)
        {
            ext_str[i] = static_cast<char>(std::tolower(ext_str[i]));
        }
        result_path.extension = ext_str;
    }
    return result_path;
}

//各種パス情報を返す
PathResult FileDialogHelper::SaveGenericFileDialog(const std::string& default_ext, const std::string& filter)
{
    PathResult result_path;
    OPENFILENAMEA ofn;
    constexpr size_t path_buffer_size = MAX_PATH;
    char file_path_buffer[path_buffer_size] = "";

    const char default_filter[] = "JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0\0";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = file_path_buffer;
    ofn.nMaxFile = static_cast<DWORD>(path_buffer_size);
    ofn.lpstrFilter = filter.empty() ? default_filter : filter.c_str();

    constexpr DWORD default_filter_index = 1;
    ofn.nFilterIndex = default_filter_index;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = NULL;

    //パス存在確認・上書き警告・ディレクトリ非変更フラグ
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    //ユーザーが拡張子を省略した際に自動付与するデフォルト拡張子を設定
    ofn.lpstrDefExt = default_ext.empty() ? NULL : default_ext.c_str();

    //ダイアログを表示し、保存先ファイル名が決定されたか判定
    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        std::filesystem::path absolute_path_obj(file_path_buffer);

        result_path.absolute_path = absolute_path_obj.string();

        std::filesystem::path current_work_dir = std::filesystem::current_path();
        std::filesystem::path relative_path_obj = std::filesystem::relative(absolute_path_obj, current_work_dir);

        result_path.relative_path = relative_path_obj.string();

        std::string ext_str = absolute_path_obj.extension().string();

        if (!ext_str.empty() && ext_str.front() == '.')
        {
            ext_str.erase(ext_str.begin());
        }

        for (size_t i = 0; i < ext_str.length(); i++)
        {
            ext_str[i] = static_cast<char>(std::tolower(ext_str[i]));
        }
        result_path.extension = ext_str;
    }
    else
    {
        OutputDebugStringA("[情報] SaveGenericFileDialog: ファイル保存ダイアログがキャンセルされたか閉じられました。\n");
    }
    return result_path;
}

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
