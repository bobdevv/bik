#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <string>
#include <vector>

namespace bik {

class BackupManager;
struct BackupInfo;

class MainWindow : public Fl_Window {
public:
    MainWindow(int w, int h, const char* title);
    ~MainWindow();
    
    void refreshBackupList();
    
private:
    static void onLoadBackup(Fl_Widget* w, void* data);
    static void onCreateBackup(Fl_Widget* w, void* data);
    static void onDeleteBackup(Fl_Widget* w, void* data);
    static void onRefresh(Fl_Widget* w, void* data);
    static void onBrowserSelect(Fl_Widget* w, void* data);
    
    void loadSelectedBackup();
    void createNewBackup();
    void deleteSelectedBackup();
    void updateDetails();
    
    Fl_Browser* m_backupBrowser;
    Fl_Box* m_detailsBox;
    Fl_Box* m_projectInfoBox;
    Fl_Button* m_loadButton;
    Fl_Button* m_createButton;
    Fl_Button* m_deleteButton;
    Fl_Button* m_refreshButton;
    
    BackupManager* m_manager;
    std::vector<BackupInfo> m_backups;
};

} // namespace bik
